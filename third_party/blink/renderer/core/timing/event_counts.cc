// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/timing/event_counts.h"

#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string_hash.h"
#include "third_party/blink/renderer/platform/wtf/wtf.h"

namespace blink {

void EventCounts::Add(const AtomicString& event_type) {
  auto iterator = event_count_map_.find(event_type);
  if (iterator == event_count_map_.end()) {
    return;
  }
  iterator->value++;
}

void EventCounts::AddMultipleEvents(const AtomicString& event_type,
                                    uint64_t count) {
  auto iterator = event_count_map_.find(event_type);
  if (iterator == event_count_map_.end())
    return;
  iterator->value += count;
}

EventCounts::EventCounts() {
  // Should contain the same types that would return true in
  // IsEventTypeForEventTiming() in event_timing.cc. Note that this list differs
  // from https://wicg.github.io/event-timing/#sec-events-exposed in that
  // dragexit is not present since it's currently not implemented in Chrome.
  DCHECK(IsMainThread());
  DEFINE_STATIC_LOCAL(
      const Vector<AtomicString>, event_types,
      ({/* MouseEvents */
        event_type_names::kAuxclick, event_type_names::kClick,
        event_type_names::kContextmenu, event_type_names::kDblclick,
        event_type_names::kMousedown, event_type_names::kMouseenter,
        event_type_names::kMouseleave, event_type_names::kMouseout,
        event_type_names::kMouseover, event_type_names::kMouseup,
        /* PointerEvents */
        event_type_names::kPointerover, event_type_names::kPointerenter,
        event_type_names::kPointerdown, event_type_names::kPointerup,
        event_type_names::kPointercancel, event_type_names::kPointerout,
        event_type_names::kPointerleave, event_type_names::kGotpointercapture,
        event_type_names::kLostpointercapture,
        /* TouchEvents */
        event_type_names::kTouchstart, event_type_names::kTouchend,
        event_type_names::kTouchcancel,
        /* KeyboardEvents */
        event_type_names::kKeydown, event_type_names::kKeypress,
        event_type_names::kKeyup,
        /* InputEvents */
        event_type_names::kBeforeinput, event_type_names::kInput,
        /* CompositionEvents */
        event_type_names::kCompositionstart,
        event_type_names::kCompositionupdate, event_type_names::kCompositionend,
        /* Drag & Drop Events */
        event_type_names::kDragstart, event_type_names::kDragend,
        event_type_names::kDragenter, event_type_names::kDragleave,
        event_type_names::kDragover, event_type_names::kDrop}));
  for (const auto& type : event_types) {
    event_count_map_.insert(type, 0u);
  }
  if (RuntimeEnabledFeatures::NavigationEventTimingEnabled()) {
    event_count_map_.insert(event_type_names::kNavigate, 0u);
    event_count_map_.insert(event_type_names::kPopstate, 0u);
    event_count_map_.insert(event_type_names::kHashchange, 0u);
  }
}

}  // namespace blink
