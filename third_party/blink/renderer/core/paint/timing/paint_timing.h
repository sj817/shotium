// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_TIMING_PAINT_TIMING_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_TIMING_PAINT_TIMING_H_

#include <array>
#include <memory>

#include "base/functional/function_ref.h"
#include "base/gtest_prod_util.h"
#include "base/time/time.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/web/web_performance_metrics_for_reporting.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/paint/timing/paint_timing_callbacks.h"
#include "third_party/blink/renderer/core/timing/animation_frame_timing_info.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_vector.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {
class AnimationFrameTimingInfo;
struct DOMPaintTimingInfo;
class LargestContentfulPaintManager;
class LocalFrame;
class PaintTimingClient;
class PaintTimingDetector;

// PaintTiming is responsible for tracking paint-related timings for a given
// document.
class CORE_EXPORT PaintTiming final : public GarbageCollected<PaintTiming>,
                                      public Supplement<Document> {
  using RequestAnimationFrameTimesAfterBackForwardCacheRestore = std::array<
      base::TimeTicks,
      WebPerformanceMetricsForReporting::
          kRequestAnimationFramesToRecordAfterBackForwardCacheRestore>;

 public:
  struct PaintTimingInfo {
    // https://w3c.github.io/paint-timing/#paint-timing-info-rendering-update-end-time
    base::TimeTicks rendering_update_end_time;

    // https://w3c.github.io/paint-timing/#paint-timing-info-implementation-defined-presentation-time
    base::TimeTicks presentation_time;
  };

  static const char kSupplementName[];

  explicit PaintTiming(Document&);
  PaintTiming(const PaintTiming&) = delete;
  PaintTiming& operator=(const PaintTiming&) = delete;
  ~PaintTiming() = default;

  static PaintTiming& From(Document&);
  static const PaintTiming* From(const Document&);

  // Mark*() methods record synchronous CPU paint timestamps once.
  void MarkFirstPaint();

  // MarkFirstImagePaint, and MarkFirstContentfulPaint
  // will also record first paint if first paint hasn't been recorded yet.
  void MarkFirstContentfulPaint();

  // MarkFirstImagePaint will also record first contentful paint if first
  // contentful paint hasn't been recorded yet.
  void MarkFirstImagePaint();

  // MarkFirstEligibleToPaint records the first time that the frame is not
  // throttled and so is eligible to paint. A null value indicates throttling.
  void MarkFirstEligibleToPaint();

  // MarkIneligibleToPaint resets the paint eligibility timestamp to null.
  // A null value indicates throttling. This call is ignored if a first
  // contentful paint has already been recorded.
  void MarkIneligibleToPaint();

  void NotifyPaint(bool is_first_paint, bool text_painted, bool image_painted);
  void NotifyPaintFinished();
  void NotifyInputEvent(WebInputEvent::Type);
  void NotifyScroll(mojom::blink::ScrollType);

  // The getters below return monotonically-increasing seconds, or zero if the
  // given paint event has not yet occurred. See the comments for
  // monotonicallyIncreasingTime in wtf/Time.h for additional details.

  // Returns the first time that anything was painted for the
  // current document after a hard navigation. This is not considering soft
  // navigations.
  base::TimeTicks FirstPaintForMetrics() const {
    return first_paint_presentation_for_ukm_;
  }

  // Times when the first paint happens after the page is restored from the
  // back-forward cache. If the element value is zero time tick, the first paint
  // event did not happen for that navigation.
  Vector<base::TimeTicks> FirstPaintsAfterBackForwardCacheRestore() const {
    return first_paints_after_back_forward_cache_restore_presentation_;
  }

  Vector<RequestAnimationFrameTimesAfterBackForwardCacheRestore>
  RequestAnimationFramesAfterBackForwardCacheRestore() const {
    return request_animation_frames_after_back_forward_cache_restore_;
  }

  // Returns the first time that 'contentful' content was painted in the current
  // document after a hard navigation. For instance, the first time that text or
  // image content was painted after the user landed on the page.
  base::TimeTicks FirstContentfulPaint() const {
    return first_contentful_paint_presentation_;
  }

  base::TimeTicks FirstContentfulPaintRenderedButNotPresentedAsMonotonicTime()
      const {
    return paint_details_.first_contentful_paint_;
  }

  // FirstImagePaint returns the first time that image content was painted.
  base::TimeTicks FirstImagePaint() const {
    return paint_details_.first_image_paint_presentation_;
  }

  base::TimeTicks FirstImagePaintRenderedButNotPresentedAsMonotonicTime()
      const {
    return paint_details_.first_image_paint_;
  }

  // FirstEligibleToPaint returns the first time that the frame is not
  // throttled and is eligible to paint. A null value indicates throttling.
  base::TimeTicks FirstEligibleToPaint() const {
    return first_eligible_to_paint_;
  }

  base::TimeTicks FirstContentfulPaintPresentation() const {
    return paint_details_.first_contentful_paint_presentation_;
  }

  base::TimeTicks FirstPaintRendered() const {
    return paint_details_.first_paint_;
  }

  Document* GetDocument() { return GetSupplementable(); }

  void OnRestoredFromBackForwardCache();


  void Trace(Visitor*) const override;

  // Returns the `LargestContentfulPaintManager` associated with this
  // `PaintTiming`. Returns null if hard LCP is no longer being recorded, e.g.
  // after first input.
  LargestContentfulPaintManager* GetLargestContentfulPaintManager() {
    return largest_contentful_paint_manager_;
  }


  // TODO(crbug.com/503420427): Consider removing this and proxying through
  // PaintTiming.
  PaintTimingDetector& GetPaintTimingDetector() {
    return *paint_timing_detector_.Get();
  }


  // Adds a `PaintTimingClient` to observe contentful paints. The client must
  // not have been previously added.
  void AddClient(PaintTimingClient*);

  // Removes a previously added `PaintTimingClient`. The client must have been
  // previously added.
  void RemoveClient(PaintTimingClient*);

  // Iterates over the `PaintTimingClient`s invoking the given function. Must
  // not add or remove clients.
  void ForEachClient(base::FunctionRef<void(PaintTimingClient*)>);

 private:
  friend class RecodingTimeAfterBackForwardCacheRestoreFrameCallback;

  // Native bookkeeping clients; static screenshots have no display feedback.
  HeapVector<Member<PaintTimingClient>> clients_;
  bool allow_client_modifications_ = true;

  void OnInputOrScroll();

  LocalFrame* GetFrame() const;
  void NotifyPaintTimingChanged();

  void DiscardPresentationCallbacks();

  // Set*() set the timing for the given paint event to the given timestamp if
  // the value is currently zero, and queue a presentation promise to record the
  // |first_*_presentation_| timestamp. These methods can be invoked from other
  // Mark*() or Set*() methods to make sure that first paint is marked as part
  // of marking first contentful paint, or that first contentful paint is marked
  // as part of marking first text/image paint, for example.
  void SetFirstPaint(base::TimeTicks stamp);

  // setFirstContentfulPaint will also set first paint time if first paint
  // time has not yet been recorded.
  void SetFirstContentfulPaint(base::TimeTicks stamp);

  void SetRequestAnimationFrameAfterBackForwardCacheRestore(wtf_size_t index,
                                                            size_t count);

  Vector<base::TimeTicks>
      first_paints_after_back_forward_cache_restore_presentation_;
  Vector<RequestAnimationFrameTimesAfterBackForwardCacheRestore>
      request_animation_frames_after_back_forward_cache_restore_;
  struct PaintDetails {
    // TODO(crbug/738235): Non first_*_presentation_ variables are only being
    // tracked to compute deltas for reporting histograms and should be removed
    // once we confirm the deltas and discrepancies look reasonable.
    base::TimeTicks first_paint_;
    base::TimeTicks first_paint_presentation_;
    base::TimeTicks first_image_paint_;
    base::TimeTicks first_image_paint_presentation_;
    base::TimeTicks first_contentful_paint_;
    base::TimeTicks first_contentful_paint_presentation_;
  };

  PaintDetails& GetRelevantPaintDetails() { return paint_details_; }

  PaintDetails paint_details_;
  // Timestamps used for UKM reporting.
  base::TimeTicks first_paint_presentation_for_ukm_;
  base::TimeTicks first_contentful_paint_presentation_;
  base::TimeTicks first_eligible_to_paint_;

  base::TimeTicks lcp_mouse_over_dispatch_time_;

  Member<PaintTimingDetector> paint_timing_detector_;
  Member<LargestContentfulPaintManager> largest_contentful_paint_manager_;
  // The callback ID for requestAnimationFrame to record its time after the page
  // is restored from the back-forward cache.
  int raf_after_bfcache_restore_measurement_callback_id_ = 0;

};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_TIMING_PAINT_TIMING_H_
