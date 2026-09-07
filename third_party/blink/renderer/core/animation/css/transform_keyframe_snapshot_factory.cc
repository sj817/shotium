// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/css/transform_keyframe_snapshot_factory.h"

#include "base/notreached.h"
#include "third_party/blink/renderer/core/animation/css/transform_keyframe_snapshot.h"
#include "third_party/blink/renderer/core/animation/property_handle.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

bool TransformKeyframeSnapshotFactory::RequiresSnapshot(
    const PropertyHandle& property) {
  if (property.IsCSSCustomProperty()) {
    return false;
  }
  switch (property.GetCSSProperty().PropertyID()) {
    case CSSPropertyID::kTransform:
    case CSSPropertyID::kTranslate:
    case CSSPropertyID::kRotate:
    case CSSPropertyID::kScale:
      return true;
    default:
      return false;
  }
}

TransformKeyframeSnapshot* TransformKeyframeSnapshotFactory::Create(
    const PropertyHandle& property,
    const ComputedStyle& style) {
  TransformOperation* transform = nullptr;
  switch (property.GetCSSProperty().PropertyID()) {
    case CSSPropertyID::kTransform:
      return MakeGarbageCollected<TransformKeyframeSnapshot>(style.Transform());
    case CSSPropertyID::kTranslate:
      transform = style.Translate();
      break;
    case CSSPropertyID::kRotate:
      transform = style.Rotate();
      break;
    case CSSPropertyID::kScale:
      transform = style.Scale();
      break;
    default:
      NOTREACHED();
  }
  TransformOperations operations;
  if (transform) {
    operations.Operations().push_back(transform);
  }
  return MakeGarbageCollected<TransformKeyframeSnapshot>(operations);
}

}  // namespace blink
