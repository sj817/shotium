// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_CUSTOM_STATE_SET_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_CUSTOM_STATE_SET_H_

#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_set.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class Element;

// This class is an implementation of 'CustomStateSet' IDL interface.
//
// The set is written only through ElementInternals.states, which is reachable
// only from a custom element's own script. It is read by selector matching for
// the :state() pseudo-class (SelectorChecker -> ElementInternals::HasState).
class CustomStateSet final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit CustomStateSet(Element& element);
  void Trace(Visitor* visitor) const override;

  // add() (the IDL setlike<> mutator, `internals.states.add("foo")`) used to
  // live here. It was reachable only from a custom element's own script, and
  // this engine has no script engine — nor can it define custom elements at
  // all without one (`customElements.define()` is itself script-only) — so
  // `list_` below can now never be populated and Has() below always reports
  // false. That's a real (if now vacuous) property of the DOM, not a stub:
  // it has always been true that `:state()` cannot match without script.
  uint32_t size() const;

  // This operation is O(size()).
  bool Has(const String& value) const;

 private:
  void InvalidateStyle() const;

  Member<Element> element_;
  // We don't use LinkedHashSet because it's difficult to
  // implement "live" iterators with them.
  // See crbug.com/1184020.
  //
  // If the O(size()) operations are problematic, we should change the type of
  // the following data member.
  Vector<String> list_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_CUSTOM_STATE_SET_H_
