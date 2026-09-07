// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SCROLL_SCROLL_ANIMATION_STATE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SCROLL_SCROLL_ANIMATION_STATE_H_

#include <memory>

#include "base/time/time.h"
#include "cc/animation/scroll_offset_animation_curve.h"
#include "cc/trees/scroll_source_type.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/scroll/scroll_types.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"

namespace blink {

class ScrollableArea;

// Shared lifecycle and coordinate conversion for CPU scroll animations.
class CORE_EXPORT ScrollAnimationState
    : public GarbageCollected<ScrollAnimationState> {
 public:
  enum class RunState {
    kIdle,
    kWaitingToStart,
    kRunningOnMainThread,
    kPostAnimationCleanup,
  };

  ScrollAnimationState(const ScrollAnimationState&) = delete;
  ScrollAnimationState& operator=(const ScrollAnimationState&) = delete;
  virtual ~ScrollAnimationState();

  bool HasAnimationThatRequiresService() const;
  void DetachElement();
  virtual void ResetAnimationState();
  virtual void CancelAnimation();
  virtual void UpdateAnimationState() = 0;
  virtual ScrollableArea* GetScrollableArea() const = 0;
  virtual void TickAnimation(base::TimeTicks monotonic_time) = 0;

  bool HasRunningAnimation() const {
    return run_state_ != RunState::kPostAnimationCleanup &&
           (animation_curve_ || run_state_ == RunState::kWaitingToStart);
  }

  virtual void Trace(Visitor* visitor) const {}

 protected:
  ScrollAnimationState();
  void ScrollOffsetChanged(const ScrollOffset&,
                           mojom::blink::ScrollType,
                           cc::ScrollSourceType);

  // Curves use positions measured from the top-left of the overflow rect.
  // Blink offsets instead follow flow direction, which differs for RTL and
  // vertical-rl content. Keep this conversion for CPU interpolation.
  gfx::PointF PositionFromOffset(ScrollOffset);
  ScrollOffset OffsetFromPosition(gfx::PointF);

  RunState run_state_ = RunState::kIdle;
  std::unique_ptr<cc::ScrollOffsetAnimationCurve> animation_curve_;

 private:
  bool element_detached_ = false;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_SCROLL_SCROLL_ANIMATION_STATE_H_
