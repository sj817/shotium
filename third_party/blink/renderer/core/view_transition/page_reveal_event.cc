// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/view_transition/page_reveal_event.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_page_reveal_event_init.h"
#include "third_party/blink/renderer/core/event_interface_names.h"
#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/view_transition/view_transition_utils.h"

namespace blink {

PageRevealEvent::PageRevealEvent()
    : Event(event_type_names::kPagereveal, Bubbles::kNo, Cancelable::kNo) {
}

PageRevealEvent::PageRevealEvent(const AtomicString& type,
                                 const PageRevealEventInit* initializer)
    : Event(type, initializer) {}

PageRevealEvent::~PageRevealEvent() = default;

const AtomicString& PageRevealEvent::InterfaceName() const {
  return event_interface_names::kPageRevealEvent;
}

void PageRevealEvent::Trace(Visitor* visitor) const {
  Event::Trace(visitor);
}

}  // namespace blink
