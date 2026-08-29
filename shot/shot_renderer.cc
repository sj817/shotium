// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_renderer.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "cc/paint/paint_canvas.h"
#include "cc/paint/paint_record.h"
#include "cc/paint/skia_paint_canvas.h"
#include "skia/ext/legacy_display_globals.h"
#include "shot/shot_capture_context.h"
#include "shot/shot_network.h"
#include "shot/shot_url_loader.h"
#include "third_party/blink/public/mojom/frame/frame.mojom-blink.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/public/common/tokens/tokens.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/renderer/core/frame/frame_types.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/loader/empty_clients.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/paint/paint_layer_scrollable_area.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/fonts/generic_font_family_settings.h"
#include "third_party/blink/renderer/platform/graphics/color.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/scheduler/public/agent_group_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/public/main_thread.h"
#include "third_party/blink/renderer/platform/scheduler/public/main_thread_scheduler.h"
#include "third_party/blink/renderer/platform/wtf/shared_buffer.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPixmap.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/core/SkSurfaceProps.h"
#include "ui/display/screen_info.h"
#include "ui/display/screen_infos.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/codec/webp_codec.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "third_party/icu/source/common/unicode/uscript.h"

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

// The quality used for jpeg and webp when the request did not say. Puppeteer
// leaves it to the encoder default, which for skia's jpeg encoder is 100 -- a
// file several times larger than anyone wants from a screenshot.
constexpr int kDefaultLossyQuality = 90;

// An upper bound on the viewport, so that a document whose height grows every
// time the viewport does cannot ask for an unallocatable surface.
constexpr int kMaximumDimension = 32767;

using FontFamilyUpdater = bool (blink::GenericFontFamilySettings::*)(
    const blink::AtomicString&,
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

base::expected<std::vector<uint8_t>, std::string> EncodeImage(
    const SkBitmap& bitmap,
    const ScreenshotRequest& request) {
  if (request.type == "png") {
    // discard_transparency = false: the corpus exercises alpha compositing, and
    // dropping the alpha channel here would hide any error in it -- and with
    // omitBackground it is the whole point of the request.
    std::optional<std::vector<uint8_t>> png =
        gfx::PNGCodec::FastEncodeBGRASkBitmap(
            bitmap, /*discard_transparency=*/false);
    if (!png) {
      return base::unexpected("could not encode the image as PNG");
    }
    return std::move(*png);
  }

  const int quality = request.quality.value_or(kDefaultLossyQuality);
  if (request.type == "jpeg") {
    std::optional<std::vector<uint8_t>> jpeg =
        gfx::JPEGCodec::Encode(bitmap, quality);
    if (!jpeg) {
      return base::unexpected("could not encode the image as JPEG");
    }
    return std::move(*jpeg);
  }
  if (request.type == "webp") {
    std::optional<std::vector<uint8_t>> webp =
        gfx::WebpCodec::Encode(bitmap, quality);
    if (!webp) {
      return base::unexpected("could not encode the image as WebP");
    }
    return std::move(*webp);
  }
  // ParseScreenshotRequest already rejects anything else, so reaching here
  // means the two lists have drifted apart.
  return base::unexpected("unsupported image type: " + request.type);
}

}  // namespace

ShotRenderer::ShotRenderer() = default;

ShotRenderer::~ShotRenderer() {
  TearDown();
}

