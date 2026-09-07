// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_TRANSITION_INTERPOLATION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_TRANSITION_INTERPOLATION_H_

#include <optional>

#include "base/check_op.h"
#include "third_party/blink/renderer/core/animation/interpolation.h"
#include "third_party/blink/renderer/core/animation/interpolation_type.h"
#include "third_party/blink/renderer/core/core_export.h"

namespace blink {

class CSSInterpolationEnvironment;
class TypedInterpolationValue;

// Interpolates the start and end values of a CSS transition on the main thread.
class CORE_EXPORT TransitionInterpolation : public Interpolation {
 public:
  TransitionInterpolation(const PropertyHandle& property,
                          const InterpolationType* type,
                          InterpolationValue&& start,
                          InterpolationValue&& end,
                          bool is_attr_tainted = false)
      : property_(property),
        type_(type),
        start_(std::move(start)),
        end_(std::move(end)),
        merge_(type->MaybeMergeSingles(start_.Clone(), end_.Clone())),
        is_attr_tainted_(is_attr_tainted) {
    // Incredibly speculative CHECKs, to try and get any insight on
    // crbug.com/826627. Somehow a crash is happening in this constructor, which
    // we believe is based on |start_| having no interpolable value. However a
    // CHECK added in TransitionKeyframe::SetValue isn't firing, so doing some
    // speculation here to try and broaden our understanding.
    // TODO(crbug.com/826627): Revert once bug is fixed.
    CHECK(start_);
    if (merge_) {
      CHECK(merge_);
      cached_interpolable_value_ = merge_.start_interpolable_value->Clone();
    }
  }

  void Apply(CSSInterpolationEnvironment&) const;

  bool IsTransitionInterpolation() const final { return true; }

  const PropertyHandle& GetProperty() const final { return property_; }

  TypedInterpolationValue* GetInterpolatedValue() const;

  void Interpolate(
      int iteration,
      double fraction,
      EffectModel::IterationCompositeOperation iteration_composite) final;

  void Trace(Visitor* visitor) const override {
    visitor->Trace(type_);
    visitor->Trace(start_);
    visitor->Trace(end_);
    visitor->Trace(merge_);
    visitor->Trace(cached_interpolable_value_);
    Interpolation::Trace(visitor);
  }

 private:
  const InterpolableValue& CurrentInterpolableValue() const;
  const NonInterpolableValue* CurrentNonInterpolableValue() const;

  const PropertyHandle property_;
  Member<const InterpolationType> type_;
  const InterpolationValue start_;
  const InterpolationValue end_;
  const PairwiseInterpolationValue merge_;
  const bool is_attr_tainted_;

  mutable std::optional<double> cached_fraction_;
  mutable int cached_iteration_ = 0;
  mutable Member<InterpolableValue> cached_interpolable_value_;
};

template <>
struct DowncastTraits<TransitionInterpolation> {
  static bool AllowFrom(const Interpolation& value) {
    return value.IsTransitionInterpolation();
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_TRANSITION_INTERPOLATION_H_
