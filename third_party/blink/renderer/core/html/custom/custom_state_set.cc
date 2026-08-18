// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/custom/custom_state_set.h"

#include "third_party/blink/renderer/core/css/css_selector.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_idioms.h"
#include "third_party/blink/renderer/core/dom/element.h"

namespace blink {

CustomStateSet::CustomStateSet(Element& element) : element_(element) {}

void CustomStateSet::Trace(Visitor* visitor) const {
  visitor->Trace(element_);
  ScriptWrappable::Trace(visitor);
}

uint32_t CustomStateSet::size() const {
  return list_.size();
}

bool CustomStateSet::Has(const String& value) const {
  return list_.Contains(value);
}

void CustomStateSet::InvalidateStyle() const {
  // TOOD(tkent): The following line invalidates all of rulesets with any
  // custom state pseudo-classes though we should invalidate only rulesets
  // with the updated state ideally. We can improve style resolution
  // performance in documents with various custom state pseudo-classes by
  // having blink::InvalidationSet for each of states.
  element_->PseudoStateChanged(CSSSelector::kPseudoState);
}

}  // namespace blink
