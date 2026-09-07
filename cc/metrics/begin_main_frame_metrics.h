// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_METRICS_BEGIN_MAIN_FRAME_METRICS_H_
#define CC_METRICS_BEGIN_MAIN_FRAME_METRICS_H_

namespace cc {

// Reason that a BeginMainFrame was triggered. Used for metrics only,
// specifically: |Compositing.BeginMainFrame.BMFReason*|.
enum class BeginMainFrameReason {
  // Catch-all bucket for anything unclassified.
  kOther = 0,
  // ServiceScriptedAnimations almost always occurs as a result of RAF, so
  // these two can be grouped together.
  kRAFOrServiceScriptedAnimations = 1,
  kRAF = kRAFOrServiceScriptedAnimations,
  kServiceScriptedAnimations = kRAFOrServiceScriptedAnimations,
  kVideoFrameCallback = 2,
  kAnimation = 3,
  kCSSAnimation = kAnimation,
  // These three are relatively infrequent, so group them all together for now.
  kStylePaintOrLayoutInvalidation = 4,
  kStyleInvalidation = kStylePaintOrLayoutInvalidation,
  kPaintInvalidation = kStylePaintOrLayoutInvalidation,
  kLayoutInvalidation = kStylePaintOrLayoutInvalidation,
  kScroll = 5,
  kInput = 6,
  kMainThreadScroll = 7,
  kDelayedTimerFired = 8,
  kMaxValue = kDelayedTimerFired,
};

}  // namespace cc

#endif  // CC_METRICS_BEGIN_MAIN_FRAME_METRICS_H_
