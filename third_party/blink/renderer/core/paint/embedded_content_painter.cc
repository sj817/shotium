// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/embedded_content_painter.h"


#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "third_party/blink/public/mojom/use_counter/metrics/web_feature.mojom-blink.h"
#include "third_party/blink/renderer/core/frame/embedded_content_view.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/box_painter.h"
#include "third_party/blink/renderer/core/paint/object_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/replaced_painter.h"
#include "third_party/blink/renderer/core/paint/scrollable_area_painter.h"
#include "third_party/blink/renderer/platform/geometry/physical_offset.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item_cache_skipper.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "ui/gfx/geometry/point.h"

namespace blink {

void EmbeddedContentPainter::PaintReplaced(const PaintInfo& paint_info,
                                           const PhysicalOffset& paint_offset) {
  EmbeddedContentView* embedded_content_view =
      layout_embedded_content_.GetEmbeddedContentView();
  if (!embedded_content_view)
    return;

  // Apply the translation to offset the content within the object's border-box
  // only if we're not using a transform node for this. If the frame size is
  // frozen then |ReplacedContentTransform| is used instead.
  gfx::Point paint_location;
  if (!layout_embedded_content_.FrozenFrameSize().has_value()) {
    // LINT.IfChange(FramePixelSnapping)
    paint_location = ToRoundedPoint(
        paint_offset + layout_embedded_content_.ReplacedContentRect().offset);
    // LINT.ThenChange(../layout/layout_embedded_content.cc:FramePixelSnapping)
  }

  gfx::Vector2d view_paint_offset = paint_location.OffsetFromOrigin();
  CullRect adjusted_cull_rect;
  if (RuntimeEnabledFeatures::AvoidEmbeddedContentViewLocationEnabled()) {
    if (!paint_info.IntersectsCullRect(
            PhysicalRect(gfx::Rect(embedded_content_view->Size())),
            PhysicalOffset(view_paint_offset))) {
      return;
    }
    // `adjusted_cull_rect` won't be used in Paint().
  } else {
    view_paint_offset -=
        embedded_content_view->DeprecatedLocation().OffsetFromOrigin();
    adjusted_cull_rect = paint_info.GetCullRect();
    adjusted_cull_rect.Move(-view_paint_offset);
  }
  embedded_content_view->Paint(paint_info, adjusted_cull_rect,
                               view_paint_offset);

}

}  // namespace blink
