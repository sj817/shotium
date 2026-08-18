// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_EVENT_COUNTS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_EVENT_COUNTS_H_

#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string_hash.h"

namespace blink {

class EventCounts final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  EventCounts();

  const HashMap<AtomicString, uint64_t>& Map() const {
    return event_count_map_;
  }

  // IDL attributes / methods
  wtf_size_t size() const { return event_count_map_.size(); }

  void Add(const AtomicString& event_type);

  // Add multiple events with the same event type.
  void AddMultipleEvents(const AtomicString& event_type, uint64_t count);

  // Checks if this specific event type (by name) is in the list of publicly
  // exposed events, which is already hard-coded for the
  // `performance.eventCounts` api.
  // See: https://www.w3.org/TR/event-timing/#sec-events-exposed
  bool IsSupportedEventType(const AtomicString& event_type) {
    return event_count_map_.Contains(event_type);
  }

  void Trace(Visitor* visitor) const override {
    ScriptWrappable::Trace(visitor);
  }

 private:

  HashMap<AtomicString, uint64_t> event_count_map_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_EVENT_COUNTS_H_
