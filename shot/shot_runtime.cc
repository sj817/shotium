// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_runtime.h"

#include <array>
#include <cstdlib>
#include <deque>
#include <functional>
#include <tuple>
#include <utility>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "base/i18n/icu_util.h"
#include "base/location.h"
#include "base/memory/discardable_memory_allocator.h"
#include "base/memory_coordinator/memory_consumer.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_pump.h"
#include "base/message_loop/message_pump_type.h"
#include "base/observer_list.h"
#include "base/path_service.h"
#include "base/synchronization/lock.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "build/build_config.h"
#include "components/discardable_memory/service/discardable_shared_memory_manager.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "partition_alloc/buildflags.h"
#include "partition_alloc/memory_reclaimer.h"
#include "partition_alloc/partition_alloc_config.h"
#include "partition_alloc/partition_alloc_constants.h"
#include "partition_alloc/shim/allocator_shim.h"
#include "partition_alloc/shim/allocator_shim_default_dispatch_to_partition_alloc.h"
#include "partition_alloc/tagging.h"
#include "partition_alloc/thread_cache.h"
#include "shot/shot_platform.h"
#include "shot/shot_renderer.h"
#include "third_party/blink/public/platform/web_runtime_features.h"
#include "skia/ext/legacy_display_globals.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/renderer/platform/heap/thread_state.h"
#include "third_party/skia/include/core/SkExecutor.h"
#include "third_party/skia/include/core/SkGraphics.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"

#if BUILDFLAG(IS_WIN)
#include <dwrite.h>
#include <wrl/client.h>

#include "skia/ext/font_utils.h"
#include "third_party/skia/include/core/SkFontMgr.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/win/web_font_rendering.h"
#include "third_party/skia/include/ports/SkTypeface_win.h"
#include "ui/gfx/font.h"
#include "ui/gfx/system_fonts_win.h"
#endif

namespace shot {

namespace {

// Sends Skia's independent scanlines to Shot's existing worker pool. The
// rendering thread borrows from the same queue while it waits, so the pool's
// three-worker cap gives a large blur four-way parallelism without creating a
// second set of threads in every CLI process or Node worker.
class ShotSkiaExecutor final : public SkExecutor {
 public:
  ShotSkiaExecutor() : state_(base::MakeRefCounted<State>()) {}
  ~ShotSkiaExecutor() override = default;

  void add(std::function<void()> work) override {
    state_->Add(std::move(work));
    base::ThreadPool::PostTask(
        FROM_HERE, {base::TaskPriority::USER_BLOCKING},
        base::BindOnce(
            [](scoped_refptr<State> state) { state->RunOne(); }, state_));
  }

  void add(std::function<void()> work, int /*work_list*/) override {
    add(std::move(work));
  }

  void borrow() override { state_->RunOne(); }

 private:
  class State : public base::RefCountedThreadSafe<State> {
   public:
    void Add(std::function<void()> work) {
      base::AutoLock lock(lock_);
      work_.push_back(std::move(work));
    }

    void RunOne() {
      std::function<void()> work;
      {
        base::AutoLock lock(lock_);
        if (work_.empty()) {
          return;
        }
        work = std::move(work_.back());
        work_.pop_back();
      }
      work();
    }

   private:
    friend class base::RefCountedThreadSafe<State>;
    ~State() = default;

    base::Lock lock_;
    std::deque<std::function<void()>> work_;
  };

  scoped_refptr<State> state_;
};

}  // namespace

class ShotRuntime::MemoryConsumerRegistry : public base::MemoryConsumerRegistry {
 public:
  MemoryConsumerRegistry() = default;
  ~MemoryConsumerRegistry() override { NotifyDestruction(); }

  // Every consumer that has registered, told to drop what it is holding above
  // its limit. This is the other half of the contract: a coordinator would
  // decide when, and in a process with no coordinator ShotRuntime decides --
  // when the request queue has been empty long enough to say so.
  void ReleaseMemory() {
    for (base::MemoryConsumer& consumer : memory_consumers_) {
      NotifyReleaseMemory(&consumer);
    }
  }

