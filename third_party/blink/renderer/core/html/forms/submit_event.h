// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_SUBMIT_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_SUBMIT_EVENT_H_

#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/html/html_element.h"

namespace blink {
class SubmitEventInit;

class SubmitEvent : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static SubmitEvent* Create(const AtomicString& type,
                             const SubmitEventInit* event_init);
  SubmitEvent(const AtomicString& type, const SubmitEventInit* event_init);

  void Trace(Visitor* visitor) const override;
  HTMLElement* submitter() const;
  EventTarget* relatedTarget() const override { return related_target_.Get(); }
  void SetRelatedTarget(EventTarget* related_target) override {
    related_target_ = related_target;
  }
  const AtomicString& InterfaceName() const override;

  DispatchEventResult DispatchEvent(EventDispatcher&) override;

  bool agentInvoked() const { return agent_invoked_; }

 private:
  // crbug.com/346835896: When ShadowRootReferenceTargetEnabled ships, the
  // event's submitter will be managed by `related_target_` instead of
  // `submitter_`. When the flag is cleaned up the `submitter_` member will be
  // removed.
  Member<HTMLElement> submitter_;
  Member<EventTarget> related_target_;
  bool agent_invoked_ = false;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_SUBMIT_EVENT_H_
