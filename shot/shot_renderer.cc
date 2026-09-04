// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/memory/discardable_memory_allocator.h"
#include "base/memory/raw_ref.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "base/containers/flat_set.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/strings/string_util.h"
#include "cc/paint/discardable_image_map.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/draw_image.h"
#include "cc/paint/paint_op.h"
#include "cc/paint/paint_op_buffer.h"
#include "cc/paint/paint_op_buffer_iterator.h"
#include "cc/trees/scroll_source_type.h"
#include "shot/shot_capture_context.h"
#include "shot/shot_image_stream.h"
#include "shot/shot_network.h"
#include "shot/shot_url_loader.h"
#include "skia/ext/legacy_display_globals.h"
#include "third_party/blink/public/common/tokens/tokens.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/mojom/frame/frame.mojom-blink.h"
#include "third_party/blink/public/mojom/scroll/scroll_enums.mojom-blink.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/frame/frame_types.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/layout/layout_box_model_object.h"
#include "third_party/blink/renderer/core/layout/map_coordinates_flags.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/loader/empty_clients.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/paint/object_paint_properties.h"
#include "third_party/blink/renderer/core/paint/paint_layer_scrollable_area.h"
#include "third_party/blink/renderer/core/scroll/scroll_types.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/fonts/generic_font_family_settings.h"
#include "third_party/blink/renderer/platform/graphics/color.h"
#include "third_party/blink/renderer/platform/graphics/compositing/paint_chunks_to_cc_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_artifact.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_chunk_subset.h"
#include "third_party/blink/renderer/platform/graphics/paint/property_tree_state.h"
#include "third_party/blink/renderer/platform/graphics/paint/transform_paint_property_node.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_vector.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/thread_state.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/scheduler/public/agent_group_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/public/main_thread.h"
#include "third_party/blink/renderer/platform/scheduler/public/main_thread_scheduler.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "partition_alloc/memory_reclaimer.h"
#include "third_party/blink/renderer/platform/graphics/image_decoding_store.h"
#include "third_party/blink/renderer/platform/disk_data_allocator.h"
#include "third_party/blink/renderer/platform/graphics/parkable_image_manager.h"
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"
#include "third_party/blink/renderer/platform/wtf/shared_buffer.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/icu/source/common/unicode/uscript.h"
#include "third_party/skia/include/core/SkGraphics.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkSurfaceProps.h"
#include "ui/display/screen_info.h"
#include "ui/display/screen_infos.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/vector2d_f.h"
#include "v8/include/cppgc/heap-consistency.h"
#include "v8/include/cppgc/heap-statistics.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>

#include <psapi.h>
#endif

namespace shot {

// The document is delivered to the frame directly, so nothing here ever asks
// the embedder to open a window, run a modal dialog or start a navigation.
// EmptyChromeClient already answers all of that with "no"; this subclass exists
// so the type has a name in stack traces, so shot-specific answers have an
// obvious home, and so the page can be told what kind of screen it is being
// rendered for.
class ShotRenderer::ChromeClient final : public blink::EmptyChromeClient {
 public:
  // The device scale factor the document sees. It reaches CSS through
  // LocalFrame::DevicePixelRatio -- which is what resolution media queries and
  // srcset/image-set selection are decided on -- so a request for scale 2 picks
  // the 2x asset rather than upscaling the 1x one.
  //
  // Only the scale factor is set. Everything else in ScreenInfo stays at its
  // default, in particular the screen rectangle: filling that in would change
  // what device-width media queries answer, and this pipeline has no screen to
  // report the size of.
  void SetDeviceScaleFactor(float scale) {
    display::ScreenInfo info;
    info.device_scale_factor = scale;
    screen_infos_ = display::ScreenInfos(info);
  }

  const display::ScreenInfo& GetScreenInfo(blink::LocalFrame&) const override {
    return screen_infos_.current();
  }
  const display::ScreenInfos& GetScreenInfos(
      blink::LocalFrame&) const override {
    return screen_infos_;
  }
  const display::ScreenInfo& GetOriginalScreenInfo(
      blink::LocalFrame&) const override {
    return screen_infos_.current();
  }

  // EmptyChromeClient swallows console messages, which makes a page that
  // renders wrong impossible to diagnose from the outside: when blink refuses a
  // subresource it says so here and nowhere else. A screenshot tool that hides
  // "Not allowed to load local resource" is a screenshot tool that silently
  // produces the wrong picture, so these go to stderr, where they do not
  // disturb the image on stdout-equivalent output paths.
  void AddMessageToConsole(blink::LocalFrame*,
                           blink::mojom::ConsoleMessageSource,
                           blink::mojom::ConsoleMessageLevel level,
                           const blink::String& message,
                           unsigned line_number,
                           const blink::String& source_id,
                           const blink::String& stack_trace) override {
    const char* level_name = "log";
    switch (level) {
      case blink::mojom::ConsoleMessageLevel::kVerbose:
        level_name = "verbose";
        break;
      case blink::mojom::ConsoleMessageLevel::kInfo:
        level_name = "info";
        break;
      case blink::mojom::ConsoleMessageLevel::kWarning:
        level_name = "warning";
        break;
      case blink::mojom::ConsoleMessageLevel::kError:
        level_name = "error";
        break;
    }
    LOG(ERROR) << "shot: console " << level_name << ": " << message.Utf8();
  }

 private:
  display::ScreenInfos screen_infos_{display::ScreenInfo()};
};

// Likewise for the frame client, except for the one thing an
// EmptyLocalFrameClient genuinely cannot do: hand ResourceFetcher a URLLoader
// for subresources. EmptyLocalFrameClient's own comment names
// CreateURLLoaderForTesting() as the hook for exactly this ("the consumer
// should define their own subclass ... and override the
// CreateURLLoaderForTesting method"). The name says testing because upstream's
// only non-//content consumers are tests; shot is the other kind.
class ShotRenderer::FrameClient final : public blink::EmptyLocalFrameClient {
 public:
  std::unique_ptr<blink::URLLoader> CreateURLLoaderForTesting() override {
    return std::make_unique<ShotURLLoader>();
  }

  // What the document reports as navigator.userAgent, and what //net puts on
  // the wire. Asking the two to agree matters: a server that serves different
  // markup to different agents would otherwise be photographed serving one
  // thing while the document believes it is another.
  blink::String UserAgent() override {
    return blink::String::FromUtf8(ShotNetwork::UserAgent());
  }

  // The hook that actually fires for a document installed with
  // ForceSynchronousDocumentInstall, and so the one the wait loop below
  // depends on. Document::CheckCompleted() calls it right after dispatching
  // the load event.
  //
  // The two below never fire on this path, which is worth stating because
  // they look like they should. FrameLoader::Init marks the DocumentLoader it
  // reuses as having already sent DidFinishLoad, so
  // DispatchDidFinishLoad() is skipped; and DidStopLoading() is
  // ProgressTracker's, which no navigation ever started. They stay because
  // they are the correct answer if a document ever does arrive by navigation,
  // but until then a load that finished in under a millisecond was only
  // noticed by the wait loop's 10 ms watchdog -- which on Windows rounds up to
  // the 15.6 ms system tick, so every ordinary capture spent ~24 ms asleep.
  //
  // IsolatedSVGDocumentHost, the other in-process consumer of a synchronously
  // installed document, hooks the same event for the same reason.
  void DispatchDidHandleOnloadEvents() override {
    if (CaptureContext* capture = CaptureContext::Current()) {
      capture->NotifyProgress();
    }
  }

  void DispatchDidFinishLoad() override {
    if (CaptureContext* capture = CaptureContext::Current()) {
      capture->NotifyProgress();
    }
  }

