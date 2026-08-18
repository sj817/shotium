// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_ADD_EVENT_LISTENER_OPTIONS_RESOLVED_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_ADD_EVENT_LISTENER_OPTIONS_RESOLVED_H_

#include "third_party/blink/renderer/bindings/core/v8/v8_add_event_listener_options.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/member.h"

namespace blink {

// EventListenerOptions and AddEventListenerOptions were hand-written here for
// one wave, while the IDL code generator was gone. It is back --
// tools/shot/gen_idl_dictionaries.py produces both from their own .idl -- so
// the hand-written copies were redefinitions of the generated classes and are
// deleted. v8_add_event_listener_options.h, included above, is the definition
// now, and it brings v8_event_listener_options.h with it.

// AddEventListenerOptionsResolved class represents resolved event listener
// options. An application requests AddEventListenerOptions and the user
// agent may change ('resolve') these settings (based on settings or policies)
// and the result and the reasons why changes occurred are stored in this class.
class CORE_EXPORT AddEventListenerOptionsResolved
    : public AddEventListenerOptions {
 public:
  AddEventListenerOptionsResolved();
  AddEventListenerOptionsResolved(const AddEventListenerOptions*);
  ~AddEventListenerOptionsResolved() override;

  void SetPassiveForcedForDocumentTarget(bool forced) {
    passive_forced_for_document_target_ = forced;
  }
  bool PassiveForcedForDocumentTarget() const {
    return passive_forced_for_document_target_;
  }

  // Set whether passive was specified when the options were
  // created by callee.
  void SetPassiveSpecified(bool specified) { passive_specified_ = specified; }
  bool PassiveSpecified() const { return passive_specified_; }

  void SetAnimationTrigger(bool val) { animation_trigger_ = val; }
  bool IsAnimationTrigger() const { return animation_trigger_; }

  void Trace(Visitor*) const override;

 private:
  bool passive_forced_for_document_target_{false};
  bool passive_specified_{false};
  bool animation_trigger_{false};
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_ADD_EVENT_LISTENER_OPTIONS_RESOLVED_H_
