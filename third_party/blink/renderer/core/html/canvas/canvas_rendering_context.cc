/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/canvas/canvas_rendering_context.h"

#include "base/byte_size.h"
#include "base/strings/stringprintf.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_union_element_elementimage.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_context_creation_attributes_core.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_image_source.h"
#include "third_party/blink/renderer/core/html/canvas/element_image.h"
#include "third_party/blink/renderer/core/html/canvas/html_canvas_accessibility_manager.h"
#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/paint/cull_rect_updater.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_layer_painter.h"
#include "third_party/blink/renderer/platform/graphics/accelerated_static_bitmap_image.h"
#include "third_party/blink/renderer/platform/graphics/canvas_non_2d_resource_provider.h"
#include "third_party/blink/renderer/platform/graphics/gpu/shared_gpu_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_canvas.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_record_builder.h"
#include "third_party/blink/renderer/platform/graphics/unaccelerated_static_bitmap_image.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/size_conversions.h"

namespace blink {
namespace {
BASE_FEATURE(kAllowAcceleratedTexElement, base::FEATURE_ENABLED_BY_DEFAULT);

// Used to hold kAccessibilityUkmRecordingDelay, the delay for the
// now-deleted scheduled UKM recording path in
// MaybeRecordUKMCanvasAccessibility() -- see that function for why it's
// gone.
}

CanvasRenderingContext::CanvasRenderingContext(
    CanvasRenderingContextHost* host,
    const CanvasContextCreationAttributesCore& attrs,
    CanvasRenderingAPI canvas_rendering_API)
    : ActiveScriptWrappable<CanvasRenderingContext>({}),
      host_(host),
      creation_attributes_(attrs),
      canvas_rendering_type_(canvas_rendering_API) {
  // The following check is for investigating crbug.com/1470622
  // If the crash stops happening in CanvasRenderingContext2D::
  // GetOrCreatePaintCanvas(), and starts happening here instead,
  // then we'll know that the bug is related to creation and the
  // new crash reports pointing to this location will provide more
  // actionable feedback on how to fix the issue. If the crash
  // continues to happen at the old location, then we'll know that
  // the problem has to do with a pre-finalizer being called
  // prematurely.
  CHECK(host_);
}

void CanvasRenderingContext::Dispose() {
  RenderTaskEnded();

  // Used to flush a pending delayed UKM recording here if
  // MaybeRecordUKMCanvasAccessibility() had scheduled one via
  // did_schedule_accessibility_ukm_recording_. That scheduling path is
  // gone (see MaybeRecordUKMCanvasAccessibility()) -- recording now always
  // happens synchronously, so there is never a pending one to flush here.

  // HTMLCanvasElement and CanvasRenderingContext have a circular reference.
  // When the pair is no longer reachable, their destruction order is non-
  // deterministic, so the first of the two to be destroyed needs to notify
  // the other in order to break the circular reference.  This is to avoid
  // an error when CanvasRenderingContext::DidProcessTask() is invoked
  // after the HTMLCanvasElement is destroyed.
  if (CanvasRenderingContextHost* host = Host()) [[likely]] {
    host->DetachContext();
    host_ = nullptr;
  }
}

// static
CanvasRenderingContext*
CanvasRenderingContext::GetEnclosingContextForDrawElement(
    Element* element,
    const String& func_name,
    ExceptionState& exception_state) {
  auto* canvas = DynamicTo<HTMLCanvasElement>(element->parentNode());
  if (!canvas) {
    exception_state.ThrowTypeError(StrCat(
        {"Only immediate children of the <canvas> element can be passed to ",
         func_name, "."}));
    return nullptr;
  }
  CanvasRenderingContext* context = canvas->RenderingContext();
  if (!context) {
    exception_state.ThrowTypeError(StrCat(
        {func_name, ": containing canvas does not have a rendering context."}));
    return nullptr;
  }
  if (!context->IsDrawElementImageEligible(element, func_name,
                                           exception_state)) {
    return nullptr;
  }
  return context;
}

bool CanvasRenderingContext::IsDrawElementImageEligible(
    Element* element,
    const String& func_name,
    ExceptionState& exception_state) {
  // Used to also reject Host()->IsOffscreenCanvas() here (elements can't be
  // drawn into an OffscreenCanvas). OffscreenCanvas
  // (third_party/blink/renderer/modules/offscreencanvas/) went with
  // modules, and HTMLCanvasElement is the only CanvasRenderingContextHost
  // subclass left, so IsOffscreenCanvas() can never be true.
  if (!Host()) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kInvalidStateError,
        "Elements cannot be drawn into an OffscreenCanvas.");
    return false;
  }

  HTMLCanvasElement* canvas_element = static_cast<HTMLCanvasElement*>(Host());
  if (!canvas_element || !canvas_element->GetDocument().View()) {
    return false;
  }

  return canvas_element->VerifyDrawElementImageEligibility(element, func_name,
                                                           exception_state);
}