 private:
  // base::MemoryConsumerRegistry:
  void OnMemoryConsumerAdded(uint32_t consumer_id,
                             std::string_view consumer_name,
                             base::MemoryConsumerTraits traits,
                             base::MemoryConsumer* consumer) override {
    memory_consumers_.AddObserver(consumer);
  }
  void OnMemoryConsumerRemoved(uint32_t consumer_id,
                               base::MemoryConsumer* consumer) override {
    memory_consumers_.RemoveObserver(consumer);
  }

  base::ObserverList<base::MemoryConsumer> memory_consumers_;
};

ShotRuntime::ShotRuntime() = default;

ShotRuntime::~ShotRuntime() {
  // SkExecutor does not take ownership of its default. Restore the process-wide
  // pointer before the member backing our replacement is destroyed.
  SkExecutor::SetDefault(previous_skia_executor_);

  // The allocator instance points at a member that is about to go away, and a
  // dangling one would be read by anything that outlives this object.
  base::DiscardableMemoryAllocator::SetInstance(nullptr);
}

// static
base::expected<std::unique_ptr<ShotRuntime>, std::string> ShotRuntime::Create(
    const NetworkConfig& network_config) {
  // Can't use make_unique: the constructor is private.
  std::unique_ptr<ShotRuntime> runtime(new ShotRuntime());

  // PartitionAlloc's per-thread cache. The allocator shim builds its
  // partition with the cache off and leaves turning it on to the embedder:
  // chrome does it in PartitionAllocSupport once its feature list is up, and
  // for a renderer raises the largest cached size to 32 KB. Nothing did it
  // here, so every allocation past the smallest buckets took the partition's
  // one lock. On one thread that is a slower malloc; on the raster threads it
  // is contention. On a page of blurred shadows and dashed borders -- each a
  // path rastered into a mask, each mask tens of KB allocated and freed --
  // three strips on three threads took 11.5 ms against 10.6 ms for the page
  // rastered whole on one, the heaviest strip 12 ms; with the cache on, the
  // three took 9.2 ms and that strip 9.5. Simple pages gain a third.
  //
  // The partition the shim starts with has no thread cache slot, so it is
  // replaced first, the way chrome's ReconfigureEarlyish and
  // ReconfigureAfterFeatureListInit do it between them: the reclaimer told
  // about the first partition, a fresh one configured with nothing chrome
  // would turn on by a feature -- no BackupRefPtr (this build has none), no
  // memory tagging, no quarantine -- and the cache enabled on that. What was
  // allocated before this stays where it is and is freed through the
  // partition it came from.
  //
  // Once per process, not per runtime: an engine is torn down and created
  // again by a restart, and the partition CHECKs a second enabling. The
  // caches are created lazily, on a thread's first allocation after this, so
  // it covers the raster threads whenever they are started.
  static const bool thread_cache_enabled = [] {
#if PA_BUILDFLAG(USE_PARTITION_ALLOC_AS_MALLOC) && \
    PA_CONFIG(THREAD_CACHE_SUPPORTED)
    allocator_shim::EnablePartitionAllocMemoryReclaimer();
    allocator_shim::ConfigurePartitions(
        allocator_shim::EnableBrp(false), /*brp_extra_extras_size=*/0,
        allocator_shim::EnableMemoryTagging(false),
        ::partition_alloc::TagViolationReportingMode::kUndefined,
        allocator_shim::BucketDistribution::kNeutral,
        ::partition_alloc::internal::SchedulerLoopQuarantineConfig(),
        ::partition_alloc::internal::SchedulerLoopQuarantineConfig(),
        ::partition_alloc::internal::SchedulerLoopQuarantineConfig(),
        allocator_shim::EventuallyZeroFreedMemory(false),
        allocator_shim::EnableFreeWithSize(false),
        allocator_shim::EnableStrictFreeSizeCheck(false));
    for (size_t token = 0; token < allocator_shim::kNumPartitions; ++token) {
      allocator_shim::internal::PartitionAllocMalloc::Allocator(
          allocator_shim::AllocToken(token))
          ->EnableThreadCacheIfSupported();
    }
    ::partition_alloc::ThreadCache::SetLargestCachedSize(
        ::partition_alloc::kThreadCacheLargeSizeThreshold);
    return true;
#else
    return false;
#endif
  }();
  std::ignore = thread_cache_enabled;

  // ICU first: WTF's string and text code assumes it during static
  // initialisation of blink.
  if (!base::i18n::InitializeICU()) {
    return base::unexpected("could not initialize ICU");
  }

  // The packed resources hold blink's user-agent stylesheet, which
  // ShotPlatform::GetDataResourceString hands back.
  ui::RegisterPathProvider();
  base::FilePath module_dir;
  if (!base::PathService::Get(base::DIR_MODULE, &module_dir)) {
    return base::unexpected("could not locate the module directory");
  }
  ui::ResourceBundle::InitSharedInstanceWithPakPath(
      module_dir.AppendASCII("shotium_strings.pak"));
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
      module_dir.AppendASCII("shotium_data.pak"), ui::kScaleFactorNone);

