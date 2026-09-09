// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/timing/paint_timing.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#include "base/auto_reset.h"
#include "base/check_deref.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/web/web_performance_metrics_for_reporting.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/frame_request_callback_collection.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/interactive_detector.h"
#include "third_party/blink/renderer/core/loader/progress_tracker.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/paint/timing/image_paint_timing_detector.h"
#include "third_party/blink/renderer/core/paint/timing/largest_contentful_paint_manager.h"
#include "third_party/blink/renderer/core/paint/timing/paint_timing_client.h"
#include "third_party/blink/renderer/core/paint/timing/paint_timing_utils.h"
#include "third_party/blink/renderer/core/paint/timing/text_paint_timing_detector.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/timing/animation_frame_timing_info.h"
#include "third_party/blink/renderer/core/timing/dom_window_performance.h"
#include "third_party/blink/renderer/core/timing/performance_entry.h"
#include "third_party/blink/renderer/core/timing/performance_timing_for_reporting.h"
#include "third_party/blink/renderer/core/timing/soft_navigation_heuristics.h"
#include "third_party/blink/renderer/core/timing/window_performance.h"
#include "third_party/blink/renderer/platform/graphics/paint/ignore_paint_timing_scope.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_vector.h"
#include "third_party/blink/renderer/platform/heap/cross_thread_handle.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_scheduler.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/wtf.h"

