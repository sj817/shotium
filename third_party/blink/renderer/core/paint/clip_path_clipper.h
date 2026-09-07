// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_CLIP_PATH_CLIPPER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_CLIP_PATH_CLIPPER_H_

#include <optional>

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/style/clip_path_operation.h"
#include "third_party/blink/renderer/core/paint/paint_flags.h"
#include "third_party/blink/renderer/platform/geometry/contoured_rect.h"
#include "third_party/blink/renderer/platform/geometry/path.h"
#include "ui/gfx/geometry/rect_f.h"

namespace blink {

class DisplayItemClient;
class GraphicsContext;
class HitTestLocation;
class LayoutObject;

class CORE_EXPORT ClipPathClipper {
  STATIC_ONLY(ClipPathClipper);

 public:
  static bool UsesZoomedReferenceBox(const LayoutObject& clip_path_owner);

  static ContouredRect RoundedReferenceBox(GeometryBox geometry_box,
                                           const LayoutObject& object);

  static void PaintClipPathAsMaskImage(GraphicsContext&,
                                       const LayoutObject&,
                                       const DisplayItemClient&,
                                       PaintFlags);

  // Returns the local reference box for a given operation. Useful for
  // when the desired operation is already known, or clip-path is not currently
  // set in style (e.g. with a cc clip path animation.)
  static gfx::RectF CalcLocalReferenceBox(
      const LayoutObject& object,
      const ClipPathOperation::OperationType clip_path_operation,
      GeometryBox geometry_box);

  // Returns the reference box used by CSS clip-path.
  static gfx::RectF LocalReferenceBox(const LayoutObject&);

  // Returns the bounding box of the computed clip path, which could be
  // smaller or bigger than the reference box. Returns nullopt if the
  // clip path is invalid.
  static std::optional<gfx::RectF> LocalClipPathBoundingBox(
      const LayoutObject&);

  // The argument |clip_path_owner| is the layout object that owns the
  // ClipPathOperation we are currently processing. Usually it is the
  // same as the layout object getting clipped, but in the case of nested
  // clip-path, it could be one of the SVG clip path in the chain.
  // Returns the path if the clip-path can use path-based clip.
  static std::optional<Path> PathBasedClip(const LayoutObject& clip_path_owner,
                                           const gfx::Vector2dF& clip_offset);

  // Returns true if `location` intersects the `clip_path_owner`'s clip-path.
  // `reference_box`, which should be calculated from `reference_box_object`, is
  // used to resolve 'objectBoundingBox' units/percentages.
  static bool HitTest(const LayoutObject& clip_path_owner,
                      const gfx::RectF& reference_box,
                      const LayoutObject& reference_box_object,
                      const HitTestLocation& location);

  // Like the above, but derives the reference box from the LayoutObject using
  // `LocalReferenceBox()`.
  static bool HitTest(const LayoutObject&, const HitTestLocation& location);

 private:
  static std::optional<Path> PathBasedClipInternal(
      const LayoutObject& clip_path_owner,
      const gfx::RectF& reference_box,
      const LayoutObject& reference_box_object,
      const gfx::Vector2dF& clip_offset);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_CLIP_PATH_CLIPPER_H_
