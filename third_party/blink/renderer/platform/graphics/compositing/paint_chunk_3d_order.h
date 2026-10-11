// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITING_PAINT_CHUNK_3D_ORDER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITING_PAINT_CHUNK_3D_ORDER_H_

#include "base/containers/span.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "ui/gfx/geometry/point3_f.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/transform.h"

namespace blink {

struct PaintChunk3DPlane {
  gfx::RectF bounds;
  gfx::Transform transform;
};

struct PaintChunk3DFragment {
  size_t plane_index;
  Vector<gfx::Point3F> vertices;
  bool split = false;
};

// CPU equivalent of viz's DrawPolygon/BspTree ordering. Vertices are in the
// destination space, including perspective division and clipping at w = 0.
// A split plane is replayed through each returned polygon's clip; coplanar
// planes retain their input paint order.
Vector<PaintChunk3DFragment> OrderPaintChunk3DPlanes(
    base::span<const PaintChunk3DPlane> planes);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITING_PAINT_CHUNK_3D_ORDER_H_
