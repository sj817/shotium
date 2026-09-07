// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/scroll/programmatic_scroll_animator.h"

#include <memory>

#include "cc/animation/scroll_offset_animation_curve_factory.h"
#include "third_party/blink/renderer/core/scroll/scrollable_area.h"
#include "ui/gfx/geometry/size.h"

namespace blink {

ProgrammaticScrollAnimator::ProgrammaticScrollAnimator(
    ScrollableArea* scrollable_area)
    : scrollable_area_(scrollable_area) {}

ProgrammaticScrollAnimator::~ProgrammaticScrollAnimator() = default;

void ProgrammaticScrollAnimator::Dispose() {
  if (on_finish_) {
    std::move(on_finish_).Run(ScrollableArea::ScrollCompletionMode::kFinished);
  }
}

void ProgrammaticScrollAnimator::ResetAnimationState() {
  ScrollAnimationState::ResetAnimationState();
  animation_curve_.reset();
  start_time_ = base::TimeTicks();
  if (on_finish_)
    std::move(on_finish_).Run(ScrollableArea::ScrollCompletionMode::kFinished);
}

mojom::blink::ScrollType ProgrammaticScrollAnimator::GetScrollType() const {
  return mojom::blink::ScrollType::kProgrammatic;
}

void ProgrammaticScrollAnimator::ScrollToOffsetWithoutAnimation(
    const ScrollOffset& offset,
    cc::ScrollSourceType source_type) {
  if (on_finish_) {
    std::move(on_finish_)
        .Run(ScrollableArea::ScrollCompletionMode::kInterruptedByScroll);
  }
  CancelAnimation();
  source_type_ = source_type;
  ScrollOffsetChanged(offset, GetScrollType(), source_type);
}

void ProgrammaticScrollAnimator::AnimateToOffset(
    const ScrollOffset& offset,
    cc::ScrollSourceType source_type,
    ScrollableArea::ScrollCallback on_finish) {
  if (run_state_ == RunState::kPostAnimationCleanup) {
    ResetAnimationState();
  }

  if (on_finish_) {
    std::move(on_finish_)
        .Run(ScrollableArea::ScrollCompletionMode::kInterruptedByScroll);
  }
  on_finish_ = std::move(on_finish);
  // Ideally, if an ongoing animation exists when we receive a request to
  // animate to a different offset, instead of cancelling the current
  // animation, we could retarget the current animation to the new
  // scroll offset, keeping the velocity of the current animation.
  // When doing this, we'd need to be careful to handle the possibility
  // of repeatedly retargeting to a drastically different location such that
  // the scroll never settles.
  if (animation_curve_ && target_offset_ == offset) {
    return;
  }
  start_time_ = base::TimeTicks();
  target_offset_ = offset;
  source_type_ = source_type;

  animation_curve_ = cc::ScrollOffsetAnimationCurveFactory::CreateAnimation(
      PositionFromOffset(target_offset_),
      cc::ScrollOffsetAnimationCurve::ScrollType::kProgrammatic);

  scrollable_area_->RegisterForAnimation();
  if (!scrollable_area_->ScheduleAnimation()) {
    ResetAnimationState();
    ScrollOffsetChanged(offset, GetScrollType(), source_type);
    return;
  }
  run_state_ = RunState::kWaitingToStart;
}

void ProgrammaticScrollAnimator::CancelAnimation() {
  ScrollAnimationState::CancelAnimation();
  if (on_finish_) {
    std::move(on_finish_)
        .Run(ScrollableArea::ScrollCompletionMode::kInterruptedByScroll);
  }
}

void ProgrammaticScrollAnimator::TickAnimation(base::TimeTicks monotonic_time) {
  if (run_state_ != RunState::kRunningOnMainThread)
    return;

  if (start_time_ == base::TimeTicks())
    start_time_ = monotonic_time;
  base::TimeDelta elapsed_time = monotonic_time - start_time_;
  bool is_finished = (elapsed_time > animation_curve_->Duration());
  ScrollOffset offset =
      OffsetFromPosition(animation_curve_->GetValue(elapsed_time));
  ScrollOffsetChanged(offset, GetScrollType(), source_type_);

  if (is_finished) {
    run_state_ = RunState::kPostAnimationCleanup;
    AnimationFinished();
  } else if (!scrollable_area_->ScheduleAnimation()) {
    ResetAnimationState();
  }
}

void ProgrammaticScrollAnimator::UpdateAnimationState() {
  if (run_state_ == RunState::kPostAnimationCleanup) {
    ResetAnimationState();
    return;
  }
  if (run_state_ == RunState::kWaitingToStart) {
    run_state_ = RunState::kRunningOnMainThread;
    animation_curve_->SetInitialValue(
        PositionFromOffset(scrollable_area_->GetScrollOffset()));
    if (!scrollable_area_->ScheduleAnimation()) {
      ScrollOffsetChanged(target_offset_, GetScrollType(), source_type_);
      ResetAnimationState();
    }
  }
}

void ProgrammaticScrollAnimator::AnimationFinished() {
  if (on_finish_)
    std::move(on_finish_).Run(ScrollableArea::ScrollCompletionMode::kFinished);
}

void ProgrammaticScrollAnimator::Trace(Visitor* visitor) const {
  visitor->Trace(scrollable_area_);
  ScrollAnimationState::Trace(visitor);
}

}  // namespace blink
