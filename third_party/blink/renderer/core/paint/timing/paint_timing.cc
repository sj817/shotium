// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/timing/paint_timing.h"

#include <memory>
#include <utility>

#include "base/check_deref.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
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
  pending_paint_events_.insert(PaintEvent::kFirstPaint);
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
  Mark(PaintEvent::kFirstImagePaint);
}

void PaintTiming::MarkFirstEligibleToPaint() {
  if (!first_eligible_to_paint_.is_null())
    return;

  first_eligible_to_paint_ = base::TimeTicks::Now();
  NotifyPaintTimingChanged();
}

// We deliberately use |paint_details_.first_paint_| here rather than
// |paint_details_.first_paint_presentation_|, because
// |paint_details_.first_paint_presentation_| is set asynchronously and we need
// to be able to rely on a synchronous check that SetFirstPaintPresentation
// hasn't been scheduled or run.
void PaintTiming::MarkIneligibleToPaint() {
  if (first_eligible_to_paint_.is_null() ||
      !paint_details_.first_paint_.is_null()) {
    return;
  }

  first_eligible_to_paint_ = base::TimeTicks();
  NotifyPaintTimingChanged();
}

void PaintTiming::SetFirstMeaningfulPaintCandidate(base::TimeTicks timestamp) {
  if (!first_meaningful_paint_candidate_.is_null())
    return;
  first_meaningful_paint_candidate_ = timestamp;
  if (GetFrame() && GetFrame()->View() && !GetFrame()->View()->IsAttached()) {
    GetFrame()->GetFrameScheduler()->OnFirstMeaningfulPaint();
  }
}

void PaintTiming::SetFirstMeaningfulPaint(
    base::TimeTicks presentation_time,
    FirstMeaningfulPaintDetector::HadUserInput had_input) {
  DCHECK(first_meaningful_paint_presentation_.is_null());
  DCHECK(!presentation_time.is_null());

  TRACE_EVENT_MARK_WITH_TIMESTAMP2("loading,rail,devtools.timeline",
                                   "firstMeaningfulPaint", presentation_time,
                                   "frame", GetFrameIdForTracing(GetFrame()),
                                   "afterUserInput", had_input);

  // Notify FMP for UMA only if there's no user input before FMP, so that layout
  // changes caused by user interactions wouldn't be considered as FMP.
  if (had_input == FirstMeaningfulPaintDetector::kNoUserInput) {
    first_meaningful_paint_presentation_ = presentation_time;
    NotifyPaintTimingChanged();
  }
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
  fmp_detector_->NotifyPaint();

  if (is_first_paint)
    GetFrame()->OnFirstPaint(text_painted, image_painted);
}

void PaintTiming::DiscardPresentationCallbacks() {
  // There is no display presentation in the static renderer. Drain callbacks
  // to preserve the detectors' end-of-paint bookkeeping without queuing Web
  // Performance entries for a frame that will never be presented.
  paint_timing_detector_->GetTextPaintTimingDetector().TakePaintTimingCallback();
  paint_timing_detector_->GetImagePaintTimingDetector().TakePaintTimingCallback();
  pending_paint_events_.clear();
}

void PaintTiming::Trace(Visitor* visitor) const {
  visitor->Trace(paint_timing_detector_);
  visitor->Trace(fmp_detector_);
  visitor->Trace(largest_contentful_paint_manager_);
  Supplement<Document>::Trace(visitor);
}