  // The thread pool. base::ThreadPool::PostTask DCHECKs on an instance, and the
  // first caller is DiscardableSharedMemoryManager, which does its accounting
  // and purging on a sequenced task runner rather than on the thread that
  // allocates. Blink's own workers -- image decoding, font loading, raster --
  // come from here too.
  //
  // Not CreateAndStartWithDefaultParams, which is what every other chromium
  // process calls: it sizes the pool at `cores - 1`, on the stated assumption
  // that the process should use the whole machine. A shot worker is the one
  // case where that assumption is wrong in both directions. It renders one
  // document at a time on this thread, so it never has `cores - 1` unblocked
  // tasks to run; and it is one of N sibling processes, so a pool that helps
  // itself to the machine is N pools all doing it at once.
  //
  // kMaxUnblockedTasks is upstream's own floor -- max(3, cores - 1) -- which
  // is the number a two-core machine gets today, and is enough for the widest
  // thing a capture actually fans out: decoding the images on one page. Warm
  // per-shot time does not move.
  //
  // What it does not do is make the process smaller, which is what it was
  // first tried for. A worker holds 32 threads on a 32-core host either way:
  // capping this took ThreadPoolForegroundWorker from as many as 31 down to
  // three, and 25 of the remaining threads turn out to belong to Windows
  // rather than to anything chromium started. The reason to cap it is
  // oversubscription, not bytes.
  constexpr size_t kMaxUnblockedTasks = 3;
  base::ThreadPoolInstance::Create("Shot");
  base::ThreadPoolInstance::Get()->Start({kMaxUnblockedTasks});
  runtime->shutdown_thread_pool_ = base::ScopedClosureRunner(
      base::BindOnce([] { base::ThreadPoolInstance::Get()->Shutdown(); }));

  // CPU image-filter passes are independent by scanline, but Skia's default
  // executor deliberately runs every task inline.
  runtime->previous_skia_executor_ = &SkExecutor::GetDefault();
  runtime->skia_executor_ = std::make_unique<ShotSkiaExecutor>();
  SkExecutor::SetDefault(runtime->skia_executor_.get());

  // Mojo, before blink. Nothing here talks to another process, but blink's
  // object graph is wired with mojo types regardless: LocalFrame::Init() builds
  // an empty PolicyContainer, and that means an AssociatedRemote, and that
  // means a message pipe. The pipe stays inside this process with nothing on
  // the far end, which is exactly what an empty policy container is for.
  mojo::core::Init();

