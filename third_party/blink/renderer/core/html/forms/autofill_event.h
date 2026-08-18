// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_AUTOFILL_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_AUTOFILL_EVENT_H_

#include "base/unguessable_token.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_autofill_field_data.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_vector.h"

namespace blink {

class AutofillFieldData;

// This class is used to dispatch an autofill event before the browser performs
// the autofill operation on a form. The event contains the fields that are
// about to be filled by autofill. If `supports_refill` is true, the `refill`
// attribute returns a Promise that, when the refill completes, resolves. If
// `supports_refill` is false, the `refill` attribute is null.
class CORE_EXPORT AutofillEvent : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static AutofillEvent* Create(
      const AtomicString& type,
      HeapVector<std::pair<Member<Element>, String>> field_data,
      const base::UnguessableToken& fill_id,
      bool supports_refill);

  AutofillEvent(const AtomicString& type,
                HeapVector<std::pair<Member<Element>, String>> field_data,
                const base::UnguessableToken& fill_id,
                bool supports_refill);

  const AtomicString& InterfaceName() const override;

  bool IsRefillDirty() const { return false; }
  const HeapVector<Member<AutofillFieldData>>& autofillValues() const;

  // The refill request itself was only reachable through the `refill` IDL
  // attribute, which handed script a function returning a Promise. That entry
  // point is gone, but the event still carries the data the embedder handed it
  // so a non-script consumer can drive WebAutofillClient::RequestRefill().
  bool SupportsRefill() const { return supports_refill_; }
  const base::UnguessableToken& FillId() const { return fill_id_; }

  void Trace(Visitor*) const final;

 private:
  HeapVector<Member<AutofillFieldData>> field_data_;
  base::UnguessableToken fill_id_;
  bool supports_refill_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_AUTOFILL_EVENT_H_