  void DidStopLoading() override {
    if (CaptureContext* capture = CaptureContext::Current()) {
      capture->NotifyProgress();
    }
  }
};

namespace {

// How long a quiet network has to stay quiet before "networkidle" is satisfied.
// The number is puppeteer's, and the reason it is not zero is that a stylesheet
// that pulls in a font produces a gap between one request finishing and the
// next starting.
constexpr base::TimeDelta kNetworkIdleWindow = base::Milliseconds(500);

// How long to let the message loop run between checks. Short enough that the
// idle window is measured to within a few percent, long enough that a 30-second
// timeout is not thousands of lifecycle updates.
constexpr base::TimeDelta kPumpSlice = base::Milliseconds(10);

// How long a load may go without a layout while requests are still arriving.
// Short enough that a stylesheet's fonts are asked for promptly; long enough
// that a burst of arrivals is laid out once.
constexpr base::TimeDelta kLifecycleInterval = base::Milliseconds(50);

// Below this the heap is not worth collecting mid-load: the collection costs
// more than the memory it would return.
constexpr size_t kWaitCollectFloorBytes = 48u << 20;

// An upper bound on the viewport, so that a document whose height grows every
// time the viewport does cannot ask for an unallocatable surface.
constexpr int kMaximumDimension = 32767;

// How far from the origin blink paints. Measured rather than read off a
// header: a box whose *painted* position reaches 32767 CSS pixels, on either
// axis, has nothing painted from that coordinate on -- the record just stops,
// with no error. Layout is not the limit (an element laid out at 50,000 and
// transformed up to 25,000 paints; one laid out at 1,000 and transformed down
// to 51,000 does not), so content further down is reached by scrolling the
// document, which moves what it paints back under the origin. That is what
// the windows in RenderDocument do.
constexpr int kPaintedExtent = 32767;
// The most document rows one paint record is asked to cover. Short of the
// extent so that a box straddling a band's bottom edge keeps its outsets --
// shadows, anti-aliasing -- inside the band that draws it.
constexpr int kPaintWindow = 32000;

// What one encoded image may measure, per format. JPEG's is the format's own.
// WebP's is libwebp's hard ceiling, which the encoder reports only as "failed".
// PNG's is chosen to match JPEG rather than the format's 2^31, because past
// this a tiles request is the right call, and a caller who did not make one is
// better told so than handed a file most viewers refuse to open.
constexpr int kMaximumPngDimension = 65535;
constexpr int kMaximumJpegDimension = 65535;
constexpr int kMaximumWebpDimension = 16383;
// The largest bitmap one image is rastered into. A gigabyte is, for a
// 1440-wide capture at scale 1, a document about 180,000 CSS pixels tall.
constexpr int64_t kMaximumSurfaceBytes = int64_t{1} << 30;

using FontFamilyUpdater =
    bool (blink::GenericFontFamilySettings::*)(const blink::AtomicString&,
                                               UScriptCode);

// Runs the message loop until a resource/frame event reports progress, with a
// short timer as a watchdog for Blink state changes that have no embedder
// callback.
//
// RunUntilIdle would not do: the network stack completes on this thread from
// timers and IO completions, and "no task queued right now" is not "nothing is
// coming". Letting real time pass is also what makes the networkidle window
// measurable at all.
void PumpFor(base::TimeDelta slice) {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  CaptureContext* capture = CaptureContext::Current();
  if (capture) {
    capture->SetProgressCallback(run_loop.QuitClosure());
  }
  base::ScopedClosureRunner clear_progress(base::BindOnce(
      [](CaptureContext* capture) {
        if (capture) {
          capture->SetProgressCallback(base::RepeatingClosure());
        }
      },
      base::Unretained(capture)));
  base::OneShotTimer watchdog;
  watchdog.Start(FROM_HERE, slice, run_loop.QuitClosure());
  run_loop.Run();
}

// One entry of a WebPreferences font map, onto Settings. The map is keyed by
// ISO 15924 script code ("Zyyy" for Common), which is what
// u_getPropertyValueEnum turns back into a UScriptCode. This is
// ApplyFontsFromMap() from web_view_impl.cc, minus the WebSettings indirection
// that shot does not have a WebView to go through.
void ApplyFontMap(const blink::web_pref::ScriptFontFamilyMap& map,
                  FontFamilyUpdater updater,
                  blink::Settings& settings) {
  bool changed = false;
  for (const auto& entry : map) {
    const int32_t script =
        u_getPropertyValueEnum(UCHAR_SCRIPT, entry.first.c_str());
    if (script < 0 || script >= USCRIPT_CODE_LIMIT) {
      continue;
    }
    UScriptCode code = static_cast<UScriptCode>(script);
    // web_view_impl.cc's GetScriptForWebSettings(): blink indexes Japanese and
    // Korean fonts by the script codes it has always used.
    switch (code) {
      case USCRIPT_HIRAGANA:
      case USCRIPT_KATAKANA:
      case USCRIPT_JAPANESE:
        code = USCRIPT_KATAKANA_OR_HIRAGANA;
        break;
      case USCRIPT_KOREAN:
        code = USCRIPT_HANGUL;
        break;
      default:
        break;
    }
    changed |= (settings.GetGenericFontFamilySettings().*updater)(
        blink::AtomicString(blink::String(base::span(entry.second))), code);
  }
  if (changed) {
    settings.NotifyGenericFontFamilyChange();
  }
}

// Chrome's rendering defaults, applied to this Page.
//
// The "initial:" values in core/frame/settings.json5 are blink's own, and they
// are not what a browser renders with -- a browser overwrites them from
// WebPreferences when it commits a document, and blink's initial value is
// whatever is safe for an embedder that has not said anything yet. The gap is
// not cosmetic: loads_images_automatically is *false* by default in blink, so
// before this ran, no <img> in any document ever issued a request. There was no
// error to see, because nothing had been attempted.
//
// The oracle these renderings are compared against is Chrome, so the values
// come from a default-constructed web_pref::WebPreferences -- Chrome's own
// struct, in this tree -- rather than from numbers written out here.
void ApplyChromeWebPreferences(blink::Settings& settings) {
  const blink::web_pref::WebPreferences prefs;

  ApplyFontMap(prefs.standard_font_family_map,
               &blink::GenericFontFamilySettings::UpdateStandard, settings);
  ApplyFontMap(prefs.fixed_font_family_map,
               &blink::GenericFontFamilySettings::UpdateFixed, settings);
  ApplyFontMap(prefs.serif_font_family_map,
               &blink::GenericFontFamilySettings::UpdateSerif, settings);
  ApplyFontMap(prefs.sans_serif_font_family_map,
               &blink::GenericFontFamilySettings::UpdateSansSerif, settings);
  ApplyFontMap(prefs.cursive_font_family_map,
               &blink::GenericFontFamilySettings::UpdateCursive, settings);
  ApplyFontMap(prefs.fantasy_font_family_map,
               &blink::GenericFontFamilySettings::UpdateFantasy, settings);
  ApplyFontMap(prefs.math_font_family_map,
               &blink::GenericFontFamilySettings::UpdateMath, settings);

  settings.SetDefaultFontSize(prefs.default_font_size);
  settings.SetDefaultFixedFontSize(prefs.default_fixed_font_size);
  settings.SetMinimumFontSize(prefs.minimum_font_size);
  settings.SetMinimumLogicalFontSize(prefs.minimum_logical_font_size);
  settings.SetDefaultTextEncodingName(
      blink::AtomicString(blink::String::FromUtf8(prefs.default_encoding)));

  settings.SetLoadsImagesAutomatically(prefs.loads_images_automatically);
  settings.SetImagesEnabled(prefs.images_enabled);
  settings.SetDownloadableBinaryFontsEnabled(prefs.remote_fonts_enabled);
  settings.SetShouldPrintBackgrounds(prefs.should_print_backgrounds);
  settings.SetTextAreasAreResizable(prefs.text_areas_are_resizable);
  settings.SetWebSecurityEnabled(prefs.web_security_enabled);
  settings.SetHyperlinkAuditingEnabled(prefs.hyperlink_auditing_enabled);
  settings.SetTextTrackMarginPercentage(prefs.text_track_margin_percentage);
}

int EnvInt(const char* name, int fallback) {
  const char* value = std::getenv(name);
  if (!value || !*value) {
    return fallback;
  }
  int parsed = 0;
  return base::StringToInt(value, &parsed) ? parsed : fallback;
}

// Logs the lifecycle split (style / layout / prepaint+paint) and the heap size
// per round. Needs --verbose too, which is what lets LOG(INFO) through.
bool ProfileEnabled() {
  static const bool enabled = EnvInt("SHOT_PROFILE", 0) != 0;
  return enabled;
}

// Whether a freed PartitionAlloc span is decommitted rather than left
// committed against the next allocation of its size, from SHOT_RECLAIM. On by
// default: without it the memory the capture gives back is invisible to the
// operating system until an idle purge that a one-shot process never reaches.
bool ReclaimEnabled() {
  static const bool enabled = EnvInt("SHOT_RECLAIM", 1) != 0;
  return enabled;
}

}  // namespace

// Opens the scratch file blink parks image bytes into, once per process and
// only when there is an image to park. A browser's renderer is handed this
// file at startup; doing the same here charged every capture for a file that
// a page without images never uses, and left a temporary file behind for each
// one. Made on the first round of the first load that has an image, which is
// milliseconds before the first byte could be written to it.
void ProvideParkingFile() {
  static bool tried = false;
  if (tried) {
    return;
  }
  tried = true;
  base::FilePath parking;
  if (!base::CreateTemporaryFile(&parking)) {
    return;
  }
  base::File file(parking, base::File::FLAG_CREATE_ALWAYS |
                               base::File::FLAG_READ | base::File::FLAG_WRITE |
                               base::File::FLAG_DELETE_ON_CLOSE);
  if (file.IsValid()) {
    blink::DiskDataAllocator::Instance().ProvideTemporaryFile(std::move(file));
  }
}

bool ParkImagesEnabled() {
  static const bool enabled = EnvInt("SHOT_PARK_IMAGES", 1) != 0;
  return enabled;
}

// Logs where the process's memory is at `stage`, in MB, when SHOT_PROFILE is
// set: the process totals the OS sees (working set, private commit and the
// peak working set so far) and the pools inside it that can be asked --
// PartitionAlloc's committed pages, the cppgc heap, skia's resource and glyph
// caches, and the discardable segments skia's image cache lives in.
void LogMemoryStage(const char* stage) {
  if (!ProfileEnabled()) {
    return;
  }
  size_t working_set = 0;
  size_t private_bytes = 0;
  size_t peak_working_set = 0;
  size_t peak_private = 0;
#if BUILDFLAG(IS_WIN)
  PROCESS_MEMORY_COUNTERS_EX counters = {};
  counters.cb = sizeof(counters);
  if (::GetProcessMemoryInfo(
          ::GetCurrentProcess(),
          reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
          sizeof(counters))) {
    working_set = counters.WorkingSetSize;
    private_bytes = counters.PrivateUsage;
    peak_working_set = counters.PeakWorkingSetSize;
    // The high-water mark of the commit charge: what a sampler outside the
    // process would have seen as peak private bytes, whichever stage it
    // happened in.
    peak_private = counters.PeakPagefileUsage;
  }
#endif
  size_t heap_used = 0;
  size_t heap_committed = 0;
  if (blink::ThreadState* thread_state = blink::ThreadState::Current()) {
    const cppgc::HeapStatistics heap = cppgc::CollectStatistics(
        thread_state->heap_handle(), cppgc::HeapStatistics::kBrief);
    heap_used = heap.used_size_bytes;
    heap_committed = heap.committed_size_bytes;
  }
  size_t discardable = 0;
  if (base::DiscardableMemoryAllocator* allocator =
          base::DiscardableMemoryAllocator::GetInstance()) {
    discardable = allocator->GetBytesAllocated();
  }
  const auto mb = [](size_t bytes) { return bytes / 1048576.0; };
  // `pa` is committed, not live; `pa_live` is what is actually allocated, so
  // the difference is what a reclaim would return. `images` counts the
  // encoded image sources still alive (ParkableImages), `decoders` the bytes
  // held by blink's cache of partially-used decoders.
  LOG(INFO) << base::StringPrintf(
      "shot: mem %-14s ws=%.1f private=%.1f peak_ws=%.1f peak_priv=%.1f "
      "pa=%.1f pa_live=%.1f cppgc=%.1f/%.1f skia=%.1f font=%.1f "
      "discardable=%.1f images=%zu decoders=%.1f",
      stage, mb(working_set), mb(private_bytes), mb(peak_working_set),
      mb(peak_private), mb(blink::Partitions::TotalSizeOfCommittedPages()),
      mb(blink::Partitions::TotalActiveBytes()), mb(heap_used),
      mb(heap_committed), mb(SkGraphics::GetResourceCacheTotalBytesUsed()),
      mb(SkGraphics::GetFontCacheUsed()), mb(discardable),
      blink::ParkableImageManager::Instance().Size(),
      mb(blink::ImageDecodingStore::Instance().MemoryUsageInBytes()));
}

namespace {

// The file a request's `path` names, opened for writing so the encoder can
// stream into it, or an invalid File when there is no path and the image is
// to be handed back in memory. `n` numbers a tile into the path's {n}.
base::expected<base::File, std::string> OpenOutput(
    const std::string& path_template,
    size_t n) {
  if (path_template.empty()) {
    return base::File();
  }
  std::string path = path_template;
  base::ReplaceSubstringsAfterOffset(&path, 0, "{n}", base::NumberToString(n));
  base::File file(base::FilePath::FromUTF8Unsafe(path),
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!file.IsValid()) {
    return base::unexpected("could not open " + path + " for writing");
  }
  return file;
}

// Where OpenOutput() put tile `n`, for the caller's report.
std::string OutputPath(const std::string& path_template, size_t n) {
  std::string path = path_template;
  base::ReplaceSubstringsAfterOffset(&path, 0, "{n}", base::NumberToString(n));
  return path;
}

// What the scroll offset carries with it, at the offset the document is at
// now.
//
// Two kinds of box are painted against the viewport rather than against the
// document: a position:fixed one always, and a position:sticky one for as
// long as it is stuck away from the place its flow put it. Painting the
// document at one scroll offset is a photograph of one viewport and both are
// right in it. Painting it at several -- which is how a document taller than
// blink can paint in one go is reached -- puts them in every one, which is
// how a fixed header ends up repeated down a long page.
//
// So the first window keeps them and the rest do not. The first window is the
// one whose viewport is the one the caller asked for, and it is the same
// answer a page short enough to need no scrolling already gives, since that
// one is photographed at offset zero and never moves.
class ViewportAnchored {
  // Holds pointers into the property trees, which are garbage collected: on
  // the stack they are found by the conservative scan, and anywhere else they
  // would need tracing. It is a local of one paint and nothing outlives that.
  STACK_ALLOCATED();

