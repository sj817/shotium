// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_CSS_TRANSFORM_KEYFRAME_SNAPSHOT_FACTORY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_CSS_TRANSFORM_KEYFRAME_SNAPSHOT_FACTORY_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator/allocator.h"

namespace blink {

class TransformKeyframeSnapshot;
class ComputedStyle;
class PropertyHandle;

class CORE_EXPORT TransformKeyframeSnapshotFactory {
  STATIC_ONLY(TransformKeyframeSnapshotFactory);

 public:
  static bool RequiresSnapshot(const PropertyHandle&);
  static TransformKeyframeSnapshot* Create(const PropertyHandle&,
                                           const ComputedStyle&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_CSS_TRANSFORM_KEYFRAME_SNAPSHOT_FACTORY_H_
