// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/view_transition/view_transition_type_set.h"

#include "third_party/blink/renderer/core/css/css_selector.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/view_transition/view_transition.h"
#include "third_party/blink/renderer/core/view_transition/view_transition_supplement.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"

namespace blink {

bool ViewTransitionTypeSet::IsValidType(const String& value) {
  String lower = value.ToAsciiLower();
  return lower != "none" && !lower.starts_with("-ua-");
}

ViewTransitionTypeSet::ViewTransitionTypeSet(
    ViewTransition* view_transition,
    const Vector<String>& initial_values) {
  view_transition_ = view_transition;
  for (const String& type : initial_values) {
    AddInternal(type);
  }
}

void ViewTransitionTypeSet::AddInternal(const String& type) {
  if (types_.Contains(type)) {
    return;
  }

  types_.push_back(type);
  if (IsValidType(type)) {
    InvalidateStyle();
  }
}

void ViewTransitionTypeSet::Trace(Visitor* visitor) const {
  ScriptWrappable::Trace(visitor);
  visitor->Trace(view_transition_);
}

void ViewTransitionTypeSet::add(const String& value,
                                ExceptionState& exception_state) {
  AddInternal(value);
}

void ViewTransitionTypeSet::InvalidateStyle() {
  if (!view_transition_) {
    return;
  }

  if (!view_transition_->DomWindow()) {
    return;
  }

  Document* document = view_transition_->DomWindow()->document();
  if (document->GetViewTransitions().GetTransition() != view_transition_) {
    return;
  }

  if (Element* document_element = document->documentElement()) {
    document_element->PseudoStateChanged(
        CSSSelector::kPseudoActiveViewTransitionType);
  }
}

}  // namespace blink