namespace blink {

class RecodingTimeAfterBackForwardCacheRestoreFrameCallback
    : public FrameCallback {
 public:
  RecodingTimeAfterBackForwardCacheRestoreFrameCallback(
      PaintTiming* paint_timing,
      wtf_size_t record_index)
      : paint_timing_(paint_timing), record_index_(record_index) {}
  ~RecodingTimeAfterBackForwardCacheRestoreFrameCallback() override = default;

  void Invoke(double high_res_time_ms) override {
    // Instead of `high_res_time_ms`, use base::TimeTicks::Now() for consistency
    // and testability.
    paint_timing_->SetRequestAnimationFrameAfterBackForwardCacheRestore(
        record_index_, count_);

    count_++;
    if (count_ ==
        WebPerformanceMetricsForReporting::
            kRequestAnimationFramesToRecordAfterBackForwardCacheRestore) {
      paint_timing_->NotifyPaintTimingChanged();
      return;
    }

    if (auto* frame = paint_timing_->GetFrame()) {
      if (auto* document = frame->GetDocument()) {
        document->RequestAnimationFrame(this, FrameCallbackType::kInternal);
      }
    }
  }

  void Trace(Visitor* visitor) const override {
    visitor->Trace(paint_timing_);
    FrameCallback::Trace(visitor);
  }

 private:
  Member<PaintTiming> paint_timing_;
  const wtf_size_t record_index_;
  size_t count_ = 0;
};

// static
const char PaintTiming::kSupplementName[] = "PaintTiming";

// static
PaintTiming& PaintTiming::From(Document& document) {
  PaintTiming* timing = Supplement<Document>::From<PaintTiming>(document);
  if (!timing) {
    timing = MakeGarbageCollected<PaintTiming>(document);
    ProvideTo(document, timing);
  }
  return *timing;
}

// static
const PaintTiming* PaintTiming::From(const Document& document) {
  PaintTiming* timing = Supplement<Document>::From<PaintTiming>(document);
  return timing;
}

void PaintTiming::MarkFirstPaint() {
  // Test that |first_paint_| is non-zero here, as well as in
  // setFirstPaint, so we avoid invoking monotonicallyIncreasingTime() on every
  // call to markFirstPaint().
  PaintDetails& relevant_paint_details = GetRelevantPaintDetails();
  if (!relevant_paint_details.first_paint_.is_null()) {
    return;
  }
  DCHECK_EQ(IgnorePaintTimingScope::IgnoreDepth(), 0);
  SetFirstPaint(base::TimeTicks::Now());
}

void PaintTiming::MarkFirstContentfulPaint() {
  // Test that |first_contentful_paint_| is non-zero here, as
  // well as in SetFirstContentfulPaint, so we avoid invoking
  // MonotonicallyIncreasingTime() on every call to
  // MarkFirstContentfulPaint().
  PaintDetails& relevant_paint_details = GetRelevantPaintDetails();
  if (!relevant_paint_details.first_contentful_paint_.is_null()) {
    return;
  }
  if (IgnorePaintTimingScope::IgnoreDepth() > 0)
    return;
  SetFirstContentfulPaint(base::TimeTicks::Now());
}

void PaintTiming::MarkFirstImagePaint() {
  PaintDetails& relevant_paint_details = GetRelevantPaintDetails();
  if (!relevant_paint_details.first_image_paint_.is_null()) {
    return;
  }
  DCHECK_EQ(IgnorePaintTimingScope::IgnoreDepth(), 0);
  relevant_paint_details.first_image_paint_ = base::TimeTicks::Now();
  SetFirstContentfulPaint(relevant_paint_details.first_image_paint_);
}

void PaintTiming::MarkFirstEligibleToPaint() {
  if (!first_eligible_to_paint_.is_null())
    return;

  first_eligible_to_paint_ = base::TimeTicks::Now();
  NotifyPaintTimingChanged();
}

// We deliberately use |paint_details_.first_paint_| here rather than
// |paint_details_.first_paint_presentation_|, because
// eligibility is determined by synchronous CPU painting.
void PaintTiming::MarkIneligibleToPaint() {
  if (first_eligible_to_paint_.is_null() ||
      !paint_details_.first_paint_.is_null()) {
    return;
  }

  first_eligible_to_paint_ = base::TimeTicks();
  NotifyPaintTimingChanged();
}

void PaintTiming::NotifyPaint(bool is_first_paint,
                              bool text_painted,
                              bool image_painted) {
  if (IgnorePaintTimingScope::IgnoreDepth() > 0)
    return;

  if (is_first_paint)
    MarkFirstPaint();
  if (text_painted)
    MarkFirstContentfulPaint();
  if (image_painted)
    MarkFirstImagePaint();

  if (is_first_paint)
    GetFrame()->OnFirstPaint(text_painted, image_painted);
}

void PaintTiming::DiscardPresentationCallbacks() {
  // There is no display presentation in the static renderer. Drain callbacks
  // to preserve the detectors' end-of-paint bookkeeping without queuing Web
  // Performance entries for a frame that will never be presented.
  paint_timing_detector_->GetTextPaintTimingDetector()
      .TakeTextRecordsOnPaintFinished();
  paint_timing_detector_->GetImagePaintTimingDetector()
      .TakeImageRecordsOnPaintFinished();
  paint_timing_detector_->GetImagePaintTimingDetector()
      .TakeAnimatedImageRecordsOnPaintFinished();
}

void PaintTiming::Trace(Visitor* visitor) const {
  visitor->Trace(paint_timing_detector_);
  visitor->Trace(largest_contentful_paint_manager_);
  visitor->Trace(clients_);
  Supplement<Document>::Trace(visitor);
}

PaintTiming::PaintTiming(Document& document)
    : Supplement<Document>(document),
      paint_timing_detector_(MakeGarbageCollected<PaintTimingDetector>(this)) {
  // `window` will be null if `document` has already been shut down (frame
  // detach). Typically `PaintTiming` will be created before this, but this
  // isn't guaranteed since it's created lazily.
  if (LocalDOMWindow* window = document.domWindow()) {
    largest_contentful_paint_manager_ =
        MakeGarbageCollected<LargestContentfulPaintManager>(
            window);
    AddClient(largest_contentful_paint_manager_);
  }
}

LocalFrame* PaintTiming::GetFrame() const {
  return GetSupplementable()->GetFrame();
}

void PaintTiming::NotifyPaintTimingChanged() {
  paint_timing::NotifyLoaderPerformanceTimingChanged(GetSupplementable());
}

void PaintTiming::SetFirstPaint(base::TimeTicks stamp) {
  PaintDetails& relevant_paint_details = GetRelevantPaintDetails();
  if (!relevant_paint_details.first_paint_.is_null()) {
    return;
  }

  DCHECK_EQ(IgnorePaintTimingScope::IgnoreDepth(), 0);

  relevant_paint_details.first_paint_ = stamp;

    LocalFrame* frame = GetFrame();
    if (frame && frame->GetDocument()) {
      frame->GetDocument()->MarkFirstPaint();
    }

}

void PaintTiming::SetFirstContentfulPaint(base::TimeTicks stamp) {
  PaintDetails& relevant_paint_details = GetRelevantPaintDetails();
  if (!relevant_paint_details.first_contentful_paint_.is_null()) {
    return;
  }
  DCHECK_EQ(IgnorePaintTimingScope::IgnoreDepth(), 0);

  relevant_paint_details.first_contentful_paint_ = stamp;

  // This only happens in hard navigations.
  LocalFrame* frame = GetFrame();
  if (!frame) {
    return;
  }
  frame->View()->OnFirstContentfulPaint();

  if (frame->IsMainFrame() && frame->GetFrameScheduler()) {
    frame->GetFrameScheduler()->OnFirstContentfulPaintInMainFrame();
  }
  SetFirstPaint(stamp);
  NotifyPaintTimingChanged();
}

void PaintTiming::SetRequestAnimationFrameAfterBackForwardCacheRestore(
    wtf_size_t index,
    size_t count) {
  auto now = base::TimeTicks::Now();

  // The elements are allocated when the page is restored from the cache.
  DCHECK_LT(index,
            request_animation_frames_after_back_forward_cache_restore_.size());
  auto& current_rafs =
      request_animation_frames_after_back_forward_cache_restore_[index];
  DCHECK_LT(count, current_rafs.size());
  DCHECK_EQ(current_rafs[count], base::TimeTicks());
  current_rafs[count] = now;
}

void PaintTiming::OnRestoredFromBackForwardCache() {
  // Allocate the last element with 0, which indicates that the first paint
  // after this navigation doesn't happen yet.
  wtf_size_t index =
      first_paints_after_back_forward_cache_restore_presentation_.size();
  DCHECK_EQ(index,
            request_animation_frames_after_back_forward_cache_restore_.size());

  first_paints_after_back_forward_cache_restore_presentation_.push_back(
      base::TimeTicks());

  request_animation_frames_after_back_forward_cache_restore_.push_back(
      RequestAnimationFrameTimesAfterBackForwardCacheRestore{});

  LocalFrame* frame = GetFrame();
  if (!frame->IsOutermostMainFrame()) {
    return;
  }

  Document* document = frame->GetDocument();
  DCHECK(document);

  // Cancel if there is already a registered callback.
  if (raf_after_bfcache_restore_measurement_callback_id_) {
    document->CancelAnimationFrame(
        raf_after_bfcache_restore_measurement_callback_id_,
        FrameCallbackType::kInternal);
    raf_after_bfcache_restore_measurement_callback_id_ = 0;
  }

  raf_after_bfcache_restore_measurement_callback_id_ =
      document->RequestAnimationFrame(
          MakeGarbageCollected<
              RecodingTimeAfterBackForwardCacheRestoreFrameCallback>(this,
                                                                     index),
          FrameCallbackType::kInternal);
}

void PaintTiming::NotifyPaintFinished() {
  DOMWindowPerformance::performance(CHECK_DEREF(GetDocument()->domWindow()))
      ->OnPaintFinished();
  paint_timing_detector_->NotifyPaintFinished();

  ForEachClient([](PaintTimingClient* client) { client->OnPaintFinished(); });

  DiscardPresentationCallbacks();
}

void PaintTiming::NotifyInputEvent(WebInputEvent::Type type) {
  // A single keyup event should be ignored. It could be caused by user actions
  // such as refreshing via Ctrl+R.
  if (type == WebInputEvent::Type::kMouseMove ||
      type == WebInputEvent::Type::kMouseEnter ||
      type == WebInputEvent::Type::kMouseLeave ||
      type == WebInputEvent::Type::kKeyUp ||
      WebInputEvent::IsPinchGestureEventType(type)) {
    return;
  }
  OnInputOrScroll();
}

void PaintTiming::NotifyScroll(mojom::blink::ScrollType scroll_type) {
  if (scroll_type != mojom::blink::ScrollType::kUser &&
      scroll_type != mojom::blink::ScrollType::kCompositor) {
    return;
  }
  OnInputOrScroll();
}

void PaintTiming::OnInputOrScroll() {
  ForEachClient([](PaintTimingClient* client) { client->OnInputOrScroll(); });

  // `largest_contentful_paint_manager_` will be non-null as long as first input
  // has not occurred and this object wasn't created while detached (in which
  // case the associated frame cannot be targeted for input).
  if (!largest_contentful_paint_manager_) {
    return;
  }

  RemoveClient(largest_contentful_paint_manager_);
  largest_contentful_paint_manager_ = nullptr;

  // Notify the metrics layer of the timestamp so it can determine which records
  // are valid.
  DOMWindowPerformance::performance(
      CHECK_DEREF(GetSupplementable()->domWindow()))
      ->timingForReporting()
      ->SetFirstInputOrScrollNotifiedTimestamp(base::TimeTicks::Now());
  paint_timing::NotifyLoaderPerformanceTimingChanged(GetSupplementable());
}

void PaintTiming::AddClient(PaintTimingClient* client) {
  CHECK(allow_client_modifications_);
  DCHECK(!clients_.Contains(client));
  clients_.push_back(client);
}

void PaintTiming::RemoveClient(PaintTimingClient* client) {
  CHECK(allow_client_modifications_);
  wtf_size_t count =
      EraseIf(clients_, [&](const auto& c) { return c == client; });
  CHECK_EQ(count, 1u);
}

void PaintTiming::ForEachClient(
    base::FunctionRef<void(PaintTimingClient*)> callback) {
  base::AutoReset<bool> scope(&allow_client_modifications_, false);
  for (PaintTimingClient* client : clients_) {
    callback(client);
  }
}

}  // namespace blink