  // blink's own scheduler, not a bare task executor.
  //
  // The simple path -- CreateMainThreadAndInitialize() -- installs
  // SimpleMainThreadScheduler, whose CreateAgentGroupScheduler() returns
  // nullptr outright. Page holds the result in a Member<AgentGroupScheduler>
  // and hands it to VisualViewport, which asks it for a compositor task runner
  // before the Page constructor has finished, so a screenshot could not get
  // past creating its page.
  //
  // This is the path a renderer takes: a real MainThreadSchedulerImpl with a
  // SequenceManager built on this thread's message pump, which produces a real
  // AgentGroupSchedulerImpl with real task queues.
  //
  // Order is fixed by platform.h: InitializeBlink() -- WTF, Partitions and the
  // cppgc heap -- must happen before the scheduler exists.
  //
  // The pump is an IO one rather than the DEFAULT a renderer uses, because this
  // thread also owns the network stack: //net watches sockets through
  // base::CurrentIOThread, which only exists when the thread's pump is a
  // MessagePumpForIO. Nothing in blink depends on the pump type -- the
  // scheduler builds its queues on whatever it is handed -- so an IO pump is a
  // strict superset of what a renderer's main thread runs on.
  blink::Platform::InitializeBlink();
  runtime->main_thread_scheduler_ =
      blink::scheduler::WebThreadScheduler::CreateMainThreadScheduler(
          base::MessagePump::Create(base::MessagePumpType::IO));

  // Discardable memory, which skia's raster step needs before it needs anything
  // else about the page: SkBlurMaskFilterImpl caches the nine-patch it builds
  // for a blurred rounded rectangle -- a CSS box-shadow -- in SkResourceCache,
  // and that cache allocates through base::DiscardableMemoryAllocator. With no
  // instance set, the first shadow in the corpus took the process down.
  //
  // DiscardableSharedMemoryManager is the real implementation, not a stand-in:
  // its own comment says it allocates and manages segments "for the process
  // which hosts this class", and the IPC half -- Bind(), which serves remote
  // processes -- is what this build does not use.
  runtime->memory_consumer_registry_ = std::make_unique<
      base::ScopedMemoryConsumerRegistry<MemoryConsumerRegistry>>();
  runtime->discardable_manager_ =
      std::make_unique<discardable_memory::DiscardableSharedMemoryManager>();
  base::DiscardableMemoryAllocator::SetInstance(
      runtime->discardable_manager_.get());

  // ~MainThreadSchedulerImpl CHECKs that Shutdown() was called, so that it
  // cannot outlive the blink heap holding stale pointers into it.
  runtime->shutdown_scheduler_ = base::ScopedClosureRunner(
      base::BindOnce(&blink::scheduler::WebThreadScheduler::Shutdown,
                     base::Unretained(runtime->main_thread_scheduler_.get())));

  // Text rendering, from the host's own font settings.
  //
  // blink's FontCache starts with antialiased_text_enabled_ = false and
  // lcd_text_enabled_ = false, so FontPlatformData::CreateSkFont() picked
  // SkFont::Edging::kAlias and every glyph came out with hard, unantialiased
  // edges. A browser fills these from gfx::GetFontRenderParams(), the host's
  // ClearType configuration -- and the first cut of this code did the same.
  //
  // A screenshot must not do that, for two reasons this code has now paid for:
  //
  //  * Subpixel (LCD) antialiasing encodes the physical RGB stripe layout of
  //    the monitor the page was rendered for into the image. A screenshot is
  //    exactly the case where the pixels will be shown on some other display.
  //
  //  * The host's answer is not even stable on the host. Rendering the same
  //    page from ten fresh processes produced nine identical images and one
  //    outlier -- always the first -- differing only in glyph-edge pixels
  //    whose channels sit on Skia's LCD gamma table ({0, 39, 87, 143, ...})
  //    and move independently per channel: the first process rasterised with
  //    different ClearType parameters than every later one, which is what the
  //    four "two renders of the same page differ" demo failures were.
  //
  // So: grayscale antialiasing, fixed gamma, no pixel geometry, everywhere.
  skia::LegacyDisplayGlobals::SetCachedParams(
      kUnknown_SkPixelGeometry, SK_GAMMA_CONTRAST, SK_GAMMA_EXPONENT);
#if BUILDFLAG(IS_WIN)
  blink::WebFontRendering::SetAntialiasedTextEnabled(true);
  blink::WebFontRendering::SetLCDTextEnabled(false);