PaintTiming::PaintTiming(Document& document)
    : Supplement<Document>(document),
      paint_timing_detector_(MakeGarbageCollected<PaintTimingDetector>(this)),
      fmp_detector_(MakeGarbageCollected<FirstMeaningfulPaintDetector>(this)) {
  // `window` will be null if `document` has already been shut down (frame
  // detach). Typically `PaintTiming` will be created before this, but this
  // isn't guaranteed since it's created lazily.
  if (LocalDOMWindow* window = document.domWindow()) {
    largest_contentful_paint_manager_ =
        MakeGarbageCollected<LargestContentfulPaintManager>(
            window);
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

  pending_paint_events_.insert(PaintEvent::kFirstPaint);
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
  Mark(PaintEvent::kFirstContentfulPaint);
  NotifyPaintTimingChanged();
}

void PaintTiming::Mark(PaintEvent event) {
  pending_paint_events_.insert(event);
}

void PaintTiming::
    RegisterNotifyFirstPaintAfterBackForwardCacheRestorePresentationTime(
        wtf_size_t index) {
  RegisterNotifyPresentationTime(
      BindOnce(&PaintTiming::
                   ReportFirstPaintAfterBackForwardCacheRestorePresentationTime,
               WrapWeakPersistent(this), index));
}

void PaintTiming::RegisterNotifyPresentationTime(ReportTimeCallback callback) {
  // ReportPresentationTime will queue a presentation-promise, the callback is
  // called when the compositor submission of the current render frame completes
  // or fails to happen.
  if (!GetFrame() || !GetFrame()->GetPage()) {
    return;
  }

  GetFrame()->GetPage()->GetChromeClient().NotifyPresentationTime(
      *GetFrame(), std::move(callback));
}

void PaintTiming::ReportPresentationTime(
    PaintEvent event,
    base::TimeTicks rendering_update_end_time,
    const viz::FrameTimingDetails& presentation_details) {
  CHECK(IsMainThread());
  base::TimeTicks timestamp =
      presentation_details.presentation_feedback.timestamp;

  switch (event) {
    case PaintEvent::kFirstPaint:
      SetFirstPaintPresentation(
          PaintTimingInfo{rendering_update_end_time, timestamp});
      return;
    case PaintEvent::kFirstContentfulPaint:
      SetFirstContentfulPaintPresentation(
          PaintTimingInfo{rendering_update_end_time, timestamp});
      RecordFirstContentfulPaintTimingMetrics(presentation_details);
      return;
    case PaintEvent::kFirstImagePaint:
      SetFirstImagePaintPresentation(timestamp);
      return;
    default:
      NOTREACHED();
  }
}

void PaintTiming::RecordFirstContentfulPaintTimingMetrics(
    const viz::FrameTimingDetails& frame_timing_details) {
  if (frame_timing_details.received_compositor_frame_timestamp ==
          base::TimeTicks() ||
      frame_timing_details.embedded_frame_timestamp == base::TimeTicks()) {
    return;
  }
  bool frame_submitted_before_embed =
      (frame_timing_details.received_compositor_frame_timestamp <
       frame_timing_details.embedded_frame_timestamp);
  base::UmaHistogramBoolean("Navigation.FCPFrameSubmittedBeforeSurfaceEmbed",
                            frame_submitted_before_embed);

  if (frame_submitted_before_embed) {
    base::UmaHistogramCustomTimes(
        "Navigation.FCPFrameSubmissionToSurfaceEmbed",
        frame_timing_details.embedded_frame_timestamp -
            frame_timing_details.received_compositor_frame_timestamp,
        base::Milliseconds(1), base::Minutes(3), 50);
  } else {
    base::UmaHistogramCustomTimes(
        "Navigation.SurfaceEmbedToFCPFrameSubmission",
        frame_timing_details.received_compositor_frame_timestamp -
            frame_timing_details.embedded_frame_timestamp,
        base::Milliseconds(1), base::Minutes(3), 50);
  }
}

void PaintTiming::ReportFirstPaintAfterBackForwardCacheRestorePresentationTime(
    wtf_size_t index,
    const viz::FrameTimingDetails& presentation_details) {
  CHECK(IsMainThread());
  SetFirstPaintAfterBackForwardCacheRestorePresentation(
      presentation_details.presentation_feedback.timestamp, index);
}

void PaintTiming::SetFirstPaintPresentation(
    const PaintTimingInfo& paint_timing_info) {
  PaintDetails& relevant_paint_details = GetRelevantPaintDetails();
  DCHECK(relevant_paint_details.first_paint_presentation_.is_null());
  relevant_paint_details.first_paint_presentation_ =
      paint_timing_info.presentation_time;
  if (first_paint_presentation_for_ukm_.is_null()) {
    first_paint_presentation_for_ukm_ = paint_timing_info.presentation_time;
  }
  probe::PaintTiming(
      GetSupplementable(), "firstPaint",
      relevant_paint_details.first_paint_presentation_.since_origin()
          .InSecondsF());
  NotifyPaintTimingChanged();
}

void PaintTiming::SetFirstContentfulPaintPresentation(
    const PaintTimingInfo& paint_timing_info) {
  PaintDetails& relevant_paint_details = GetRelevantPaintDetails();
  DCHECK(relevant_paint_details.first_contentful_paint_presentation_.is_null());
  TRACE_EVENT_INSTANT_WITH_TIMESTAMP0(
      "benchmark,loading", "GlobalFirstContentfulPaint",
      TRACE_EVENT_SCOPE_GLOBAL, paint_timing_info.presentation_time);
  relevant_paint_details.first_contentful_paint_presentation_ =
      paint_timing_info.presentation_time;
  CHECK(first_contentful_paint_presentation_.is_null());
  first_contentful_paint_presentation_ = paint_timing_info.presentation_time;
  probe::PaintTiming(
      GetSupplementable(), "firstContentfulPaint",
      relevant_paint_details.first_contentful_paint_presentation_.since_origin()
          .InSecondsF());

  NotifyPaintTimingChanged();
  fmp_detector_->NotifyFirstContentfulPaint(
      paint_details_.first_contentful_paint_presentation_);
  InteractiveDetector* interactive_detector =
      InteractiveDetector::From(*GetSupplementable());
  if (interactive_detector) {
    interactive_detector->OnFirstContentfulPaint(
        paint_details_.first_contentful_paint_presentation_);
  }

  if (GetFrame()) {
    GetFrame()->OnFirstContentfulPaint(paint_timing_info.presentation_time);
    GetFrame()->Loader().Progress().DidFirstContentfulPaint();

    // First contentful paint used to be reported to the browser's performance
    // manager here. OnFirstContentfulPaint() above is Blink bookkeeping;
    // Shot waits for document load completion and resource requests instead.
  }
}

void PaintTiming::SetFirstImagePaintPresentation(base::TimeTicks stamp) {
  PaintDetails& relevant_paint_details = GetRelevantPaintDetails();
  DCHECK(relevant_paint_details.first_image_paint_presentation_.is_null());
  relevant_paint_details.first_image_paint_presentation_ = stamp;
  probe::PaintTiming(
      GetSupplementable(), "firstImagePaint",
      relevant_paint_details.first_image_paint_presentation_.since_origin()
          .InSecondsF());
  NotifyPaintTimingChanged();
}

void PaintTiming::SetFirstPaintAfterBackForwardCacheRestorePresentation(
    base::TimeTicks stamp,
    wtf_size_t index) {
  // The elements are allocated when the page is restored from the cache.
  DCHECK_GE(first_paints_after_back_forward_cache_restore_presentation_.size(),
            index);
  DCHECK(first_paints_after_back_forward_cache_restore_presentation_[index]
             .is_null());
  first_paints_after_back_forward_cache_restore_presentation_[index] = stamp;
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
  RegisterNotifyFirstPaintAfterBackForwardCacheRestorePresentationTime(index);

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
  paint_timing_detector_->NotifyPaintFinished();
  // We should never be painting detached frames.
  CHECK(GetFrame());
  LocalDOMWindow* window = GetFrame()->DomWindow();
  CHECK(window);
  DOMWindowPerformance::performance(*window)->OnPaintFinished();
  if (auto* heuristics = window->GetSoftNavigationHeuristics()) {
    heuristics->OnPaintFinished();
  }

  DiscardPresentationCallbacks();
}

void PaintTiming::OnInputOrScroll() {
  // `largest_contentful_paint_manager_` will be non-null as long as first input
  // has not occurred and this object wasn't created while detached (in which
  // case the associated frame cannot be targeted for input).
  if (!largest_contentful_paint_manager_) {
    return;
  }
  // LCP stops recording on first input or scroll.
  largest_contentful_paint_manager_->OnFirstInputOrScroll();
  largest_contentful_paint_manager_ = nullptr;

  // Notify the metrics layer of the timestamp so it can determine which records
  // are valid.
  DOMWindowPerformance::performance(
      CHECK_DEREF(GetSupplementable()->domWindow()))
      ->timingForReporting()
      ->SetFirstInputOrScrollNotifiedTimestamp(base::TimeTicks::Now());
  paint_timing::NotifyLoaderPerformanceTimingChanged(GetSupplementable());
}

}  // namespace blink
