// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/timing/performance_long_animation_frame_timing.h"

#include "third_party/blink/renderer/core/dom/dom_high_res_time_stamp.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/dom_window.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/performance_entry_names.h"
#include "third_party/blink/renderer/core/timing/dom_window_performance.h"
#include "third_party/blink/renderer/core/timing/performance.h"
#include "third_party/blink/renderer/core/timing/performance_script_timing.h"
#include "third_party/blink/renderer/core/timing/task_attribution_timing.h"
#include "third_party/blink/renderer/core/timing/window_performance.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

PerformanceLongAnimationFrameTiming*
PerformanceLongAnimationFrameTiming::Create(
    AnimationFrameTimingInfo* info,
    base::TimeTicks time_origin,
    bool cross_origin_isolated_capability,
    ExecutionContext* execution_context,
    const std::optional<DOMPaintTimingInfo>& paint_timing_info,
    uint64_t navigation_id) {
  // Worker contexts have no window, so `source` is nullptr there. The observer
  // security origin filters which scripts may expose their source attribution.
  DOMWindow* source = DynamicTo<LocalDOMWindow>(execution_context);
  const SecurityOrigin* observer_security_origin =
      execution_context ? execution_context->GetSecurityOrigin() : nullptr;

  DOMHighResTimeStamp startTime =
      Performance::MonotonicTimeToDOMHighResTimeStamp(
          time_origin, info->FrameStartTime(),
          /*allow_negative_value=*/false, cross_origin_isolated_capability);
  double duration =
      paint_timing_info
          ? (paint_timing_info->paint_time - startTime)
          : Performance::ClampTimeResolution(info->Duration(),
                                             cross_origin_isolated_capability);
  PerformanceLongAnimationFrameTiming* entry =
      MakeGarbageCollected<PerformanceLongAnimationFrameTiming>(
          duration, startTime, info, time_origin,
          cross_origin_isolated_capability, source, observer_security_origin,
          navigation_id);
  if (paint_timing_info.has_value()) {
    entry->SetPaintTimingInfo(*paint_timing_info);
  }
  return entry;
}

PerformanceLongAnimationFrameTiming::PerformanceLongAnimationFrameTiming(
    double duration,
    DOMHighResTimeStamp startTime,
    AnimationFrameTimingInfo* info,
    base::TimeTicks time_origin,
    bool cross_origin_isolated_capability,
    DOMWindow* source,
    const SecurityOrigin* observer_security_origin,
    uint64_t navigation_id)
    : PerformanceEntry(duration,
                       AtomicString("long-animation-frame"),
                       startTime,
                       source,
                       navigation_id),
      script_count_(info->ScriptCount()),
      render_start_(Performance::MonotonicTimeToDOMHighResTimeStamp(
          time_origin,
          info->RenderStartTime(),
          /*allow_negative_value=*/false,
          cross_origin_isolated_capability)),
      style_and_layout_start_(Performance::MonotonicTimeToDOMHighResTimeStamp(
          time_origin,
          info->StyleAndLayoutStartTime(),
          /*allow_negative_value=*/false,
          cross_origin_isolated_capability)),
      first_ui_event_timestamp_(Performance::MonotonicTimeToDOMHighResTimeStamp(
          time_origin,
          info->FirstUIEventTime(),
          /*allow_negative_value=*/false,
          cross_origin_isolated_capability)),
      blocking_duration_(
          Performance::ClampTimeResolution(info->TotalBlockingDuration(),
                                           cross_origin_isolated_capability)),
      style_duration_(
          Performance::ClampTimeResolution(info->StyleDuration(),
                                           cross_origin_isolated_capability)),
      layout_duration_(
          Performance::ClampTimeResolution(info->LayoutDuration(),
                                           cross_origin_isolated_capability)) {
  for (ScriptTimingInfo* script : info->Scripts()) {
    if (!observer_security_origin ||
        observer_security_origin->CanAccess(script->GetSecurityOrigin())) {
      scripts_.push_back(MakeGarbageCollected<PerformanceScriptTiming>(
          script, time_origin, cross_origin_isolated_capability, source,
          navigation_id));
    }
  }

  // This used to build PerformanceMarkConditional/PerformanceMeasureConditional
  // entries from info->ConditionalMarks()/ConditionalMeasures() when
  // RuntimeEnabledFeatures::ConditionalTracingLoAFEnabled(), merging them
  // into user_timing_entries_. Those two classes backed
  // performance.markConditional()/measureConditional() (a DevTools tracing
  // feature: see performance_mark_conditional.idl /
  // performance_measure_conditional.idl) and were deleted, without their
  // implementation, in the v8ectomy/tracing cuts along with the rest of
  // DevTools instrumentation. markConditional()/measureConditional()
  // themselves are not wired up anywhere any more, so ConditionalMarks()/
  // ConditionalMeasures() can never be non-empty; the CHECKs below still
  // verify that.
  CHECK(info->ConditionalMarks().empty());
  CHECK(info->ConditionalMeasures().empty());
}

PerformanceLongAnimationFrameTiming::~PerformanceLongAnimationFrameTiming() =
    default;

const AtomicString& PerformanceLongAnimationFrameTiming::entryType() const {
  return performance_entry_names::kLongAnimationFrame;
}

PerformanceEntryType PerformanceLongAnimationFrameTiming::EntryTypeEnum()
    const {
  return PerformanceEntry::EntryType::kLongAnimationFrame;
}

void PerformanceLongAnimationFrameTiming::Trace(Visitor* visitor) const {
  PerformanceEntry::Trace(visitor);
  visitor->Trace(scripts_);
  visitor->Trace(user_timing_entries_);
}

}  // namespace blink