bool CanvasRenderingContext::IsDrawElementImageEligible(
    const V8UnionElementOrElementImage* element_or_image,
    const String& func_name,
    ExceptionState& exception_state) {
  if (element_or_image->IsElement()) {
    return IsDrawElementImageEligible(element_or_image->GetAsElement(),
                                      func_name, exception_state);
  }

  const auto& record = element_or_image->GetAsElementImage()->PaintRecord();
  if (!record) {
    exception_state.ThrowDOMException(DOMExceptionCode::kInvalidStateError,
                                      "The ElementImage has been closed.");
    return false;
  }

  DOMNodeId current_canvas_node_id = kInvalidDOMNodeId;
  if (Host()) {
    // Used to branch here on Host()->IsOffscreenCanvas() and, when true,
    // read the id from an OffscreenCanvas host instead. OffscreenCanvas
    // (third_party/blink/renderer/modules/offscreencanvas/) went with
    // modules, and HTMLCanvasElement is now the only
    // CanvasRenderingContextHost subclass left -- it always constructs with
    // HostType::kCanvasHost, so IsOffscreenCanvas() can never be true here.
    current_canvas_node_id =
        static_cast<HTMLCanvasElement*>(Host())->GetDomNodeId();
  }

  if (current_canvas_node_id == kInvalidDOMNodeId ||
      record->paint_state.canvas_node_id != current_canvas_node_id) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kInvalidStateError,
        "The source was captured from a different canvas.");
    return false;
  }
  return true;
}

std::optional<CanvasChildPaintRecord>
CanvasRenderingContext::GetChildPaintRecord(Element* element) {
  return Host()->GetCanvasChildPaintRecord(element->GetDomNodeId());
}

