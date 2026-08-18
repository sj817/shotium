// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/forms/submit_event.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_submit_event_init.h"
#include "third_party/blink/renderer/core/dom/events/event_dispatcher.h"
#include "third_party/blink/renderer/core/dom/events/event_path.h"
#include "third_party/blink/renderer/core/event_interface_names.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

SubmitEvent::SubmitEvent(const AtomicString& type,
                         const SubmitEventInit* event_init)
    : Event(type, event_init),
      submitter_(event_init ? event_init->submitter() : nullptr),
      related_target_(event_init ? event_init->submitter() : nullptr),
      agent_invoked_(event_init && event_init->agentInvoked()) {}

SubmitEvent* SubmitEvent::Create(const AtomicString& type,
                                 const SubmitEventInit* event_init) {
  return MakeGarbageCollected<SubmitEvent>(type, event_init);
}

void SubmitEvent::Trace(Visitor* visitor) const {
  visitor->Trace(submitter_);
  visitor->Trace(related_target_);
  Event::Trace(visitor);
}

HTMLElement* SubmitEvent::submitter() const {
  if (!submitter_) {
    CHECK(!related_target_);
    return nullptr;
  }

  if (RuntimeEnabledFeatures::ShadowRootReferenceTargetEnabled(
          submitter_->GetExecutionContext())) {
    EventTarget* related_target = related_target_.Get();
    return related_target ? DynamicTo<HTMLElement>(related_target->ToNode())
                          : nullptr;
  }
  return submitter_.Get();
}

const AtomicString& SubmitEvent::InterfaceName() const {
  return event_interface_names::kSubmitEvent;
}

DispatchEventResult SubmitEvent::DispatchEvent(EventDispatcher& dispatcher) {
  if (RuntimeEnabledFeatures::ShadowRootReferenceTargetEnabled(
          dispatcher.GetNode().GetExecutionContext())) {
    GetEventPath().AdjustForRelatedTarget(dispatcher.GetNode(),
                                          relatedTarget());
  }
  return dispatcher.Dispatch();
}

}  // namespace blink
