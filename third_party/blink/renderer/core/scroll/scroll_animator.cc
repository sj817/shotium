/*
 * Copyright (c) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/scroll/scroll_animator.h"

#include <memory>

#include "base/functional/callback_helpers.h"
#include "build/build_config.h"
#include "cc/animation/scroll_offset_animation_curve_factory.h"
#include "third_party/blink/renderer/core/scroll/scrollable_area.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"

#include "base/win/windows_h_disallowed.h"

namespace blink {

ScrollAnimatorBase* ScrollAnimatorBase::Create(
    ScrollableArea* scrollable_area) {
  if (scrollable_area && scrollable_area->ScrollAnimatorEnabled())
    return MakeGarbageCollected<ScrollAnimator>(scrollable_area);
  return MakeGarbageCollected<ScrollAnimatorBase>(scrollable_area);
}

ScrollAnimator::ScrollAnimator(ScrollableArea* scrollable_area,
                               const base::TickClock* tick_clock)
    : ScrollAnimatorBase(scrollable_area),
      tick_clock_(tick_clock),
      last_granularity_(ui::ScrollGranularity::kScrollByPixel) {}

ScrollAnimator::~ScrollAnimator() {
  if (on_finish_) {
    std::move(on_finish_).Run(ScrollableArea::ScrollCompletionMode::kFinished);
  }
}

ScrollOffset ScrollAnimator::DesiredTargetOffset() const {
  return (animation_curve_ ||
          run_state_ == RunState::kWaitingToStart)
             ? target_offset_
             : CurrentOffset();
}

ScrollOffset ScrollAnimator::ComputeDeltaToConsume(
    const ScrollOffset& delta) const {
  ScrollOffset pos = DesiredTargetOffset();
  ScrollOffset new_pos = scrollable_area_->ClampScrollOffset(pos + delta);
  return new_pos - pos;
}

void ScrollAnimator::ResetAnimationState() {
  ScrollAnimationState::ResetAnimationState();
  if (animation_curve_)
    animation_curve_.reset();
  start_time_ = base::TimeTicks();
  if (on_finish_)
    std::move(on_finish_).Run(ScrollableArea::ScrollCompletionMode::kFinished);
}

ScrollConsumption ScrollAnimator::UserScroll(
    ui::ScrollGranularity granularity,
    const ScrollOffset& delta,
    cc::ScrollSourceType source_type,
    ScrollableArea::ScrollCallback on_finish) {
  // We only store on_finish_ when running an animation, and it should be
  // invoked as soon as the animation is finished. If we don't animate the
  // scroll, the callback is invoked immediately without being stored.
  DCHECK(HasRunningAnimation() || on_finish_.is_null());

  ScrollableArea::ScrollCallback run_on_return(blink::BindOnce(
      [](ScrollableArea::ScrollCallback callback,
         ScrollableArea::ScrollCompletionMode mode) {
        if (callback) {
          std::move(callback).Run(mode);
        }
      },
      std::move(on_finish)));

  if (!scrollable_area_->ScrollAnimatorEnabled() ||
      granularity == ui::ScrollGranularity::kScrollByPrecisePixel) {
    // Cancel scroll animation because asked to instant scroll.
    if (HasRunningAnimation())
      CancelAnimation();
    return ScrollAnimatorBase::UserScroll(granularity, delta, source_type,
                                          std::move(run_on_return));
  }

  TRACE_EVENT0("blink", "ScrollAnimator::scroll");

  bool needs_post_animation_cleanup =
      run_state_ == RunState::kPostAnimationCleanup;
  if (run_state_ == RunState::kPostAnimationCleanup)
    ResetAnimationState();

  ScrollOffset consumed_delta = ComputeDeltaToConsume(delta);
  ScrollOffset target_offset = DesiredTargetOffset();
  target_offset += consumed_delta;

  if (WillAnimateToOffset(target_offset)) {
    source_type_ = source_type;
    last_granularity_ = granularity;
    if (on_finish_) {
      std::move(on_finish_)
          .Run(ScrollableArea::ScrollCompletionMode::kInterruptedByScroll);
    }
    on_finish_ = std::move(run_on_return);
    // Report unused delta only if there is no animation running. See
    // comment below regarding scroll latching.
    // TODO(bokan): Need to standardize how ScrollAnimators report
    // unusedDelta. This differs from ScrollAnimatorMac currently.
    return ScrollConsumption(true, true, 0, 0);
  }

  // Keep a pending cleanup until the lifecycle services animation state.
  if (needs_post_animation_cleanup)
    run_state_ = RunState::kPostAnimationCleanup;

  // Report unused delta only if there is no animation and we are not
  // starting one. This ensures we latch for the duration of the
  // animation rather than animating multiple scrollers at the same time.
  if (on_finish_)
    std::move(on_finish_).Run(ScrollableArea::ScrollCompletionMode::kFinished);

  std::move(run_on_return).Run(ScrollableArea::ScrollCompletionMode::kFinished);
  return ScrollConsumption(false, false, delta.x(), delta.y());
}

bool ScrollAnimator::WillAnimateToOffset(const ScrollOffset& target_offset) {
  if (run_state_ == RunState::kPostAnimationCleanup)
    ResetAnimationState();

  if (animation_curve_) {
    if ((target_offset - target_offset_).IsZero())
      return true;
    target_offset_ = target_offset;
    DCHECK_EQ(run_state_, RunState::kRunningOnMainThread);
    animation_curve_->UpdateTarget(tick_clock_->NowTicks() - start_time_,
                                    PositionFromOffset(target_offset));
    GetScrollableArea()->ScheduleAnimation();
    return true;
  }

  if ((target_offset - CurrentOffset()).IsZero())
    return false;

  target_offset_ = target_offset;
  start_time_ = tick_clock_->NowTicks();
  if (RegisterAndScheduleAnimation())
    run_state_ = RunState::kWaitingToStart;
  return true;
}

void ScrollAnimator::AdjustAnimation(const gfx::Vector2d& adjustment) {
  if (HasRunningAnimation()) {
    target_offset_ += ScrollOffset(adjustment);
    if (animation_curve_)
      animation_curve_->ApplyAdjustment(adjustment);
  }
}

void ScrollAnimator::ScrollToOffsetWithoutAnimation(
    const ScrollOffset& offset,
    cc::ScrollSourceType source_type) {
  current_offset_ = offset;
  source_type_ = source_type;

  ResetAnimationState();
  ScrollOffsetChanged(current_offset_, mojom::blink::ScrollType::kUser,
                      source_type);
}

void ScrollAnimator::TickAnimation(base::TimeTicks monotonic_time) {
  if (run_state_ != RunState::kRunningOnMainThread)
    return;

  TRACE_EVENT0("blink", "ScrollAnimator::tickAnimation");
  base::TimeDelta elapsed_time = monotonic_time - start_time_;

  bool is_finished = (elapsed_time > animation_curve_->Duration());
  ScrollOffset offset = OffsetFromPosition(
      is_finished ? animation_curve_->target_value()
                  : animation_curve_->GetValue(elapsed_time));

  offset = scrollable_area_->ClampScrollOffset(offset);

  current_offset_ = offset;

  if (is_finished) {
    run_state_ = RunState::kPostAnimationCleanup;
    if (on_finish_) {
      std::move(on_finish_)
          .Run(ScrollableArea::ScrollCompletionMode::kFinished);
    }
  } else {
    GetScrollableArea()->ScheduleAnimation();
  }

  TRACE_EVENT0("blink", "ScrollAnimator::notifyOffsetChanged");
  ScrollOffsetChanged(current_offset_, mojom::blink::ScrollType::kUser,
                      source_type_);
}

void ScrollAnimator::CreateAnimationCurve() {
  DCHECK(!animation_curve_);
  // It is not correct to assume the input type from the granularity, but we've
  // historically determined animation parameters from granularity.
  cc::ScrollOffsetAnimationCurve::ScrollType scroll_type =
      (last_granularity_ == ui::ScrollGranularity::kScrollByPixel)
          ? cc::ScrollOffsetAnimationCurve::ScrollType::kMouseWheel
          : cc::ScrollOffsetAnimationCurve::ScrollType::kKeyboard;
  animation_curve_ = cc::ScrollOffsetAnimationCurveFactory::CreateAnimation(
      PositionFromOffset(target_offset_), scroll_type);
  animation_curve_->SetInitialValue(
      PositionFromOffset(CurrentOffset()));
}

void ScrollAnimator::UpdateAnimationState() {
  if (run_state_ == RunState::kPostAnimationCleanup) {
    ResetAnimationState();
    return;
  }
  if (run_state_ == RunState::kWaitingToStart) {
    if (!animation_curve_)
      CreateAnimationCurve();
    if (RegisterAndScheduleAnimation())
      run_state_ = RunState::kRunningOnMainThread;
  }
}

void ScrollAnimator::CancelAnimation() {
  ScrollAnimationState::CancelAnimation();
  if (on_finish_)
    std::move(on_finish_).Run(ScrollableArea::ScrollCompletionMode::kFinished);
}

bool ScrollAnimator::RegisterAndScheduleAnimation() {
  GetScrollableArea()->RegisterForAnimation();
  if (!scrollable_area_->ScheduleAnimation()) {
    ScrollToOffsetWithoutAnimation(target_offset_, source_type_);
    ResetAnimationState();
    return false;
  }
  return true;
}

void ScrollAnimator::Trace(Visitor* visitor) const {
  ScrollAnimatorBase::Trace(visitor);
}

}  // namespace blink