scoped_refptr<StaticBitmapImage> CanvasRenderingContext::GetElementImage(
    const V8UnionElementOrElementImage* element,
    std::optional<float> sx,
    std::optional<float> sy,
    std::optional<float> swidth,
    std::optional<float> sheight,
    std::optional<uint32_t> width,
    std::optional<uint32_t> height,
    gpu::SharedImageUsageSet usage,
    const String& func_name,
    ExceptionState& exception_state) {
  if (!IsDrawElementImageEligible(element, func_name, exception_state)) {
    return nullptr;
  }

  std::optional<CanvasChildPaintRecord> child_paint_record;
  if (element->IsElement()) {
    child_paint_record = GetChildPaintRecord(element->GetAsElement());
  } else {
    if (const auto& record = element->GetAsElementImage()->PaintRecord()) {
      child_paint_record = *record;
    }
  }

  if (!child_paint_record) {
    if (element->IsElementImage()) {
      exception_state.ThrowDOMException(DOMExceptionCode::kInvalidStateError,
                                        "The ElementImage has been closed.");
    } else {
      exception_state.ThrowDOMException(DOMExceptionCode::kInvalidStateError,
                                        "No cached paint record for element.");
    }
    return nullptr;
  }

  // Element size in physical coordinates.
  gfx::RectF src_rect(child_paint_record->paint_state.box_size);
  if (sx && sy && swidth && sheight) {
    float dpr = child_paint_record->paint_state.effective_zoom;
    src_rect = gfx::RectF(*sx * dpr, *sy * dpr, *swidth * dpr, *sheight * dpr);
  }

  // The default destination size for GetElementImage is the source content
  // size scaled to canvas grid coordinates. This causes the element to have
  // the same proportions when appearing inside the canvas as it would have
  // were it painted outside the canvas.
  gfx::SizeF intrinsic_size(src_rect.size());
  gfx::Vector2dF canvas_scale =
      GetCanvasGridScaleFactor(child_paint_record->paint_state, Host()->Size());
  intrinsic_size.Scale(canvas_scale.x(), canvas_scale.y());
  gfx::Size intrinsic_dest_size = gfx::ToCeiledSize(intrinsic_size);
  gfx::Size dest_size(intrinsic_dest_size);
  if (width && height) {
    dest_size = gfx::Size(width.value(), height.value());
    canvas_scale.Scale(
        static_cast<float>(dest_size.width()) / intrinsic_dest_size.width(),
        static_cast<float>(dest_size.height()) / intrinsic_dest_size.height());
  }
  if (dest_size.IsEmpty()) {
    return nullptr;
  }

  auto draw_to_canvas = [&](cc::PaintCanvas& canvas) {
    canvas.scale(canvas_scale.x(), canvas_scale.y());
    canvas.translate(-src_rect.x(), -src_rect.y());
    canvas.drawPicture(child_paint_record->record);
  };

  if (base::FeatureList::IsEnabled(kAllowAcceleratedTexElement) &&
      SharedGpuContext::IsGpuCompositingEnabled()) {
    if (auto wrapper = SharedGpuContext::ContextProviderWrapper()) {
      auto resource_provider = CanvasNon2DResourceProvider::Create(
          dest_size, GetN32FormatForCanvas(), kPremul_SkAlphaType,
          gfx::ColorSpace::CreateSRGB(), gfx::HDRMetadata(), wrapper,
          gpu::SHARED_IMAGE_USAGE_RASTER_WRITE | usage);

      // GetOrCreateImageProvider() to make sure one is created prior to the
      // call to SetAnimatedImageFrameIndexMaps().
      resource_provider->GetOrCreateImageProvider();
      resource_provider->SetAnimatedImageFrameIndexes(
          child_paint_record->paint_state.animated_image_frame_index_map);

      return resource_provider->DoExternalOverdrawAndSnapshot(
          [&](cc::PaintCanvas& canvas) { draw_to_canvas(canvas); },
          ImageOrientation());
    }
  }

  return UnacceleratedStaticBitmapImage::CreateFromRaster(
      dest_size, draw_to_canvas,
      child_paint_record->paint_state.animated_image_frame_index_map);
}

void CanvasRenderingContext::DidDraw(
    const gfx::Rect& dirty_rect,
    CanvasPerformanceMonitor::DrawType draw_type) {
  CanvasRenderingContextHost* const host = Host();
  host->DidDraw(dirty_rect);

  did_draw_text_ |= (draw_type == CanvasPerformanceMonitor::DrawType::kText);

  auto& monitor = GetCanvasPerformanceMonitor();
  monitor.DidDraw(draw_type);
  if (did_draw_in_current_task_)
    return;

  monitor.CurrentTaskDrawsToContext(this);
  did_draw_in_current_task_ = true;
  // We need to store whether the document is being printed because the
  // document may exit printing state by the time DidProcessTask is called.
  // This is an issue with beforeprint event listeners.
  did_print_in_current_task_ |= host->IsPrinting();
  Thread::Current()->AddTaskObserver(this);
}

void CanvasRenderingContext::DidProcessTask(
    const base::PendingTask& /* pending_task */) {
  RenderTaskEnded();

  // The end of a script task that drew content to the canvas is the point
  // at which the current frame may be considered complete.
  FlushReason reason = did_print_in_current_task_
                           ? FlushReason::kCanvasPushFrameWhilePrinting
                           : FlushReason::kCanvasPushFrame;
  FinalizeFrame(reason);
  did_print_in_current_task_ = false;
  if (CanvasRenderingContextHost* host = Host()) [[likely]] {
    host->PostFinalizeFrame(reason);
  }

  did_process_task_ = true;
  MaybeRecordUKMCanvasAccessibility();
}

