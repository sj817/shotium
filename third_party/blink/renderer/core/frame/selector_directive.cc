// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/selector_directive.h"

#include "third_party/blink/renderer/core/editing/position.h"
#include "third_party/blink/renderer/core/editing/range_in_flat_tree.h"
#include "third_party/blink/renderer/platform/bindings/exception_code.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"

namespace blink {

SelectorDirective::SelectorDirective(Type type) : Directive(type) {}
SelectorDirective::~SelectorDirective() = default;

void SelectorDirective::DidFinishMatching(const RangeInFlatTree* range) {
  DCHECK(!selected_range_);
  matching_finished_ = true;

  if (range) {
    selected_range_ = MakeGarbageCollected<RangeInFlatTree>(
        range->StartPosition(), range->EndPosition());

    DCHECK(!selected_range_->IsCollapsed());
    // TODO(bokan): what if selected_range_ spans into a shadow tree?
    DCHECK(selected_range_->StartPosition().GetDocument());
    DCHECK_EQ(selected_range_->StartPosition().GetDocument(),
              selected_range_->EndPosition().GetDocument());
  }
}

void SelectorDirective::Trace(Visitor* visitor) const {
  Directive::Trace(visitor);
  visitor->Trace(selected_range_);
}

}  // namespace blink
