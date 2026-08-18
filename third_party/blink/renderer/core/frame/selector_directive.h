// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_SELECTOR_DIRECTIVE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_SELECTOR_DIRECTIVE_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/frame/directive.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/member.h"

namespace blink {
class RangeInFlatTree;

// Provides the JavaScript-exposed SelectorDirective base class. Selector
// directives are those that select a specific part of the page to scroll to.
// This is the base interface for all selector directive types and provides
// functionality to allow authors to extract the Node Range that the selector
// is scrolling to.
// See: https://github.com/WICG/scroll-to-text-fragment/issues/160
// TODO(bokan): Update link once we have better public documentation.
class CORE_EXPORT SelectorDirective : public Directive {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit SelectorDirective(Type);
  ~SelectorDirective() override;

  // Called by Blink-internal code once the selector has finished running. It
  // records the located Range, or nothing if one wasn't found.
  void DidFinishMatching(const RangeInFlatTree*);
  void Trace(Visitor*) const override;

 private:
  // The range this selector matched, cached once matching has finished.
  Member<RangeInFlatTree> selected_range_;

  bool matching_finished_ = false;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_SELECTOR_DIRECTIVE_H_
