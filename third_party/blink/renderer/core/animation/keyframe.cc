// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/keyframe.h"

#include "base/numerics/safe_conversions.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_timeline_range_offset.h"
#include "third_party/blink/renderer/core/animation/effect_model.h"
#include "third_party/blink/renderer/core/animation/invalidatable_interpolation.h"
#include "third_party/blink/renderer/core/animation/timeline_range.h"
#include "third_party/blink/renderer/core/css/cssom/css_unit_value.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

const double Keyframe::kNullComputedOffset =
    std::numeric_limits<double>::quiet_NaN();

Keyframe::PropertySpecificKeyframe::PropertySpecificKeyframe(
    double offset,
    scoped_refptr<TimingFunction> easing,
    EffectModel::CompositeOperation composite)
    : offset_(offset), easing_(std::move(easing)), composite_(composite) {
  DCHECK(std::isfinite(offset));
  if (!easing_)
    easing_ = LinearTimingFunction::Shared();
}

Interpolation* Keyframe::PropertySpecificKeyframe::CreateInterpolation(
    const PropertyHandle& property_handle,
    const Keyframe::PropertySpecificKeyframe& end,
    const Keyframe::PropertySpecificKeyframe* final_keyframe) const {
  // const_cast to take refs.
  return MakeGarbageCollected<InvalidatableInterpolation>(
      property_handle, const_cast<PropertySpecificKeyframe*>(this),
      const_cast<PropertySpecificKeyframe*>(&end),
      const_cast<PropertySpecificKeyframe*>(final_keyframe));
}

Vector<PropertyHandle> Keyframe::PropertiesVector() const {
  Vector<PropertyHandle> result;
  const auto& properties = Properties();
  result.ReserveInitialCapacity(
      base::checked_cast<wtf_size_t>(properties.size()));
  for (const auto& property : properties) {
    result.push_back(property);
  }
  return result;
}

bool Keyframe::ResolveTimelineOffset(const TimelineRange& timeline_range,
                                     double range_start,
                                     double range_end) {
  if (!timeline_offset_) {
    return false;
  }

  double relative_offset =
      timeline_range.ToFractionalOffset(timeline_offset_.value());
  double range = range_end - range_start;
  if (!range) {
    if (offset_) {
      offset_.reset();
      computed_offset_ = kNullComputedOffset;
      return true;
    }
  } else {
    double resolved_offset = (relative_offset - range_start) / range;
    if (!offset_ || offset_.value() != resolved_offset) {
      offset_ = resolved_offset;
      computed_offset_ = resolved_offset;
      return true;
    }
  }

  return false;
}

/* static */
bool Keyframe::LessThan(const Member<Keyframe>& a, const Member<Keyframe>& b) {
  std::optional first =
      a->ComputedOffset().has_value() ? a->ComputedOffset() : a->Offset();
  std::optional second =
      b->ComputedOffset().has_value() ? b->ComputedOffset() : b->Offset();

  if (first < second) {
    return true;
  }

  if (first > second) {
    return false;
  }

  if (a->original_index_ < b->original_index_) {
    return true;
  }

  return false;
}

}  // namespace blink