void CanvasRenderingContext::MaybeRecordUKMCanvasAccessibility() {
  if (did_record_accessibility_ukm_ || !did_process_task_) {
    return;
  }

  // Used to gate this on
  // base::FeatureList::IsEnabled(
  //     ::features::kEnableCollectAccessibilityHeuristicInCanvasUkm), a flag
  // declared in ui/accessibility/accessibility_features.h and, when
  // enabled, delay recording by posting a task so the accessibility
  // heuristic (GetNeedsAccessibilitySupportHeuristic(), which needs a
  // LayoutObject-driven accessibility pass) has time to settle first.
  // ui/accessibility is gone, so that flag can never exist/be enabled, and
  // heuristic collection is unconditionally disabled -- meaning the UKM is
  // always recorded immediately, which is what the code below now always
  // does.
  RecordUKMCanvasAccessibility();
}

void CanvasRenderingContext::RecordUKMCanvasAccessibility() {
  if (did_record_accessibility_ukm_ || !did_process_task_) {
    return;
  }

  CanvasRenderingContextHost* const host = Host();
  if (!host) {
    return;
  }

  bool has_keyboard_listener = false;
  bool has_mouse_listener = false;
  // Used to branch here on host->IsOffscreenCanvas() and, if true, collect
  // only offscreen canvases that push their rendered output to a
  // placeholder element in DOM (via
  // static_cast<OffscreenCanvas*>(host)->HasPlaceholderCanvas()).
  // OffscreenCanvas went with modules, and HTMLCanvasElement is the only
  // CanvasRenderingContextHost subclass left, so is_offscreen is always
  // false and that branch is gone; the non-offscreen path below always
  // runs.
  const bool is_offscreen = false;
  HTMLCanvasElement* canvas_element = static_cast<HTMLCanvasElement*>(host);
  // Non offscreen canvases are only collected if they are visible.
  if (!canvas_element->IsDisplayed()) {
    return;
  }

  DEFINE_STATIC_LOCAL(
      const Vector<AtomicString>, keyboard_event_types,
      ({event_type_names::kKeydown, event_type_names::kKeypress,
        event_type_names::kKeyup}));
  DEFINE_STATIC_LOCAL(
      const Vector<AtomicString>, mouse_event_types,
      ({event_type_names::kClick, event_type_names::kMousedown,
        event_type_names::kMouseup, event_type_names::kMousemove,
        event_type_names::kMouseover, event_type_names::kMouseout}));

  has_keyboard_listener =
      canvas_element->HasAnyEventListeners(keyboard_event_types);
  has_mouse_listener =
      canvas_element->HasAnyEventListeners(mouse_event_types);

  const auto& ukm_params = host->GetUkmParameters();
  auto ukm_builder = ukm::builders::Accessibility_Canvas(ukm_params.source_id);
  ukm_builder.SetRenderingContext(static_cast<int>(canvas_rendering_type_))
      .SetIsOffscreen(is_offscreen)
      .SetHasKeyboardListener(has_keyboard_listener)
      .SetHasMouseListener(has_mouse_listener)
      .SetHasText(did_draw_text_);

  // Used to also gate populating SetNeedsAccessibilitySupport() on
  // !is_offscreen &&
  // base::FeatureList::IsEnabled(
  //     ::features::kEnableCollectAccessibilityHeuristicInCanvasUkm).
  // is_offscreen is now always false (see above) and the feature flag no
  // longer exists (ui/accessibility is gone), so this was simplified the
  // same way as MaybeRecordUKMCanvasAccessibility() above: heuristic
  // collection is unconditionally disabled, so the field is never set.

  ukm_builder.Record(ukm_params.ukm_recorder);

  did_record_accessibility_ukm_ = true;
}