 public:
  // `window_top` is the first document row this window is being painted for.
  ViewportAnchored(blink::LocalFrameView* view, int window_top) {
    blink::LayoutView* layout_view = view->GetLayoutView();
    if (!layout_view) {
      return;
    }
    if (const auto* properties =
            layout_view->FirstFragment().PaintProperties()) {
      scroll_translation_ = properties->ScrollTranslation();
    }
    if (!scroll_translation_) {
      return;
    }
    // Which sticky boxes are stuck is a property of where the document is
    // scrolled to, so this is read after the scroll, once per window.
    for (blink::LayoutObject* object = layout_view; object;
         object = object->NextInPreOrder()) {
      const auto* box = blink::DynamicTo<blink::LayoutBoxModelObject>(object);
      if (!box || !box->HasStickyConstraints() ||
          box->StickyPositionOffset().IsZero()) {
        // Sitting where its flow put it, which makes it ordinary document
        // content at this offset.
        continue;
      }
      const auto* properties = box->FirstFragment().PaintProperties();
      const auto* sticky = properties ? properties->StickyTranslation() : nullptr;
      if (!sticky ||
          &sticky->NearestScrollTranslationNode() != scroll_translation_) {
        // Stuck to a scroller inside the document rather than to the document
        // itself. This capture never moves those, so the box sits at one
        // document position and belongs to whichever window covers it.
        continue;
      }
      // Where the box would be with nothing shifting it: no scroll offset, no
      // sticky offset, which is document coordinates. A window that starts
      // below that has already had this box in an earlier one and is looking
      // at a repeat. A window that starts above it has not -- the box's own
      // row is in this window -- and dropping it here would lose it from the
      // picture entirely rather than de-duplicate it, which is the worse of
      // the two mistakes.
      const gfx::Rect flow = box->AbsoluteBoundingBoxRect(
          {blink::MapCoordinatesMode::kIgnoreScrollOffset,
           blink::MapCoordinatesMode::kIgnoreStickyOffset});
      if (flow.y() < window_top) {
        stuck_.push_back(sticky);
      }
    }
  }

  // Whether a chunk with this transform belongs in a window after the first.
  bool KeepInScrolledWindow(
      const blink::TransformPaintPropertyNode& transform) const {
    if (!scroll_translation_) {
      // Nothing scrolls the document, so nothing is anchored to its offset.
      return true;
    }
    for (const auto* node = &transform; node; node = node->UnaliasedParent()) {
      if (node == scroll_translation_) {
        // Reached the document's own scroll: this content moves with the
        // document, and where it lands is where it belongs.
        return true;
      }
      // Linear, because a page has a handful of stuck boxes at any one
      // scroll offset and usually none at all, which is a walk of nothing.
      for (const auto& sticky : stuck_) {
        if (sticky == node) {
          return false;
        }
      }
    }
    // The document's scroll was never on the way up, so this is painted
    // against the viewport: position:fixed, or the viewport's own furniture.
    return false;
  }

