// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_FROZEN_ARRAY_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_FROZEN_ARRAY_H_

#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/heap_traits.h"

namespace blink {

// FrozenArray<T> implements the IDL frozen array types.
//
// Upstream this was two classes in two directories:
//
//   platform/bindings/frozen_array_base.h  bindings::FrozenArrayBase
//   bindings/core/v8/frozen_array.h        FrozenArray<IDLType>
//
// and the split existed for exactly one reason: FrozenArrayBase implemented
// Wrap()/AssociateWithWrapper() to hand script a *frozen* (immutable) V8 array,
// caching one wrapper per world so that repeated conversions of the same
// FrozenArray returned the identical JS object; the template subclass supplied
// only the type-dependent MakeV8ArrayToBeFrozen(). Every V8 mention in both
// files is in that path.
//
// What the 31 callers in core actually use is the other half: an immutable,
// traceable vector they can hand out by const reference (Element's assigned
// slot list, ResizeObserverEntry's box sizes, CSSContainerRule's conditions).
// So that half is here, in one header, and the wrapper half is gone. "Frozen"
// is still accurate -- there is no mutating API and `array_` is const.
//
// Limitation worth stating: upstream's parameter was an *IDL* type, mapped to
// the Blink implementation type by IDLTypeToBlinkImplType. Here the parameter
// is the Blink type directly. For interface types those are the same thing,
// which is every instantiation in this tree (Element, ResizeObserverSize,
// ElementBehavior, CSSContainerCondition). FrozenArray<IDLString> would need
// that mapping back; it would fail to compile rather than do something subtly
// wrong.
template <typename T>
class FrozenArray final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  using VectorType = VectorOf<T>;
  using size_type = typename VectorType::size_type;
  using value_type = typename VectorType::value_type;
  using const_reference = typename VectorType::const_reference;
  using const_pointer = typename VectorType::const_pointer;
  using const_iterator = typename VectorType::const_iterator;
  using const_reverse_iterator = typename VectorType::const_reverse_iterator;

  FrozenArray() = default;
  explicit FrozenArray(VectorType array) : array_(std::move(array)) {}
  ~FrozenArray() override = default;

  // Vector-compatible APIs
  size_type size() const { return array_.size(); }
  bool empty() const { return array_.empty(); }
  const_reference at(size_type index) const { return array_.at(index); }
  const_reference operator[](size_type index) const { return array_[index]; }
  const value_type* data() const { return array_.data(); }
  const_iterator begin() const { return array_.begin(); }
  const_iterator end() const { return array_.end(); }
  const_reverse_iterator rbegin() const { return array_.rbegin(); }
  const_reverse_iterator rend() const { return array_.rend(); }
  const_reference front() const { return array_.front(); }
  const_reference back() const { return array_.back(); }

  const VectorType& AsVector() const { return array_; }

  void Trace(Visitor* visitor) const override {
    ScriptWrappable::Trace(visitor);
    TraceIfNeeded<VectorType>::Trace(visitor, array_);
  }

 private:
  const VectorType array_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_FROZEN_ARRAY_H_