  // Grayscale alone shrank the differences but did not close them, and the
  // remaining state turned out to live outside the process entirely. Measured
  // on one machine: the first process ever to rasterise a given (font, size,
  // rendering mode) tuple produces slightly different glyph edges than every
  // process after it; the warm state survives across processes and batches,
  // and a byte-identical copy of the executable stays warm, so it is keyed by
  // what was drawn, not by who drew it. That is the Windows font cache
  // service: a shared DirectWrite factory consults it, and a render that runs
  // while an entry is still being built takes the in-process path, whose
  // rasterisation differs by a hair. CI runners are always that first
  // machine.
  //
  // An ISOLATED factory never talks to the service, so every process --
  // including the first on a freshly booted machine -- rasterises the same
  // way. The cost is losing the cross-process font list cache, paid once per
  // process start, and shot's serve mode starts one process.
  {
    Microsoft::WRL::ComPtr<IUnknown> dwrite_unknown;
    Microsoft::WRL::ComPtr<IDWriteFactory> dwrite_factory;
    CHECK(SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED,
                                        __uuidof(IDWriteFactory),
                                        &dwrite_unknown)));
    CHECK(SUCCEEDED(dwrite_unknown.As(&dwrite_factory)));
    skia::OverrideDefaultSkFontMgr(
        SkFontMgr_New_DirectWrite(dwrite_factory.Get()));
  }

  // The Windows shell fonts, which blink cannot ask for itself.
  //
  // FontCache::SystemFontFamily() -- what `font-family: system-ui` resolves to,
  // and what the -webkit-control/-webkit-menu keywords use -- returns
  // MenuFontFamily(), and MenuFontFamily() dereferences a static pointer that
  // is null until SetMenuFontMetrics has been called. In Chrome the browser
  // process reads NONCLIENTMETRICS, puts it in RendererPreferences and the
  // renderer calls these three from
  // WebViewImpl::UpdateFontRenderingFromRendererPrefs. shot has no browser
  // process and no WebView, so it reads the same NONCLIENTMETRICS through
  // gfx::win::GetSystemFont and tells blink directly.
  //
  // Before this, every document using `font-family: system-ui` crashed in
  // FontCache::GetFontPlatformData on the null AtomicString. Nothing in the
  // corpus used it, so nothing had ever asked; it is one of the commonest
  // families on the real web, which is how bringing up HTTP found it.
  const auto set_system_font = [](gfx::win::SystemFont which,
                                  void (*setter)(const blink::WebString&,
                                                 int32_t)) {
    const gfx::Font& font = gfx::win::GetSystemFont(which);
    setter(blink::WebString::FromUtf8(font.GetFontName()),
           static_cast<int32_t>(font.GetFontSize()));
  };
  set_system_font(gfx::win::SystemFont::kMenu,
                  &blink::WebFontRendering::SetMenuFontMetrics);
  set_system_font(gfx::win::SystemFont::kSmallCaption,
                  &blink::WebFontRendering::SetSmallCaptionFontMetrics);
  set_system_font(gfx::win::SystemFont::kStatus,
                  &blink::WebFontRendering::SetStatusFontMetrics);
#endif

  // With RasterInducingScroll on, blink's paint conversion wraps each
  // scroller's contents in a DrawScrollingContentsOp whose playback CHECKs
  // for a table of live scroll offsets -- the compositor's, which this
  // process does not have. Off, a scroller's contents are emitted in place at
  // the offset it was painted with, which is what a screenshot wants anyway.
  blink::WebRuntimeFeatures::EnableFeatureFromString("RasterInducingScroll",
                                                     false);

  runtime->platform_ = std::make_unique<ShotPlatform>();
  mojo::BinderMap binders;
  blink::Initialize(runtime->platform_.get(), &binders,
                    runtime->main_thread_scheduler_.get());

  // Last, because it is the only step that needs the thread to be finished:
  // URLRequestContextBuilder reads base::CurrentIOThread and posts to the
  // thread pool while it builds, and the disk cache backend starts opening its
  // index the moment it exists.
  auto network = ShotNetwork::Create(network_config);
  if (!network.has_value()) {
    return base::unexpected(network.error());
  }
  runtime->network_ = std::move(network).value();
  runtime->renderer_ = std::make_unique<ShotRenderer>();

  return runtime;
}

