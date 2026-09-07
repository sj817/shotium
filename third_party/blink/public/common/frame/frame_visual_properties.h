// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_FRAME_FRAME_VISUAL_PROPERTIES_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_FRAME_FRAME_VISUAL_PROPERTIES_H_

#include "third_party/blink/public/common/common_export.h"

namespace blink {

// Geometry stability thresholds used by FrameView for iframe visibility.
struct BLINK_COMMON_EXPORT FrameVisualProperties {
  static double MaxChildFrameScreenRectMovement();
  static int MinScreenRectStableTimeMs();

  // TODO(szager): These values override the above two values for frames that
  // utilize IntersectionObserver V2 (i.e. occlusion detection). The purpose of
  // this specialization is to preserve existing behavior while the above two
  // parameters are experimentally dialed in.
  // See kTargetFrameMovedRecentlyForIOv2 in web_input_event.h.
  static int MaxChildFrameScreenRectMovementForIOv2();
  static int MinScreenRectStableTimeMsForIOv2();

};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_FRAME_FRAME_VISUAL_PROPERTIES_H_