void ShotRenderer::PurgeMemory() {
  surface_.reset();
  surface_width_ = 0;
  surface_height_ = 0;
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
  int rounds = 0;

  while (true) {
    // The lifecycle has to run inside the loop, not just after it. A
    // @font-face is not fetched because it was parsed; it is fetched because
    // style resolution found text that uses it, and that happens in
    // UpdateAllLifecyclePhases. Same for an <img> whose layout decides it is
    // actually going to be painted. Pumping without running the lifecycle would
    // wait forever for requests that had never been made.
    frame_->View()->UpdateAllLifecyclePhases(
        blink::DocumentUpdateReason::kBeginMainFrame);

    const unsigned active = document->Fetcher()->ActiveRequestCount();
    const bool loaded = document->HasFinishedParsing() &&
                        document->IsLoadCompleted() && active == 0;

    if (!network_idle) {
      if (loaded) {
        LOG(INFO) << "shot: load settled after " << rounds << " round(s)";
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
        LOG(INFO) << "shot: network idle after " << rounds << " round(s)";
        return base::ok();
      }
    } else {
      quiet_since = base::TimeTicks();
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
    // queue is what delivers a response; the loop then re-runs the lifecycle so
    // the newly arrived font or image can affect layout and paint.
    PumpFor(kPumpSlice);
  }
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
    for (int pass = 0;
         pass < 3 && (bounds->x() < 0 || bounds->y() < 0); ++pass) {
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
    const gfx::Size contents =
        frame_->View()->LayoutViewport()->ContentsSize();
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

base::expected<std::vector<uint8_t>, std::string> ShotRenderer::Render(
    const RenderInput& input,
    const ScreenshotRequest& request) {
  // Whatever happens below, this instance ends the call with no page attached.
  // A resident worker calls Render() many times, and CreatePage() overwrites
  // page_ and frame_ unconditionally -- without this the previous document's
  // frame would be dropped still attached, taking its Document, its
  // ResourceFetcher and everything they hold with it.
  base::ScopedClosureRunner tear_down(
      base::BindOnce(&ShotRenderer::TearDown, base::Unretained(this)));
  TearDown();

  CaptureStats* stats = CaptureContext::Current()
                            ? &CaptureContext::Current()->stats()
                            : nullptr;
  const base::TimeTicks setup_started = base::TimeTicks::Now();
  if (auto created = CreatePage(input, request); !created.has_value()) {
    return base::unexpected(created.error());
  }

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

  scoped_refptr<blink::SharedBuffer> data =
      blink::SharedBuffer::Create(base::span(input.body));
  frame_->ForceSynchronousDocumentInstall(
      blink::AtomicString(blink::String::FromUtf8(
          input.mime_type.empty() ? "text/html" : input.mime_type)),
      *data, blink::KURL(input.url));

  if (stats) {
    stats->setup = base::TimeTicks::Now() - setup_started;
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

  const gfx::Rect cull_rect = capture.value();
  const base::TimeTicks paint_started = base::TimeTicks::Now();
  cc::PaintRecord record = view->GetPaintRecord(&cull_rect);
  if (stats) {
    stats->paint = base::TimeTicks::Now() - paint_started;
  }

  const base::TimeTicks raster_started = base::TimeTicks::Now();
  const SkSurfaceProps surface_props =
      skia::LegacyDisplayGlobals::GetSkSurfaceProps();
  const int pixel_width =
      static_cast<int>(std::lround(cull_rect.width() * request.scale));
  const int pixel_height =
      static_cast<int>(std::lround(cull_rect.height() * request.scale));
  if (pixel_width <= 0 || pixel_height <= 0 ||
      pixel_width > kMaximumDimension || pixel_height > kMaximumDimension) {
    return base::unexpected(
        "the requested region scales to an unusable " +
        base::NumberToString(pixel_width) + "x" +
        base::NumberToString(pixel_height) + " image");
  }
  // The surface carries the pixel geometry, and skia will not rasterise LCD
  // text onto a surface whose geometry is unknown -- which is what the default
  // SkSurfaceProps says. ShotRuntime put the host's geometry in
  // LegacyDisplayGlobals; this is where it has to be honoured, or the SkFont
  // edging chosen for the glyphs is silently downgraded at draw time.
  const bool reuse_surface = surface_ && surface_width_ == pixel_width &&
                             surface_height_ == pixel_height;
  if (!reuse_surface) {
    surface_ = SkSurfaces::Raster(
        SkImageInfo::MakeN32Premul(pixel_width, pixel_height), &surface_props);
    if (!surface_) {
      return base::unexpected("could not allocate a " +
                              base::NumberToString(pixel_width) + "x" +
                              base::NumberToString(pixel_height) + " bitmap");
    }
    surface_width_ = pixel_width;
    surface_height_ = pixel_height;
  }
  sk_sp<SkSurface> surface = surface_;
  // Make new and retained surfaces start from the same defined state. This is
  // required for byte-identical clip/selector output and for omitBackground,
  // where uncovered pixels must remain transparent.
  surface->getCanvas()->clear(SK_ColorTRANSPARENT);

  {
    SkAutoCanvasRestore restore(surface->getCanvas(), /*doSave=*/true);
    cc::SkiaPaintCanvas canvas(surface->getCanvas());
    if (request.scale != 1.0) {
      canvas.scale(static_cast<float>(request.scale),
                   static_cast<float>(request.scale));
    }
    // The record is in document coordinates; the surface starts at the origin
    // of the requested region. Scale first, then translate, so the translation
    // is measured in the same CSS pixels the record is.
    if (cull_rect.x() != 0 || cull_rect.y() != 0) {
      canvas.translate(static_cast<float>(-cull_rect.x()),
                       static_cast<float>(-cull_rect.y()));
    }
    canvas.drawPicture(std::move(record));
  }

  // SkPngEncoder is skia's libpng-backed encoder, and this build does not have
  // it: chromium's skia/BUILD.gn compiles skia_encode_rust_png_srcs instead, so
  // the only SkPngEncoder in the tree is a header with no implementation behind
  // it. gfx::PNGCodec is the wrapper that knows which encoder is actually
  // there, //ui/gfx/codec is already in this binary's dependency graph, and its
  // return type is exactly what Render() returns.
  //
  // The surface is a raster one, so its pixels are already in memory and the
  // SkBitmap below only points at them; it stays valid because `surface`
  // outlives this scope.
  SkPixmap pixmap;
  if (!surface->peekPixels(&pixmap)) {
    return base::unexpected("could not read back the raster surface's pixels");
  }
  SkBitmap bitmap;
  if (!bitmap.installPixels(pixmap)) {
    return base::unexpected("could not wrap the rendered pixels as a bitmap");
  }
  if (stats) {
    stats->raster = base::TimeTicks::Now() - raster_started;
  }

  // Timed separately from the rest because it is the one phase whose cost the
  // caller can change without changing the page: a full-page PNG of a long
  // document can outweigh the layout that produced it, and `quality` or a
  // switch to JPEG is the answer. Folded into "render" it would look like the
  // document's fault.
  const base::TimeTicks encode_started = base::TimeTicks::Now();
  auto encoded = EncodeImage(bitmap, request);
  if (CaptureContext* context = CaptureContext::Current()) {
    context->stats().encode = base::TimeTicks::Now() - encode_started;
  }
  return encoded;
}

}  // namespace shot