ShotRenderer& ShotRuntime::renderer() {
  return *renderer_;
}

void ShotRuntime::PurgeMemory() {
  // SHOT_PROFILE=1 logs how long each stage below took, the same switch the
  // renderer's profile lines answer to.
  static const bool profile = [] {
    const char* value = std::getenv("SHOT_PROFILE");
    return value && *value && *value != '0';
  }();
  base::TimeTicks stage_started = base::TimeTicks::Now();
  std::array<double, 5> stage_ms = {};
  auto stage_done = [&](size_t index) {
    const base::TimeTicks now = base::TimeTicks::Now();
    stage_ms.at(index) = (now - stage_started).InMillisecondsF();
    stage_started = now;
  };
  // The page a small capture left for the idle turn, and the bitmap kept for
  // the next one: an explicit release is the one time neither should be kept.
  // The page has to go before the collection below, or it is not garbage yet.
  renderer_->ReleaseRetained();
  stage_done(0);
  // Blink's heap first, and everything else after, because the collection is
  // what makes the rest of it worth doing: the Page, the Document, every
  // LayoutObject and every Resource the capture built are unreachable the
  // moment Capture() returns, but cppgc frees nothing until someone asks. The
  // caches below hold entries keyed by objects that are still alive until it
  // does, so purging in the other order leaves the largest part behind.
  blink::ThreadState::Current()->CollectAllGarbageForMemoryPressure();
  stage_done(1);

  // The caches that survive a collection because they are deliberate: blink's
  // resource cache, the font cache, the shaping cache, the parkable strings,
  // and the discardable segments skia rasterises through. They register as
  // memory consumers precisely so that something can tell them to shrink.
  memory_consumer_registry_->Get().ReleaseMemory();
  stage_done(2);

  // Skia's own two, which are not memory consumers: the glyph raster cache and
  // SkResourceCache's non-discardable half.
  SkGraphics::PurgeAllCaches();
  stage_done(3);

  // And the free lists underneath all of it. Everything above returns memory
  // to PartitionAlloc, which keeps it: the pages stay committed and charged to
  // this process against the next allocation of that size. Reclaiming is what
  // turns a purge into a smaller process. ReclaimAll is the aggressive kind:
  // it empties the per-thread allocator caches first -- this thread's now, the
  // others' on their next allocation -- which chrome does on a timer and a
  // worker between requests, with no timer running, does here.
  //
  // Blink starts a periodic reclaimer of its own (Platform::Initialize, via an
  // idle task) and it has never run here, because a worker between requests is
  // not idle in the scheduler's sense -- it is blocked on the request stream,
  // with no idle period for the task to be scheduled in.
  ::partition_alloc::MemoryReclaimer::Instance()->ReclaimAll();
  stage_done(4);
  if (profile) {
    LOG(INFO) << "shot: profile purge release=" << stage_ms[0]
              << " gc=" << stage_ms[1] << " consumers=" << stage_ms[2]
              << " skia=" << stage_ms[3] << " reclaim=" << stage_ms[4];
  }
  LogMemoryStage("purged");
}

void ShotRuntime::ReleaseWorkingSet() {
#if BUILDFLAG(IS_WIN)
  // Both sizes (SIZE_T)-1 is the documented way to say "trim the working set
  // now": the pages go to the standby list, where they cost this process
  // nothing and are still in RAM, so the next request faults them back
  // without touching the disk. That is why this is cheap enough to be worth
  // doing at all, and why it is still not free -- 30 MB is 7,500 soft faults.
  ::SetProcessWorkingSetSizeEx(::GetCurrentProcess(),
                               static_cast<SIZE_T>(-1),
                               static_cast<SIZE_T>(-1), 0);
#endif
}

}  // namespace shot
