// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_CSS_TRANSFORM_KEYFRAME_SNAPSHOT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_CSS_TRANSFORM_KEYFRAME_SNAPSHOT_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/transforms/transform_operations.h"

namespace blink {

// Resolved keyframe geometry used by CPU paint to preserve subpixel placement
// and determine whether animated transforms preserve axis alignment.
class CORE_EXPORT TransformKeyframeSnapshot final
    : public GarbageCollected<TransformKeyframeSnapshot> {
 public:
  explicit TransformKeyframeSnapshot(const TransformOperations& transform)
      : transform_(transform) {}

  void Trace(Visitor* visitor) const { visitor->Trace(transform_); }
  const TransformOperations& GetTransformOperations() const {
    return transform_;
  }

 private:
  const TransformOperations transform_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_CSS_TRANSFORM_KEYFRAME_SNAPSHOT_H_
