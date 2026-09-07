// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/scroll/scroll_animation_state.h"

#include "third_party/blink/renderer/core/scroll/scrollable_area.h"

namespace blink {

ScrollAnimationState::ScrollAnimationState() = default;
ScrollAnimationState::~ScrollAnimationState() = default;

void ScrollAnimationState::DetachElement() {
  DCHECK(!element_detached_);
  if (RuntimeEnabledFeatures::ProgrammaticScrollPromiseEnabled()) {
    CancelAnimation();
  }
  element_detached_ = true;
}

void ScrollAnimationState::ResetAnimationState() {
  run_state_ = RunState::kIdle;
}

bool ScrollAnimationState::HasAnimationThatRequiresService() const {
  return run_state_ != RunState::kIdle;
}

void ScrollAnimationState::CancelAnimation() {
  switch (run_state_) {
    case RunState::kIdle:
    case RunState::kPostAnimationCleanup:
      break;
    case RunState::kWaitingToStart:
      ResetAnimationState();
      break;
    case RunState::kRunningOnMainThread:
      run_state_ = RunState::kPostAnimationCleanup;
      break;
  }
}

gfx::PointF ScrollAnimationState::PositionFromOffset(ScrollOffset offset) {
  return GetScrollableArea()->ScrollOffsetToPosition(offset);
}

ScrollOffset ScrollAnimationState::OffsetFromPosition(gfx::PointF position) {
  return GetScrollableArea()->ScrollPositionToOffset(position);
}

void ScrollAnimationState::ScrollOffsetChanged(
    const ScrollOffset& offset,
    mojom::blink::ScrollType scroll_type,
    cc::ScrollSourceType source_type) {
  ScrollOffset clamped_offset = GetScrollableArea()->ClampScrollOffset(offset);
  GetScrollableArea()->ScrollOffsetChanged(clamped_offset, scroll_type,
                                           source_type);
}

}  // namespace blink