 private:
  const blink::TransformPaintPropertyNode* scroll_translation_ = nullptr;
  blink::HeapVector<blink::Member<const blink::TransformPaintPropertyNode>>
      stuck_;
};

// The document's paint as a cc display list, replayable at the root property
// tree state: what a compositor would raster a layer from. With `anchored`,
// what that object says is painted against the viewport is left out.
scoped_refptr<cc::DisplayItemList> BuildDisplayList(
    blink::LocalFrameView* view,
    const gfx::Rect& cull_rect,
    const ViewportAnchored* anchored) {
  auto list = base::MakeRefCounted<cc::DisplayItemList>();
  auto keep = [&](const blink::TransformPaintPropertyNode& transform) {
    return anchored->KeepInScrolledWindow(transform);
  };
  blink::ChunkTransformFilter filter(keep);
  blink::PaintChunksToCcLayer::ConvertInto(
      blink::PaintChunkSubset(view->GetPaintArtifact()),
      blink::PropertyTreeState::Root(), gfx::Vector2dF(),
      /*under_invalidation_checking_params=*/nullptr, *list, &cull_rect,
      anchored ? &filter : nullptr);
  list->Finalize();
  return list;
}

void DumpOps(const cc::PaintOpBuffer& buffer) {
  std::map<std::string, int> histogram;
  int total = 0;
  for (const cc::PaintOp& op : cc::PaintOpBuffer::Iterator(buffer)) {
    ++total;
    ++histogram[cc::PaintOpTypeToString(op.GetType())];
    auto describe = [&](const char* what, const SkRect& bounds) {
      LOG(INFO) << "shot: op " << what << " bounds="
                << (bounds.left() == SK_ScalarInfinity
                        ? std::string("unset")
                        : base::NumberToString(bounds.left()) + "," +
                              base::NumberToString(bounds.top()) + " " +
                              base::NumberToString(bounds.width()) + "x" +
                              base::NumberToString(bounds.height()));
    };
    switch (op.GetType()) {
      case cc::PaintOpType::kSaveLayer:
        describe("SaveLayer", static_cast<const cc::SaveLayerOp&>(op).bounds);
        break;
      case cc::PaintOpType::kSaveLayerAlpha:
        describe("SaveLayerAlpha",
                 static_cast<const cc::SaveLayerAlphaOp&>(op).bounds);
        break;
      case cc::PaintOpType::kSaveLayerFilters:
        describe("SaveLayerFilters",
                 static_cast<const cc::SaveLayerFiltersOp&>(op).bounds);
        break;
      default:
        break;
    }
  }
  std::string summary;
  for (const auto& [name, count] : histogram) {
    summary += name + "=" + base::NumberToString(count) + " ";
  }
  LOG(INFO) << "shot: paint has " << total << " ops: " << summary;
}

int MaximumImageDimension(const std::string& type) {
  if (type == "webp") {
    return kMaximumWebpDimension;
  }
  if (type == "jpeg") {
    return kMaximumJpegDimension;
  }
  return kMaximumPngDimension;
}

// Refuses a bitmap the format or the memory budget cannot take, and says which
// and what to do about it. `tiled` picks the advice: a tiles request that is
// too big wants a smaller tile, a plain one wants to become a tiles request.
base::expected<void, std::string> CheckImageSize(
    int width,
    int height,
    const ScreenshotRequest& request,
    bool tiled) {
  const std::string size =
      base::NumberToString(width) + "x" + base::NumberToString(height);
  if (width <= 0 || height <= 0) {
    return base::unexpected("the requested region scales to an empty " + size +
                            " image");
  }
  const char* advice = tiled ? "; lower scale, or use a smaller tile.height"
                             : "; lower scale, or capture in tiles";
  const int limit = MaximumImageDimension(request.type);
  if (width > limit || height > limit) {
    return base::unexpected("the requested region scales to a " + size +
                            " image, and " + request.type + " cannot exceed " +
                            base::NumberToString(limit) + " pixels on a side" +
                            advice);
  }
  const int64_t bytes = int64_t{width} * height * 4;
  if (bytes > kMaximumSurfaceBytes) {
    return base::unexpected(
        "the requested region scales to a " + size + " image, which needs " +
        base::NumberToString(bytes >> 20) + " MB to raster" + advice);
  }
  return base::ok();
}

}  // namespace

ShotRenderer::ShotRenderer() = default;

ShotRenderer::~ShotRenderer() {
  TearDown();
}

void ShotRenderer::TearDown() {
  if (frame_) {
    frame_->Detach(blink::FrameDetachType::kRemove);
  }
  frame_ = nullptr;
  page_ = nullptr;
}

base::expected<void, std::string> ShotRenderer::WaitForLoad(
    const std::string& wait_until,
    base::TimeDelta timeout) {
  blink::Document* document = frame_->GetDocument();
  if (!document) {
    return base::unexpected("the frame has no document after install");
  }

  const bool network_idle = (wait_until == "networkidle");
  const base::TimeTicks deadline = base::TimeTicks::Now() + timeout;
  base::TimeTicks quiet_since;
  base::TimeTicks last_lifecycle;
  int rounds = 0;
  int lifecycles = 0;
  const bool profile_wait = ProfileEnabled() && EnvInt("SHOT_PROFILE_WAIT", 0);
  // SHOT_WAIT_GC_MB lowers the floor, which is how the collection below is
  // exercised on a page that does not churn enough to reach it.
  const size_t collect_floor =
      static_cast<size_t>(EnvInt(
          "SHOT_WAIT_GC_MB", static_cast<int>(kWaitCollectFloorBytes >> 20)))
      << 20;
  size_t collect_above = collect_floor;

  while (true) {
    // The lifecycle has to run inside the loop, not just after it. A
    // @font-face is not fetched because it was parsed; it is fetched because
    // shaping found text that uses it, and that happens in layout. Pumping
    // without running the lifecycle would wait forever for requests that had
    // never been made.
    //
    // But not on every round. A font that arrives makes the whole document
    // lay out again, and a page whose text spans sixty unicode-range subsets
    // of one family laid its 40,000 pixels out sixty times -- 12 ms and
    // 1.5 MB of garbage each -- for a result that needed one. So the
    // lifecycle runs when it can discover requests: on the first round, when
    // nothing is in flight (which is also what makes the final layout reflect
    // the last arrival), and otherwise at most every kLifecycleInterval, so
    // that a stylesheet that lands while a slow image is still loading still
    // has its fonts requested promptly.
    const unsigned in_flight = document->Fetcher()->ActiveRequestCount();
    const bool run_lifecycle =
        rounds == 0 || in_flight == 0 ||
        base::TimeTicks::Now() - last_lifecycle >= kLifecycleInterval;
    if (run_lifecycle) {
      RunLifecycle(document, rounds);
      last_lifecycle = base::TimeTicks::Now();
      ++lifecycles;
    }

    // Park the images that have finished arriving: their bytes go to the
    // runtime's parking file on a background thread and the copy here is
    // dropped when that lands (a task on this thread, which the pump below
    // runs). Left to itself the manager would do this two seconds from now.
    //
    // The manager would do this two seconds from now, which is two seconds
    // after this capture has finished. Asking on every round is what makes it
    // happen while the page is still loading, which is the only time it is
    // worth anything: the bytes go out on a background thread during a wait
    // the main thread was going to spend on the network regardless.
    if (ParkImagesEnabled() &&
        blink::ParkableImageManager::Instance().Size() > 0) {
      ProvideParkingFile();
      blink::ParkableImageManager::Instance().MaybeParkImagesForTesting();
    }

    // Behind its own switch: the memory read alone costs a millisecond, and
    // a page that waits three hundred rounds would spend a third of its load
    // measuring itself.
    if (profile_wait) {
      LogMemoryStage(base::StringPrintf("wait %d", rounds).c_str());
    }

    const unsigned active = document->Fetcher()->ActiveRequestCount();
    const bool loaded = run_lifecycle && document->HasFinishedParsing() &&
                        document->IsLoadCompleted() && active == 0;

    if (!network_idle) {
      if (loaded) {
        LOG(INFO) << "shot: load settled after " << rounds << " round(s), "
                  << lifecycles << " layout(s)";
        if (ParkImagesEnabled()) {
          // One more turn of the loop for the last images' parking to land
          // before the raster starts from what is left resident.
          PumpFor(kPumpSlice);
        }
        return base::ok();
      }
    } else if (loaded) {
      // The quiet window starts when the last request finishes, not when the
      // load event fires, so a document that finished loading long ago
      // satisfies networkidle on the next two checks rather than never.
      const base::TimeTicks now = base::TimeTicks::Now();
      if (quiet_since.is_null()) {
        quiet_since = now;
      } else if (now - quiet_since >= kNetworkIdleWindow) {
        LOG(INFO) << "shot: network idle after " << rounds << " round(s), "
                  << lifecycles << " layout(s)";
        return base::ok();
      }
    } else {
      quiet_since = base::TimeTicks();
    }

    // Collection is off for the capture (see RenderDocument), which is right
    // for a document that is laid out once and wrong for one that is laid out
    // again every time a font lands: everything the previous layout built is
    // garbage, and it stayed until the capture ended. So while requests are
    // still arriving, the heap is collected each time it has doubled since
    // the last time it was measured -- proportional, so a page whose heap is
    // genuinely large pays once, and a page that only churns pays a few
    // milliseconds to stay small.
    if (run_lifecycle && active > 0 && gc_disabled_) {
      blink::ThreadState* thread_state = blink::ThreadState::Current();
      const size_t used =
          cppgc::CollectStatistics(thread_state->heap_handle(),
                                   cppgc::HeapStatistics::kBrief)
              .used_size_bytes;
      if (used > collect_above) {
        const base::TimeTicks gc_started = base::TimeTicks::Now();
        SetGarbageCollection(/*enabled=*/true);
        thread_state->heap().ForceGarbageCollectionSlow(
            "shot", "while loading",
            cppgc::Heap::StackState::kMayContainHeapPointers);
        SetGarbageCollection(/*enabled=*/false);
        const size_t after =
            cppgc::CollectStatistics(thread_state->heap_handle(),
                                     cppgc::HeapStatistics::kBrief)
                .used_size_bytes;
        // Next when it has doubled again -- from what survived, or from what
        // there was if little of it was garbage.
        collect_above = std::max(collect_floor,
                                 2 * (used - after < used / 4 ? used : after));
        if (ProfileEnabled()) {
          LOG(INFO) << "shot: profile gc_while_loading="
                    << (base::TimeTicks::Now() - gc_started).InMillisecondsF()
                    << " used_before_kb=" << (used >> 10)
                    << " used_after_kb=" << (after >> 10)
                    << " next_above_kb=" << (collect_above >> 10);
        }
      }
    }

    if (rounds < 4) {
      LOG(INFO) << "shot: round " << rounds
                << " parsed=" << document->HasFinishedParsing()
                << " loaded=" << document->IsLoadCompleted()
                << " active=" << active;
    }
    ++rounds;
    if (base::TimeTicks::Now() > deadline) {
      return base::unexpected(
          "the document did not finish loading within " +
          base::NumberToString(timeout.InMilliseconds()) + "ms (" +
          base::NumberToString(document->Fetcher()->ActiveRequestCount()) +
          " request(s) still active)");
    }

    // ShotURLLoader answers by posting back to this thread, so draining the
    // queue is what delivers a response; the loop then decides whether the
    // newly arrived font or image is worth a layout yet.
    const base::TimeTicks pump_started = base::TimeTicks::Now();
    PumpFor(kPumpSlice);
    if (ProfileEnabled()) {
      LOG(INFO) << "shot: profile pump="
                << (base::TimeTicks::Now() - pump_started).InMillisecondsF();
    }
  }
}

void ShotRenderer::SetGarbageCollection(bool enabled) {
  blink::ThreadState* thread_state = blink::ThreadState::Current();
  if (!thread_state || gc_disabled_ == !enabled) {
    return;
  }
  if (enabled) {
    cppgc::subtle::NoGarbageCollectionScope::Leave(thread_state->heap_handle());
  } else {
    cppgc::subtle::NoGarbageCollectionScope::Enter(thread_state->heap_handle());
  }
  gc_disabled_ = !enabled;
}

void ShotRenderer::RunLifecycle(blink::Document* document, int round) {
  // SHOT_PROFILE=1 splits the lifecycle into style, layout and prepaint+paint
  // per round, which is how the cost of a slow page is attributed. Running the
  // phases separately costs a little more than one UpdateAllLifecyclePhases
  // would, so it is behind the flag rather than always on.
  if (!ProfileEnabled()) {
    frame_->View()->UpdateAllLifecyclePhases(
        blink::DocumentUpdateReason::kBeginMainFrame);
    return;
  }
  const base::TimeTicks t0 = base::TimeTicks::Now();
  document->UpdateStyleAndLayoutTree();
  const base::TimeTicks t1 = base::TimeTicks::Now();
  frame_->View()->UpdateLifecycleToLayoutClean(
      blink::DocumentUpdateReason::kBeginMainFrame);
  const base::TimeTicks t2 = base::TimeTicks::Now();
  frame_->View()->UpdateAllLifecyclePhases(
      blink::DocumentUpdateReason::kBeginMainFrame);
  const base::TimeTicks t3 = base::TimeTicks::Now();
  const size_t used =
      cppgc::CollectStatistics(blink::ThreadState::Current()->heap_handle(),
                               cppgc::HeapStatistics::kBrief)
          .used_size_bytes;
  LOG(INFO) << "shot: profile round " << round
            << " heap_used_kb=" << (used >> 10)
            << " style=" << (t1 - t0).InMillisecondsF()
            << " layout=" << (t2 - t1).InMillisecondsF()
            << " prepaint+paint=" << (t3 - t2).InMillisecondsF()
            << " parsed=" << document->HasFinishedParsing()
            << " loaded=" << document->IsLoadCompleted()
            << " active=" << document->Fetcher()->ActiveRequestCount();
}

base::expected<gfx::Rect, std::string> ShotRenderer::ResolveCaptureRect(
    const ScreenshotRequest& request) {
  const gfx::Rect viewport(0, 0, request.width, request.height);

  if (!request.selector.empty()) {
    blink::Document* document = frame_->GetDocument();
    // A default-constructed ExceptionState records what was thrown instead of
    // throwing it, which is what a selector coming from outside the process
    // needs: an invalid one is an error message, not a crash. There is no V8 to
    // throw into anyway.
    blink::ExceptionState exception_state;
    blink::Element* element = document->QuerySelector(
        blink::AtomicString(blink::String::FromUtf8(request.selector)),
        exception_state);
    if (exception_state.HadException()) {
      return base::unexpected("selector is not valid: " +
                              exception_state.Message().Utf8());
    }
    if (!element) {
      return base::unexpected("no element matches the selector " +
                              request.selector);
    }
    auto read_bounds = [&]() -> base::expected<gfx::Rect, std::string> {
      const blink::LayoutObject* layout = element->GetLayoutObject();
      if (!layout) {
        return base::unexpected("the element matching " + request.selector +
                                " has no box to photograph (display:none?)");
      }
      const gfx::Rect bounds = layout->AbsoluteBoundingBoxRect();
      if (bounds.IsEmpty()) {
        return base::unexpected("the element matching " + request.selector +
                                " has a zero-sized box");
      }
      return bounds;
    };

    auto bounds = read_bounds();
    if (!bounds.has_value()) {
      return base::unexpected(bounds.error());
    }

    // record_whole_document retains ordinary content beyond the right and
    // bottom edges without changing layout. Negative document coordinates are
    // different: there is no scrollable surface there to record. A common
    // screenshot-card layout creates them by centering a fixed-size element in
    // a smaller default viewport. Grow only that case, then re-read the box;
    // leaving positive overflow alone preserves media queries and vw/vh.
    for (int pass = 0; pass < 3 && (bounds->x() < 0 || bounds->y() < 0);
         ++pass) {
      blink::LocalFrameView* view = frame_->View();
      const gfx::Size current = view->Size();
      const int width = std::min(
          std::max({current.width(), bounds->width(), bounds->right()}),
          kMaximumDimension);
      const int height = std::min(
          std::max({current.height(), bounds->height(), bounds->bottom()}),
          kMaximumDimension);
      if (width == current.width() && height == current.height()) {
        break;
      }
      const gfx::Size grown(width, height);
      view->Resize(grown);
      page_->GetVisualViewport().SetSize(grown);
      view->UpdateAllLifecyclePhases(
          blink::DocumentUpdateReason::kBeginMainFrame);
      bounds = read_bounds();
      if (!bounds.has_value()) {
        return base::unexpected(bounds.error());
      }
    }
    return *bounds;
  }

  if (request.clip.has_value()) {
    const Clip& clip = *request.clip;
    const gfx::Rect rect(clip.x, clip.y, clip.width, clip.height);
    return rect;
  }

  if (request.full_page) {
    const gfx::Size contents = frame_->View()->LayoutViewport()->ContentsSize();
    return gfx::Rect(0, 0, std::max(request.width, contents.width()),
                     std::max(request.height, contents.height()));
  }

  return viewport;
}

base::expected<void, std::string> ShotRenderer::CreatePage(
    const RenderInput& input,
    const ScreenshotRequest& request) {
  blink::MainThreadScheduler* scheduler =
      blink::Thread::MainThread()->Scheduler()->ToMainThreadScheduler();
  if (!scheduler) {
    return base::unexpected("blink main thread scheduler is not available");
  }

  auto* chrome_client = blink::MakeGarbageCollected<ChromeClient>();
  chrome_client->SetDeviceScaleFactor(static_cast<float>(request.scale));
  page_ = blink::Page::CreateNonOrdinary(
      *chrome_client, *scheduler->CreateAgentGroupScheduler(),
      /*color_provider_colors=*/nullptr);
  if (!page_) {
    return base::unexpected("could not create the page");
  }

  // Script never runs: there is no V8 in this binary at all. Saying so up front
  // means the code paths that ask "can this document run script?" answer
  // consistently rather than discovering it at the point of execution.
  blink::Settings& settings = page_->GetSettings();
  ApplyChromeWebPreferences(settings);
  settings.SetScriptEnabled(false);
  settings.SetPluginsEnabled(false);

  // The encoding from the Content-Type header, where there was one. This is the
  // *default* encoding rather than an override, so a document that declares its
  // own -- a meta charset, or a byte order mark -- still wins, which is the
  // wrong way round from the HTML spec's precedence but is the only lever
  // ForceSynchronousDocumentInstall leaves: it takes a MIME type and bytes, and
  // there is no DocumentLoader here to carry the header down.
  if (!input.charset.empty()) {
    settings.SetDefaultTextEncodingName(
        blink::AtomicString(blink::String::FromUtf8(input.charset)));
  }

  // The same setting --hide-scrollbars sets. A scrollbar is chrome around the
  // document rather than part of it, it is drawn in the platform's own style,
  // and it narrows the layout viewport -- so leaving it on would reflow every
  // line in the page relative to the oracle as well as painting a strip down
  // the right-hand side.
  settings.SetHideScrollbars(true);
  if (request.full_page || request.clip.has_value() ||
      !request.selector.empty()) {
    // Chrome's Page.captureScreenshot(captureBeyondViewport=true) sets
    // record_whole_document, which WebViewImpl maps to this setting. It keeps
    // the caller's layout viewport intact while retaining display items beyond
    // it, so selector/clip/fullPage do not re-run media queries or change vw/vh
    // merely because a larger output region was requested.
    settings.SetMainFrameClipsContent(false);
  }

  // What navigator.userAgent and resolution media queries answer. The scale
  // override is the same lever DevTools' device emulation pulls; it changes
  // what the document is told about its pixels without changing its layout,
  // which is exactly what a deviceScaleFactor is.
  page_->SetInspectorDeviceScaleFactorOverride(
      static_cast<float>(request.scale));

  auto* frame_client = blink::MakeGarbageCollected<FrameClient>();
  frame_ = blink::MakeGarbageCollected<blink::LocalFrame>(
      frame_client, *page_, /*owner=*/nullptr, /*parent=*/nullptr,
      /*previous_sibling=*/nullptr,
      blink::FrameInsertType::kInsertInConstructor, blink::LocalFrameToken(),
      /*inheriting_agent_factory=*/nullptr, /*interface_registry=*/nullptr,
      mojo::NullRemote());
  frame_->SetView(blink::MakeGarbageCollected<blink::LocalFrameView>(*frame_));
  frame_->Init(/*opener=*/nullptr, blink::DocumentToken(),
               /*policy_container=*/nullptr, blink::StorageKey(),
               /*document_ukm_source_id=*/ukm::kInvalidSourceId,
               /*creator_base_url=*/blink::NullUrl());

  const gfx::Size viewport(request.width, request.height);
  frame_->View()->Resize(viewport);
  page_->GetVisualViewport().SetSize(viewport);

  // With omitBackground the view stops painting its opaque base colour, so
  // whatever the document does not cover stays at the surface's own zeroed
  // pixels: transparent. Without it the base colour is white, which is what a
  // browser paints behind a page that declares no background.
  if (request.omit_background) {
    frame_->View()->SetBaseBackgroundColor(blink::Color::kTransparent);
  }
  return base::ok();
}

base::expected<EncodedTile, std::string> ShotRenderer::Render(
    RenderInput& input,
    const ScreenshotRequest& request) {
  if (request.tile.has_value()) {
    return base::unexpected(
        "a request with tile set is answered by RenderTiles");
  }
  EncodedTile image;
  auto rendered = RenderDocument(
      input, request,
      base::BindRepeating(
          [](EncodedTile* out,
             EncodedTile tile) -> base::expected<void, std::string> {
            *out = std::move(tile);
            return base::ok();
          },
          &image));
  // What the raster freed -- strips, decoded images, display lists -- goes
  // back to the system now rather than at the next idle purge, so that a
  // worker between requests is the size of a worker between requests.
  if (ReclaimEnabled()) {
    ::partition_alloc::MemoryReclaimer::Instance()->ReclaimAll();
  }
  if (!rendered.has_value()) {
    return base::unexpected(rendered.error());
  }
  return image;
}

base::expected<void, std::string> ShotRenderer::RenderTiles(
    RenderInput& input,
    const ScreenshotRequest& request,
    const TileSink& sink) {
  if (!request.tile.has_value()) {
    return base::unexpected("RenderTiles needs tile.height");
  }
  auto rendered = RenderDocument(input, request, sink);
  if (ReclaimEnabled()) {
    ::partition_alloc::MemoryReclaimer::Instance()->ReclaimAll();
  }
  return rendered;
}

base::expected<void, std::string> ShotRenderer::RenderDocument(
    RenderInput& input,
    const ScreenshotRequest& request,
    const TileSink& sink) {
  // Whatever happens below, this instance ends the call with no page attached.
  // A resident worker calls Render() many times, and CreatePage() overwrites
  // page_ and frame_ unconditionally -- without this the previous document's
  // frame would be dropped still attached, taking its Document, its
  // ResourceFetcher and everything they hold with it.
  base::ScopedClosureRunner tear_down(
      base::BindOnce(&ShotRenderer::TearDown, base::Unretained(this)));
  TearDown();

  // Blink's heap is a standalone cppgc heap, because there is no V8 here to own
  // a unified one. cppgc grows it from 1 MB in 1.5x steps and collects at every
  // step it crosses, with marking and sweeping both atomic -- the whole heap,
  // stop-the-world. That is a reasonable policy for a browser tab, where the
  // allocation is spread over a session and Oilpan's increments ride along with
  // V8's own collections.
  //
  // A capture is the opposite shape: it allocates a whole document in one go
  // and keeps essentially all of it alive until the paint record is taken.
  // Laying out a 36,000-element page allocates about 270 MB, crossing the step
  // more than a dozen times, and every crossing marks a heap that is almost
  // entirely live. Measured, that was 500 ms of a 750 ms capture, and it is why
  // large pages were the one case Chrome was faster at.
  //
  // So collection is disabled for the duration of a capture -- nothing in one
  // benefits from it, since the document, its style and its layout tree are
  // live from install until paint -- and paid for here instead, where the
  // previous document has just been torn down and its entire object graph is
  // garbage. The cost is proportional to what there is to reclaim: 0.4 ms for a
  // small page, 66 ms after a 270 MB one.
  //
  // The consequence to be aware of is that within a single capture the heap
  // only grows. A page that churns garbage across many lifecycle rounds -- a
  // slow drip of fonts, say -- holds all of it until the capture ends. That is
  // bounded by the request timeout, and by the size of the document that
  // request asked for.
  blink::ThreadState* thread_state = blink::ThreadState::Current();
  if (thread_state) {
    const size_t used = cppgc::CollectStatistics(thread_state->heap_handle(),
                                                 cppgc::HeapStatistics::kBrief)
                            .used_size_bytes;
    // Below this there is not enough garbage to be worth a stop-the-world
    // sweep, and skipping it lets a run of small captures amortise one
    // collection over several documents rather than paying per capture. It has
    // to be a size rather than "every capture" in one direction and cannot be
    // omitted in the other: with collection disabled during captures and no
    // allocation happening between them, this call is the only thing that ever
    // reclaims anything.
    constexpr size_t kCollectAboveBytes = 16u << 20;
    if (used > kCollectAboveBytes) {
      const base::TimeTicks gc_started = base::TimeTicks::Now();
      thread_state->heap().ForceGarbageCollectionSlow(
          "shot", "between captures",
          cppgc::Heap::StackState::kMayContainHeapPointers);
      if (ProfileEnabled()) {
        const size_t after =
            cppgc::CollectStatistics(thread_state->heap_handle(),
                                     cppgc::HeapStatistics::kBrief)
                .used_size_bytes;
        LOG(INFO) << "shot: profile gc_between="
                  << (base::TimeTicks::Now() - gc_started).InMillisecondsF()
                  << " used_before_kb=" << (used >> 10)
                  << " used_after_kb=" << (after >> 10);
      }
    }
  }
  // Released after `tear_down` is declared, so it runs first: the teardown
  // this guards must not run with collection still disabled. A flag on the
  // renderer rather than a scope object on this stack because WaitForLoad()
  // lifts it for a collection of its own when a slow load has churned enough
  // garbage to be worth one.
  base::ScopedClosureRunner enable_gc(
      base::BindOnce(&ShotRenderer::SetGarbageCollection,
                     base::Unretained(this), /*enabled=*/true));
  SetGarbageCollection(/*enabled=*/false);

  CaptureStats* stats =
      CaptureContext::Current() ? &CaptureContext::Current()->stats() : nullptr;
  LogMemoryStage("start");
  const base::TimeTicks setup_started = base::TimeTicks::Now();
  if (auto created = CreatePage(input, request); !created.has_value()) {
    return base::unexpected(created.error());
  }
  LogMemoryStage("page");

  // ForceSynchronousDocumentInstall parses the bytes into this frame's document
  // with no navigation and no DocumentLoader: exactly one document, delivered
  // by the caller. Subresources (<img>, @font-face, <link rel=stylesheet>) go
  // through ResourceFetcher to ShotURLLoader.
  //
  // Local resource access, before the parse rather than after it: an <img> is
  // requested while the parser is still running, and a request refused then is
  // not retried.
  //
  // SecurityOrigin::CanDisplay() refuses a file: URL unless the requesting
  // origin CanLoadLocalResources(), and nothing has said this one can. In a
  // browser that flag arrives with the navigation -- DocumentLoader sets it
  // from grant_load_local_resources_, which the browser process sets when it
  // commits a file: URL it has decided this renderer may read. shot has no
  // browser process to decide, so the caller decides, per request.
  if (request.allow_file_access) {
    if (blink::LocalDOMWindow* window = frame_->DomWindow()) {
      window->GetMutableSecurityOrigin()->GrantLoadLocalResources();
    }
  }

  const base::TimeTicks install_started = base::TimeTicks::Now();
  {
    scoped_refptr<blink::SharedBuffer> data =
        blink::SharedBuffer::Create(base::span(input.body));
    // The caller's copy is dead the moment blink has one: the parser reads
    // the buffer below, and nothing after this looks at the bytes again. On a
    // page whose <style> block is three megabytes, keeping it to the end of
    // the capture was three megabytes resident for no reader.
    std::string().swap(input.body);
    frame_->ForceSynchronousDocumentInstall(
        blink::AtomicString(blink::String::FromUtf8(
            input.mime_type.empty() ? "text/html" : input.mime_type)),
        *data, blink::KURL(input.url));
  }

  if (stats) {
    stats->setup = base::TimeTicks::Now() - setup_started;
  }
  LogMemoryStage("installed");
  if (ProfileEnabled()) {
    blink::Document* doc = frame_->GetDocument();
    LOG(INFO) << "shot: profile create_page="
              << (install_started - setup_started).InMillisecondsF()
              << " install="
              << (base::TimeTicks::Now() - install_started).InMillisecondsF()
              << " parsed=" << (doc && doc->HasFinishedParsing())
              << " loaded=" << (doc && doc->IsLoadCompleted()) << " lifecycle="
              << (doc ? static_cast<int>(doc->Lifecycle().GetState()) : -1)
              << " has_layout_view=" << (doc && doc->GetLayoutView())
              << " needs_layout="
              << (doc && doc->GetLayoutView() &&
                  doc->GetLayoutView()->NeedsLayout());
  }

  blink::LocalFrameView* view = frame_->View();
  if (!view) {
    return base::unexpected("the frame has no view after install");
  }

  const base::TimeTicks wait_started = base::TimeTicks::Now();
  if (auto waited = WaitForLoad(request.wait_until,
                                base::Milliseconds(request.timeout_ms));
      !waited.has_value()) {
    return base::unexpected(waited.error());
  }
  if (stats) {
    stats->wait = base::TimeTicks::Now() - wait_started;
  }
  LogMemoryStage("loaded");

  const base::TimeTicks lifecycle_started = base::TimeTicks::Now();
  auto capture = ResolveCaptureRect(request);
  if (!capture.has_value()) {
    return base::unexpected(capture.error());
  }

  // Style, layout, prepaint and paint. The bool is "did the lifecycle reach
  // paint-clean"; if it did not, the PaintRecord below would be of a
  // half-updated tree, which is worth failing on rather than shipping.
  if (!view->UpdateAllLifecyclePhases(
          blink::DocumentUpdateReason::kBeginMainFrame)) {
    return base::unexpected("the document did not reach a painted state");
  }
  if (stats) {
    stats->lifecycle = base::TimeTicks::Now() - lifecycle_started;
  }
  LogMemoryStage("lifecycle");

  // Where the pixels come from and where they go.
  //
  // The document's paint is taken as cc's own display list rather than a
  // flat record, because a display list carries an R-tree over its paint
  // chunks: a strip of the image plays only the chunks that touch it, which is
  // what makes strips cheaper than a surface rather than dearer. One list
  // serves every slice that is painted at the same scroll position.
  //
  // Blink stops painting at kPaintedExtent, so a region that reaches further
  // down is taken in windows: the document is scrolled so that each window's
  // top row paints near the origin, and the rows are placed in the image
  // where they belong. A region inside the extent is one window that is never
  // scrolled -- exactly what a viewport capture was before windows existed.
  const gfx::Rect region = capture.value();
  if (region.right() > kPaintedExtent) {
    return base::unexpected(
        "the region is " + base::NumberToString(region.width()) +
        " CSS pixels wide and the engine paints at most " +
        base::NumberToString(kPaintedExtent) + "; use a narrower viewport");
  }
  const bool tiled = request.tile.has_value();
  const double scale = request.scale;
  const SkSurfaceProps surface_props =
      skia::LegacyDisplayGlobals::GetSkSurfaceProps();
  // Without omitBackground the view paints its base colour under everything
  // in the region, so every pixel is opaque and the encoders are told so up
  // front: PNG writes RGB and WebP has no alpha plane. A streamed image
  // cannot find this out by reading itself back the way a bitmap could.
  const bool opaque = !request.omit_background;

  // The region in slices: a tile each for a tiles request, otherwise as many
  // rows as one paint covers. Consecutive slices are grouped into windows --
  // the most rows one scroll position lets blink paint -- so that a document
  // is scrolled, and painted, as few times as it takes.
  const int slice_height = tiled ? request.tile->height : kPaintWindow;
  if (slice_height > kPaintWindow) {
    return base::unexpected("tile.height is at most " +
                            base::NumberToString(kPaintWindow));
  }
  std::vector<gfx::Rect> slices;
  for (int y = region.y(); y < region.bottom(); y += slice_height) {
    slices.emplace_back(region.x(), y, region.width(),
                        std::min(slice_height, region.bottom() - y));
  }
  struct Window {
    int top;
    int bottom;
    size_t first_slice;
    size_t end_slice;
  };
  std::vector<Window> windows;
  const bool scrolled = region.bottom() > kPaintedExtent;
  if (!scrolled) {
    windows.push_back({0, region.bottom(), 0, slices.size()});
  }
  for (size_t i = 0; scrolled && i < slices.size(); ++i) {
    if (!windows.empty() &&
        slices[i].bottom() - windows.back().top <= kPaintWindow) {
      windows.back().bottom = slices[i].bottom();
      windows.back().end_slice = i + 1;
      continue;
    }
    windows.push_back({slices[i].y(), slices[i].bottom(), i, i + 1});
  }

  const int width_px = static_cast<int>(std::lround(region.width() * scale));
  const int height_px = static_cast<int>(std::lround(region.height() * scale));
  auto rows_px = [&](int css_y) {
    return static_cast<int>(std::lround((css_y - region.y()) * scale));
  };
  std::unique_ptr<ImageStream> whole;
  if (!tiled) {
    if (auto size =
            CheckImageSize(width_px, height_px, request, /*tiled=*/false);
        !size.has_value()) {
      return base::unexpected(size.error());
    }
    auto output = OpenOutput(request.path, 1);
    if (!output.has_value()) {
      return base::unexpected(output.error());
    }
    auto stream = ImageStream::Create(width_px, height_px, request, opaque,
                                      surface_props, std::move(*output));
    if (!stream.has_value()) {
      return base::unexpected(stream.error());
    }
    whole = std::move(*stream);
  }
  auto account = [&](const ImageStreamStats& s, const gfx::Rect& what) {
    if (stats) {
      stats->raster += s.raster;
      stats->encode += s.encode;
    }
    LOG(INFO) << "shot: raster " << what.ToString() << " strips=" << s.strips
              << " threads=" << s.threads
              << " window_kb=" << (s.peak_window_bytes >> 10)
              << " decoded_kb=" << (s.peak_decoded_bytes >> 10)
              << " decode_ms=" << s.decode.InMillisecondsF();
  };

  struct PreparedWindow {
    Window window;
    int offset;
    scoped_refptr<cc::DisplayItemList> list;
  };
  std::vector<PreparedWindow> prepared_windows;
  prepared_windows.reserve(windows.size());

  base::TimeDelta scrolling;
  for (const Window& window : windows) {
    // Scroll so the window's top row paints at the origin. The offset reached
    // can fall short of what was asked for -- the document cannot scroll past
    // its last viewport's worth -- which only moves the window's painted
    // coordinates down a little, so everything is measured from the offset
    // actually reached rather than the one asked for.
    int offset = 0;
    if (scrolled) {
      const base::TimeTicks scroll_started = base::TimeTicks::Now();
      auto reached = ScrollTo(view, window.top);
      if (!reached.has_value()) {
        return base::unexpected(reached.error());
      }
      scrolling += base::TimeTicks::Now() - scroll_started;
      offset = *reached;
      if (window.bottom - offset > kPaintedExtent) {
        return base::unexpected(
            "the document could not be scrolled to " +
            base::NumberToString(window.top) + " (it stopped at " +
            base::NumberToString(offset) + "), so rows below " +
            base::NumberToString(offset + kPaintedExtent) +
            " cannot be painted");
      }
    }

    const gfx::Rect& first_slice = slices[window.first_slice];
    const gfx::Rect& last_slice = slices[window.end_slice - 1];
    const gfx::Rect paint_cull(first_slice.x(), first_slice.y() - offset,
                               first_slice.width(),
                               last_slice.bottom() - first_slice.y());
    // Every window but the first leaves out what is painted against the
    // viewport, so that a fixed header is in the photograph once rather than
    // once per scroll position. Read after the scroll, because which sticky
    // boxes are stuck depends on it.
    std::optional<ViewportAnchored> anchored;
    if (!prepared_windows.empty()) {
      anchored.emplace(view, first_slice.y());
    }

    const base::TimeTicks paint_started = base::TimeTicks::Now();
    scoped_refptr<cc::DisplayItemList> list = BuildDisplayList(
        view, paint_cull, anchored.has_value() ? &*anchored : nullptr);
    if (stats) {
      stats->paint += base::TimeTicks::Now() - paint_started;
    }
    if (ProfileEnabled()) {
      LOG(INFO) << "shot: profile display_list_kb=" << (list->BytesUsed() >> 10)
                << " ops=" << list->TotalOpCount();
    }
    LogMemoryStage("record");
    // SHOT_DUMP_OPS=1 logs what the paint is made of, so that a slow raster
    // can be read off the ops rather than guessed at. An unbounded saveLayer
    // shows up here as "bounds=unset", which is what every page-sized
    // intermediate this pipeline has hit looked like before it was bounded.
    if (EnvInt("SHOT_DUMP_OPS", 0)) {
      DumpOps(list->paint_op_buffer());
    }

    prepared_windows.push_back({window, offset, std::move(list)});
  }

  // A finalized display list owns everything needed to raster its images.
  // Once every scroll position has one, the Page, DOM, layout tree and resource
  // cache no longer contribute to the raster/encode peak. Keeping only the
  // lists also narrows the lifetime of encoded image data: an image used only
  // in an earlier paint window goes away when that window's list is released,
  // while an image repeated in a later window remains alive through that list.
  // This is the same ownership boundary between Blink paint and compositor
  // raster in a browser, made explicit because Shot performs both in one
  // process.
  view = nullptr;
  SetGarbageCollection(/*enabled=*/true);
  TearDown();
  if (thread_state) {
    // kNoHeapPointers: a precise collection that ignores this thread's stack.
    // A conservative one keeps alive whatever a stale local in this frame or
    // its callers still points at -- the Document, its images, their bytes --
    // and nothing below this line touches a blink object again: the display
    // lists are cc, and the slices are geometry.
    thread_state->heap().ForceGarbageCollectionSlow(
        "shot", "page painted", cppgc::Heap::StackState::kNoHeapPointers);
  }
  // The collection frees the page's objects into PartitionAlloc's free lists,
  // which stay committed. Decommit them now, or the raster below starts from
  // the page's footprint rather than the display lists'.
  if (ReclaimEnabled()) {
    ::partition_alloc::MemoryReclaimer::Instance()->ReclaimAll();
  }
  LogMemoryStage("detached");

  // The page is gone, so an image's compressed bytes have exactly one
  // remaining reader: the slices below. Each slice may drop an image once its
  // last strip is encoded, unless a later slice -- further down this window or
  // in the next one, since the windows overlap at their seam -- draws it
  // again. Walked backwards so that each slice sees the union of what follows.
  std::vector<base::flat_set<cc::PaintImage::Id>> drawn_after(slices.size());
  {
    base::flat_set<cc::PaintImage::Id> later;
    for (size_t w = prepared_windows.size(); w-- > 0;) {
      const PreparedWindow& prepared = prepared_windows[w];
      scoped_refptr<cc::DiscardableImageMap> map =
          prepared.list->GenerateDiscardableImageMap(cc::ScrollOffsetMap());
      for (size_t i = prepared.window.end_slice;
           i-- > prepared.window.first_slice;) {
        drawn_after[i] = later;
        if (!map || map->empty()) {
          continue;
        }
        const gfx::Rect& slice = slices[i];
        // One row of slack, as the stream's own query allows for a slice
        // boundary that falls inside a device pixel.
        const gfx::Rect painted(slice.x(), slice.y() - prepared.offset,
                                slice.width(), slice.height() + 1);
        for (const cc::DrawImage* draw :
             map->GetDiscardableImagesInRect(painted)) {
          later.insert(draw->paint_image().stable_id());
        }
      }
    }
  }

  for (PreparedWindow& prepared : prepared_windows) {
    const Window& window = prepared.window;
    const int offset = prepared.offset;
    scoped_refptr<cc::DisplayItemList> list = std::move(prepared.list);

    for (size_t i = window.first_slice; i < window.end_slice; ++i) {
      const gfx::Rect& slice = slices[i];
      if (tiled) {
        const int tile_px =
            static_cast<int>(std::lround(slice.height() * scale));
        if (auto size =
                CheckImageSize(width_px, tile_px, request, /*tiled=*/true);
            !size.has_value()) {
          return base::unexpected(size.error());
        }
        // Tiles are numbered in the order they are delivered, which is slice
        // order.
        auto output = OpenOutput(request.path, i + 1);
        if (!output.has_value()) {
          return base::unexpected(output.error());
        }
        auto stream = ImageStream::Create(width_px, tile_px, request, opaque,
                                          surface_props, std::move(*output));
        if (!stream.has_value()) {
          return base::unexpected(stream.error());
        }
        // The tile in painted coordinates: document coordinates less the
        // scroll offset.
        const gfx::Rect cull_rect(slice.x(), slice.y() - offset, slice.width(),
                                  slice.height());
        if (auto added = (*stream)->AddSlice(list, cull_rect, scale,
                                             /*device_top=*/0, tile_px,
                                             drawn_after[i]);
            !added.has_value()) {
          return base::unexpected(added.error());
        }
        auto encoded = (*stream)->Finish();
        account((*stream)->stats(), slice);
        if (!encoded.has_value()) {
          return base::unexpected(encoded.error());
        }
        EncodedTile tile;
        tile.region = slice;
        tile.bytes = std::move(*encoded);
        tile.size = (*stream)->stats().encoded_bytes;
        if (!request.path.empty()) {
          tile.path = OutputPath(request.path, i + 1);
        }
        if (auto delivered = sink.Run(std::move(tile));
            !delivered.has_value()) {
          return base::unexpected(delivered.error());
        }
        continue;
      }
      // Into the whole image's rows for this slice. The list is placed as the
      // whole region would be, so that a device row maps to the same CSS row
      // whichever slice drew it -- there is no seam to hide.
      const int top_px = rows_px(slice.y());
      const int bottom_px = rows_px(slice.bottom());
      const gfx::Rect cull_rect(region.x(), region.y() - offset, region.width(),
                                region.height());
      if (auto added = whole->AddSlice(list, cull_rect, scale, top_px,
                                       bottom_px - top_px, drawn_after[i]);
          !added.has_value()) {
        return base::unexpected(added.error());
      }
    }
    list = nullptr;
  }
  if (stats) {
    stats->lifecycle += scrolling;
  }
  if (scrolled || tiled) {
    LOG(INFO) << "shot: banded " << region.ToString() << " into "
              << windows.size() << " window(s), " << slices.size()
              << (tiled ? " tile(s)" : " slice(s)");
  }
  if (tiled) {
    return base::ok();
  }
  auto encoded = whole->Finish();
  account(whole->stats(), region);
  if (!encoded.has_value()) {
    return base::unexpected(encoded.error());
  }
  LogMemoryStage("encoded");
  EncodedTile tile;
  tile.region = region;
  tile.bytes = std::move(*encoded);
  tile.size = whole->stats().encoded_bytes;
  if (!request.path.empty()) {
    tile.path = OutputPath(request.path, 1);
  }
  return sink.Run(std::move(tile));
}

base::expected<int, std::string> ShotRenderer::ScrollTo(
    blink::LocalFrameView* view,
    int top) {
  blink::PaintLayerScrollableArea* scroller = view->LayoutViewport();
  if (!scroller) {
    return base::unexpected("the document has no layout viewport to scroll");
  }
  scroller->SetScrollOffset(blink::ScrollOffset(0, static_cast<float>(top)),
                            blink::mojom::blink::ScrollType::kProgrammatic,
                            cc::ScrollSourceType::kAbsoluteScroll);
  // Prepaint and paint again with the new scroll translation. There is no
  // layout in this: scrolling moves where the document is painted, not where
  // anything in it is.
  if (!view->UpdateAllLifecyclePhases(
          blink::DocumentUpdateReason::kBeginMainFrame)) {
    return base::unexpected(
        "the document did not reach a painted state after scrolling");
  }
  return static_cast<int>(std::lround(scroller->GetScrollOffset().y()));
}

}  // namespace shot
