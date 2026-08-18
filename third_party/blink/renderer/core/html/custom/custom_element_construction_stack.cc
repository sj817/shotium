// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/custom/custom_element_construction_stack.h"

#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_registry.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_map.h"
#include "third_party/blink/renderer/platform/heap/disallow_new_wrapper.h"

namespace blink {

namespace {

// We manage the construction stacks in a map of maps, where the first map key
// is a window, and the second map key is a constructor and value is a
// construction stack.

// Keyed by definition rather than by constructor: a definition identifies
// its constructor uniquely, and the constructor is a script function that
// cannot exist here. The map itself stays -- it is what makes a custom element
// constructor re-entrant.
using ConstructorToStackMap =
    GCedHeapHashMap<Member<CustomElementDefinition>,
                    Member<CustomElementConstructionStack>>;

using WindowMap =
    HeapHashMap<Member<const LocalDOMWindow>, Member<ConstructorToStackMap>>;
using WindowMapHolder = DisallowNewWrapper<WindowMap>;

Persistent<WindowMapHolder>& GetWindowMapHolder() {
  // This map is created only when upgrading custom elements and cleared when it
  // finishes, so it never leaks. This is because construction stacks are
  // populated only during custom element upgrading.
  DEFINE_STATIC_LOCAL(Persistent<WindowMapHolder>, holder, ());
  return holder;
}

WindowMap& EnsureWindowMap() {
  Persistent<WindowMapHolder>& holder = GetWindowMapHolder();
  if (!holder) {
    holder = MakeGarbageCollected<WindowMapHolder>();
  }
  return holder->Value();
}

ConstructorToStackMap& EnsureConstructorToStackMap(
    const LocalDOMWindow* window) {
  WindowMap& window_map = EnsureWindowMap();
  auto add_result = window_map.insert(window, nullptr);
  if (add_result.is_new_entry) {
    add_result.stored_value->value =
        MakeGarbageCollected<ConstructorToStackMap>();
  }
  return *add_result.stored_value->value;
}

CustomElementConstructionStack& EnsureConstructionStack(
    CustomElementDefinition& definition) {
  const LocalDOMWindow* window = definition.GetRegistry().GetOwnerWindow();
  ConstructorToStackMap& stack_map = EnsureConstructorToStackMap(window);

  auto add_result = stack_map.insert(&definition, nullptr);
  if (add_result.is_new_entry) {
    add_result.stored_value->value =
        MakeGarbageCollected<CustomElementConstructionStack>();
  }
  return *add_result.stored_value->value;
}

}  // namespace

wtf_size_t CustomElementConstructionStackScope::nesting_level_ = 0;

CustomElementConstructionStackScope::CustomElementConstructionStackScope(
    CustomElementDefinition& definition,
    Element& element)
    : construction_stack_(EnsureConstructionStack(definition)) {
  // Push the construction stack.
  construction_stack_.push_back(
      CustomElementConstructionStackEntry(element, definition));
  ++nesting_level_;
#if DCHECK_IS_ON()
  element_ = &element;
  depth_ = construction_stack_.size();
#endif
}

CustomElementConstructionStackScope::~CustomElementConstructionStackScope() {
#if DCHECK_IS_ON()
  DCHECK(!construction_stack_.back().element ||
         construction_stack_.back().element == element_);
  DCHECK_EQ(construction_stack_.size(), depth_);  // It's a *stack*.
#endif
  // Pop the construction stack.
  construction_stack_.pop_back();
  // Clear the memory backing if all construction stacks are empty.
  if (--nesting_level_ == 0) {
    GetWindowMapHolder().Clear();
  }
}

}  // namespace blink
