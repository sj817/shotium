// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/forms/autofill_event.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/event_interface_names.h"
#include "third_party/blink/renderer/core/event_type_names.h"

namespace blink {

AutofillEvent::AutofillEvent(
    const AtomicString& type,
    HeapVector<std::pair<Member<Element>, String>> field_data,
    const base::UnguessableToken& fill_id,
    bool supports_refill)
    : field_data_(field_data,
                  [](const std::pair<Member<Element>, String>& pair) {
                    AutofillFieldData* data = AutofillFieldData::Create();
                    data->setField(pair.first.Get());
                    data->setValue(pair.second);
                    return data;
                  }),
      fill_id_(fill_id),
      supports_refill_(supports_refill) {}

AutofillEvent* AutofillEvent::Create(
    const AtomicString& type,
    HeapVector<std::pair<Member<Element>, String>> field_data,
    const base::UnguessableToken& fill_id,
    bool supports_refill) {
  AutofillEvent* event = MakeGarbageCollected<AutofillEvent>(
      type, std::move(field_data), fill_id, supports_refill);
  event->initEvent(type, false, false);
  return event;
}

void AutofillEvent::Trace(Visitor* visitor) const {
  visitor->Trace(field_data_);
  Event::Trace(visitor);
}

const AtomicString& AutofillEvent::InterfaceName() const {
  return event_interface_names::kAutofillEvent;
}

const HeapVector<Member<AutofillFieldData>>& AutofillEvent::autofillValues()
    const {
  return field_data_;
}

}  // namespace blink
