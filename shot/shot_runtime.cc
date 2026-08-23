// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_runtime.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/i18n/icu_util.h"
#include "base/memory/discardable_memory_allocator.h"
#include "base/memory_coordinator/memory_consumer.h"
#include "base/message_loop/message_pump.h"
#include "base/message_loop/message_pump_type.h"
#include "base/observer_list.h"
#include "base/path_service.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "build/build_config.h"
#include "components/discardable_memory/service/discardable_shared_memory_manager.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "shot/shot_platform.h"
#include "skia/ext/legacy_display_globals.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"
#include "third_party/blink/public/web/blink.h"
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

class ShotRuntime::MemoryConsumerRegistry : public base::MemoryConsumerRegistry {
 public:
  MemoryConsumerRegistry() = default;
  ~MemoryConsumerRegistry() override { NotifyDestruction(); }

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
  // The allocator instance points at a member that is about to go away, and a
  // dangling one would be read by anything that outlives this object.
  base::DiscardableMemoryAllocator::SetInstance(nullptr);
}

// static
base::expected<std::unique_ptr<ShotRuntime>, std::string> ShotRuntime::Create(
    const NetworkConfig& network_config) {
  // Can't use make_unique: the constructor is private.
  std::unique_ptr<ShotRuntime> runtime(new ShotRuntime());

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
      module_dir.AppendASCII("shot_strings.pak"));
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
      module_dir.AppendASCII("shot_data.pak"), ui::kScaleFactorNone);

  // The thread pool. base::ThreadPool::PostTask DCHECKs on an instance, and the
  // first caller is DiscardableSharedMemoryManager, which does its accounting
  // and purging on a sequenced task runner rather than on the thread that
  // allocates. Blink's own workers -- image decoding, font loading, raster --
  // come from here too.
  //
  // CreateAndStartWithDefaultParams is what every chromium process calls; the
  // pool sizes itself to the machine.
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("Shot");
  runtime->shutdown_thread_pool_ = base::ScopedClosureRunner(
      base::BindOnce([] { base::ThreadPoolInstance::Get()->Shutdown(); }));

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

  return runtime;
}

}  // namespace shot
