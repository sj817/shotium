// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/animation_events.h"

namespace cc {

AnimationPlaybackEvent::AnimationPlaybackEvent(
    AnimationPlaybackEvent::Type type,
    UniqueKeyframeModelId uid,
    int group_id,
    int target_property,
    base::TimeTicks monotonic_time)
    : type(type),
      uid(uid),
      group_id(group_id),
      target_property(target_property),
      monotonic_time(monotonic_time),
      is_impl_only(false) {}

AnimationPlaybackEvent::AnimationPlaybackEvent(
    const AnimationPlaybackEvent& other) {
  type = other.type;
  uid = other.uid;
  group_id = other.group_id;
  target_property = other.target_property;
  monotonic_time = other.monotonic_time;
  is_impl_only = other.is_impl_only;
  animation_start_time = other.animation_start_time;
  if (other.curve)
    curve = other.curve->Clone();
}

AnimationPlaybackEvent& AnimationPlaybackEvent::operator=(
    const AnimationPlaybackEvent& other) {
  type = other.type;
  uid = other.uid;
  group_id = other.group_id;
  target_property = other.target_property;
  monotonic_time = other.monotonic_time;
  is_impl_only = other.is_impl_only;
  animation_start_time = other.animation_start_time;
  if (other.curve)
    curve = other.curve->Clone();
  return *this;
}

AnimationPlaybackEvent::~AnimationPlaybackEvent() = default;

AnimationEvents::AnimationEvents() = default;

AnimationEvents::~AnimationEvents() = default;

bool AnimationEvents::IsEmpty() const {
  return events().empty();
}

bool AnimationPlaybackEvent::ShouldDispatchToKeyframeEffectAndModel() const {
  // is_impl_only events are not dispatched because they don't have
  // corresponding main thread components.
  return !is_impl_only;
}

AnimationTriggerEvent::AnimationTriggerEvent(int trigger_id,
                                             Type type,
                                             base::TimeTicks time)
    : trigger_id(trigger_id), type(type), time(time) {}

AnimationTriggerEvent::AnimationTriggerEvent(
    const AnimationTriggerEvent& other) {
  type = other.type;
  trigger_id = other.trigger_id;
  time = other.time;
}

}  // namespace cc