void CanvasRenderingContext::RecordUMACanvasRenderingAPI() {
  const CanvasRenderingContextHost* const host = Host();
  if (auto* window =
          DynamicTo<LocalDOMWindow>(host->GetTopExecutionContext())) {
    // Used to branch here on host->IsOffscreenCanvas() and record an
    // OffscreenCanvas_* WebFeature instead. OffscreenCanvas went with
    // modules, and HTMLCanvasElement is the only CanvasRenderingContextHost
    // subclass left, so that branch is gone; only the HTMLCanvasElement_*
    // counters can ever fire now.
    WebFeature feature;
    switch (canvas_rendering_type_) {
      case CanvasRenderingContext::CanvasRenderingAPI::k2D:
        feature = WebFeature::kHTMLCanvasElement_2D;
        break;
      case CanvasRenderingContext::CanvasRenderingAPI::kWebgl:
        feature = WebFeature::kHTMLCanvasElement_WebGL;
        break;
      case CanvasRenderingContext::CanvasRenderingAPI::kWebgl2:
        feature = WebFeature::kHTMLCanvasElement_WebGL2;
        break;
      case CanvasRenderingContext::CanvasRenderingAPI::kBitmaprenderer:
        feature = WebFeature::kHTMLCanvasElement_BitmapRenderer;
        break;
      case CanvasRenderingContext::CanvasRenderingAPI::kWebgpu:
        feature = WebFeature::kHTMLCanvasElement_WebGPU;
        break;
      default:
        NOTREACHED();
    }
    UseCounter::Count(window->document(), feature);
  }
}

void CanvasRenderingContext::RecordUKMCanvasRenderingAPI() {
  CanvasRenderingContextHost* const host = Host();
  DCHECK(host);
  const auto& ukm_params = host->GetUkmParameters();
  // Used to branch here on host->IsOffscreenCanvas() and record
  // SetOffscreenCanvas_RenderingContext instead. OffscreenCanvas went with
  // modules, and HTMLCanvasElement is the only CanvasRenderingContextHost
  // subclass left, so that branch is gone.
  ukm::builders::ClientRenderingAPI(ukm_params.source_id)
      .SetCanvas_RenderingContext(static_cast<int>(canvas_rendering_type_))
      .Record(ukm_params.ukm_recorder);
}

void CanvasRenderingContext::RecordUKMCanvasDrawnToRenderingAPI() {
  CanvasRenderingContextHost* const host = Host();
  DCHECK(host);
  const auto& ukm_params = host->GetUkmParameters();
  // Used to branch here on host->IsOffscreenCanvas() and record
  // SetOffscreenCanvas_RenderingContextDrawnTo instead. OffscreenCanvas
  // went with modules, and HTMLCanvasElement is the only
  // CanvasRenderingContextHost subclass left, so that branch is gone.
  ukm::builders::ClientRenderingAPI(ukm_params.source_id)
      .SetCanvas_RenderingContextDrawnTo(
          static_cast<int>(canvas_rendering_type_))
      .Record(ukm_params.ukm_recorder);
}

CanvasRenderingContext::CanvasRenderingAPI
CanvasRenderingContext::RenderingAPIFromId(const String& id) {
  if (id == "2d") {
    return CanvasRenderingAPI::k2D;
  }
  if (id == "experimental-webgl") {
    return CanvasRenderingAPI::kWebgl;
  }
  if (id == "webgl") {
    return CanvasRenderingAPI::kWebgl;
  }
  if (id == "webgl2") {
    return CanvasRenderingAPI::kWebgl2;
  }
  if (id == "bitmaprenderer") {
    return CanvasRenderingAPI::kBitmaprenderer;
  }
  if (id == "webgpu") {
    return CanvasRenderingAPI::kWebgpu;
  }
  return CanvasRenderingAPI::kUnknown;
}

void CanvasRenderingContext::Trace(Visitor* visitor) const {
  visitor->Trace(host_);
  ActiveScriptWrappable::Trace(visitor);
}

void CanvasRenderingContext::RenderTaskEnded() {
  if (!did_draw_in_current_task_)
    return;

  Thread::Current()->RemoveTaskObserver(this);
  did_draw_in_current_task_ = false;
}

CanvasPerformanceMonitor&
CanvasRenderingContext::GetCanvasPerformanceMonitor() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<CanvasPerformanceMonitor>,
                                  monitor, ());
  return *monitor;
}

}  // namespace blink
