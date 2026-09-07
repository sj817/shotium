// Copyright 2025 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/animation_trigger.h"

#include "third_party/blink/renderer/platform/bindings/script_forbidden_scope.h"

#include "base/time/time.h"
#include "third_party/blink/renderer/core/animation/animation.h"
#include "third_party/blink/renderer/core/animation/animation_timeline.h"
#include "third_party/blink/renderer/core/animation/css/css_animation.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"

namespace blink {

using Behavior = AnimationTrigger::Behavior;

namespace {

// Behavior implementations per
// https://github.com/w3c/csswg-drafts/issues/12611#issue-3326243729

void PerformPlay(Animation& animation,
                 V8AnimationPlayState::Enum play_state,
                 ExceptionState& exception_state) {
  Animation::AutoRewind auto_rewind = Animation::AutoRewind::kDisabled;
  // TODO(crbug.com/390314945): EventTriggers currently do not pause animations
  // when added, possibly leaving the animation idle. This causes trigger play
  // actions that do not auto-rewind to fail (be no-op). EventTriggers should
  // pause animation when added.
  if (play_state == V8AnimationPlayState::Enum::kIdle) {
    auto_rewind = Animation::AutoRewind::kEnabled;
  }
  animation.PlayInternal(auto_rewind, exception_state);

  V8AnimationPlayState::Enum new_play_state =
      animation.CalculateAnimationPlayState();
  DCHECK(animation.PendingInternal() ||
         new_play_state == V8AnimationPlayState::Enum::kRunning ||
         new_play_state == V8AnimationPlayState::Enum::kFinished);
}

void PerformPause(Animation& animation,
                  V8AnimationPlayState::Enum play_state,
                  ExceptionState& exception_state) {
  if (play_state == V8AnimationPlayState::Enum::kRunning) {
    animation.PauseInternal(ASSERT_NO_EXCEPTION);
  }
}

void PerformPlayForwards(Animation& animation,
                         V8AnimationPlayState::Enum play_state,
                         ExceptionState& exception_state) {
  Animation::AutoRewind auto_rewind = Animation::AutoRewind::kDisabled;
  // See PerformPlay for why we use kEnabled for idle animations.
  if (play_state == V8AnimationPlayState::Enum::kIdle) {
    auto_rewind = Animation::AutoRewind::kEnabled;
  }
  if (animation.EffectivePlaybackRate() > 0) {
    animation.PlayInternal(auto_rewind, exception_state);
  } else {
    animation.ReverseInternal(auto_rewind, exception_state);
  }
  DCHECK_GT(animation.EffectivePlaybackRate(), 0);

  V8AnimationPlayState::Enum new_play_state =
      animation.CalculateAnimationPlayState();
  DCHECK(animation.PendingInternal() ||
         new_play_state == V8AnimationPlayState::Enum::kRunning ||
         new_play_state == V8AnimationPlayState::Enum::kFinished);
}

void PerformPlayBackwards(Animation& animation,
                          V8AnimationPlayState::Enum play_state,
                          ExceptionState& exception_state) {
  Animation::AutoRewind auto_rewind = Animation::AutoRewind::kDisabled;
  // See PerformPlay for why we use kEnabled for idle animations.
  if (play_state == V8AnimationPlayState::Enum::kIdle) {
    auto_rewind = Animation::AutoRewind::kEnabled;
  }
  if (animation.EffectivePlaybackRate() < 0) {
    animation.PlayInternal(auto_rewind, exception_state);
  } else {
    animation.ReverseInternal(auto_rewind, exception_state);
  }
  DCHECK_LT(animation.EffectivePlaybackRate(), 0);

  V8AnimationPlayState::Enum new_play_state =
      animation.CalculateAnimationPlayState();
  DCHECK(animation.PendingInternal() ||
         new_play_state == V8AnimationPlayState::Enum::kRunning ||
         new_play_state == V8AnimationPlayState::Enum::kFinished);
}

void PerformPlayOnce(Animation& animation,
                     V8AnimationPlayState::Enum play_state,
                     ExceptionState& exception_state) {
  if (play_state != V8AnimationPlayState::Enum::kFinished) {
    PerformPlay(animation, play_state, exception_state);
  }
}

void PerformReset(Animation& animation,
                  V8AnimationPlayState::Enum play_state,
                  ExceptionState& exception_state) {
  animation.ResetPlayback();
}

void PerformReplay(Animation& animation,
                   ExceptionState& exception_state) {
  animation.PlayInternal(Animation::AutoRewind::kForced, exception_state);
}
}  // namespace

// static
void AnimationTrigger::PerformBehavior(
    Animation& animation,
    Behavior behavior,
    ExceptionState& exception_state) {
  ScriptForbiddenScope forbid_script;
  V8AnimationPlayState::Enum play_state =
      animation.CalculateAnimationPlayState();
  switch (behavior) {
    case Behavior::kPlay:
      PerformPlay(animation, play_state, exception_state);
      break;
    case Behavior::kPause:
      PerformPause(animation, play_state, exception_state);
      break;
    case Behavior::kPlayForwards:
      PerformPlayForwards(animation, play_state, exception_state);
      break;
    case Behavior::kPlayBackwards:
      PerformPlayBackwards(animation, play_state, exception_state);
      break;
    case Behavior::kPlayOnce:
      PerformPlayOnce(animation, play_state, exception_state);
      break;
    case Behavior::kReset:
      PerformReset(animation, play_state, exception_state);
      break;
    case Behavior::kReplay:
      PerformReplay(animation, exception_state);
      break;
    case Behavior::kNone:
      break;
    default:
      NOTREACHED();
  };
}

// static
bool AnimationTrigger::HasPausedCSSPlayState(Animation* animation) {
  if (!animation->IsCSSAnimation()) {
    return false;
  }

  CSSAnimation* css_animation = To<CSSAnimation>(animation);

  if (css_animation->GetIgnoreCSSPlayState()) {
    return false;
  }

  return animation->GetTriggerActionPlayState() == EAnimPlayState::kPaused;
}

void AnimationTrigger::addAnimation(
    Animation* animation,
    V8AnimationTriggerBehavior activate_behavior,
    V8AnimationTriggerBehavior deactivate_behavior,
    ExceptionState& exception_state) {
  CHECK(!is_activating_or_deactivating_);

  if (!animation) {
    return;
  }

  const HeapHashSet<WeakMember<AnimationTrigger>>& animation_triggers =
      animation->GetTriggers();
  if (!animation_triggers.empty() && !animation_triggers.Contains(this)) {
    // TODO(crbug.com/474398437): Support multiple triggers per animation when
    // the working group resolevs to do so:
    // https://github.com/w3c/csswg-drafts/issues/12399#issuecomment-3089703026
    exception_state.ThrowDOMException(
        DOMExceptionCode::kNotSupportedError,
        "Attaching multiple triggers to an animation is not allowed.");
  }

  WillAddAnimation(animation, activate_behavior.AsEnum(),
                   deactivate_behavior.AsEnum(), exception_state);
  if (exception_state.HadException()) {
    return;
  }

  UpdateBehaviorMap(*animation, activate_behavior.AsEnum(),
                    deactivate_behavior.AsEnum());
  animation->AddTrigger(this);

  DidAddAnimation();
}

void AnimationTrigger::removeAnimation(Animation* animation) {
  CHECK(!is_activating_or_deactivating_);

  if (!animation) {
    return;
  }

  animation->RemoveTrigger(this);
  animation_behavior_map_.erase(animation);
  DidRemoveAnimation(animation);
}

HeapVector<Member<Animation>> AnimationTrigger::getAnimations() {
  HeapVector<Member<Animation>> animations;

  for (auto& [animation, behaviors] : animation_behavior_map_) {
    animations.push_back(animation);
  }

  std::sort(animations.begin(), animations.end(), Animation::CompareAnimations);
  return animations;
}

bool AnimationTrigger::IsTimelineTrigger() const {
  return false;
}

bool AnimationTrigger::IsEventTrigger() const {
  return false;
}

void AnimationTrigger::WillAddAnimation(Animation* animation,
                                        Behavior enter_behavior,
                                        Behavior exit_behavior,
                                        ExceptionState& exception_state) {}

void AnimationTrigger::DidAddAnimation() {}

void AnimationTrigger::DidRemoveAnimation(Animation* animation) {}

void AnimationTrigger::UpdateBehaviorMap(Animation& animation,
                                         Behavior activate_behavior,
                                         Behavior deactivate_behavior) {
  animation_behavior_map_.Set(
      &animation, std::make_pair<>(activate_behavior, deactivate_behavior));
}

void AnimationTrigger::PerformActivate() {
  base::AutoReset<bool> is_activating(&is_activating_or_deactivating_, true);
  for (auto [animation, behaviors] : animation_behavior_map_) {
    if (HasPausedCSSPlayState(animation))
      continue;
    PerformBehavior(*animation, behaviors.first, ASSERT_NO_EXCEPTION);
  }
}

void AnimationTrigger::PerformDeactivate() {
  base::AutoReset<bool> is_deactivating(&is_activating_or_deactivating_, true);
  for (auto [animation, behaviors] : animation_behavior_map_) {
    if (HasPausedCSSPlayState(animation))
      continue;
    PerformBehavior(*animation, behaviors.second, ASSERT_NO_EXCEPTION);
  }
}

void AnimationTrigger::Trace(Visitor* visitor) const {
  visitor->Trace(animation_behavior_map_);
  ScriptWrappable::Trace(visitor);
}
}  // namespace blink
