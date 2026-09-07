// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/video_painter.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/renderer/core/layout/layout_video.h"
#include "third_party/blink/renderer/core/paint/box_painter.h"
#include "third_party/blink/renderer/core/paint/image_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"

namespace blink {

void VideoPainter::PaintReplaced(const PaintInfo& paint_info,
                                 const PhysicalOffset& paint_offset) {
  if (paint_info.phase != PaintPhase::kForeground &&
      paint_info.phase != PaintPhase::kSelectionDragImage)
    return;

  bool force_video_poster =
      layout_video_.GetDocument().GetPaintPreviewState() ==
      Document::kPaintingPreviewSkipAcceleratedContent;
  bool should_display_poster =
      layout_video_.GetDisplayMode() == LayoutVideo::kPoster ||
      force_video_poster;
  if (!should_display_poster)
    return;

  if (paint_info.IsPrivacyPreserving() &&
      !layout_video_.ImageResource()->IsCorsSameOrigin()) {
    return;
  }

  PhysicalRect replaced_rect = layout_video_.ReplacedContentRect();
  replaced_rect.Move(paint_offset);
  gfx::Rect snapped_replaced_rect = ToPixelSnappedRect(replaced_rect);

  if (snapped_replaced_rect.IsEmpty())
    return;

  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, layout_video_, paint_info.phase))
    return;

  GraphicsContext& context = paint_info.context;
  // Poster images participate in first-contentful-paint like other images.
  context.GetPaintController().SetImagePainted();
  PhysicalRect content_box_rect = layout_video_.PhysicalContentBoxRect();
  content_box_rect.Move(paint_offset);

  if (layout_video_.GetDocument().GetPaintPreviewState() !=
      Document::kNotPaintingPreview) {
    // Create a canvas and draw a URL rect to it for the paint preview.
    BoxDrawingRecorder recorder(context, layout_video_, paint_info.phase,
                                paint_offset);
    context.SetURLForRect(layout_video_.GetDocument().Url(),
                          snapped_replaced_rect);
  }

  const PhysicalRect visual_rect =
      layout_video_.ClipsToContentBox() ? content_box_rect : replaced_rect;

  DrawingRecorder recorder(context, layout_video_, paint_info.phase,
                           ToEnclosingRect(visual_rect));

  ImagePainter(layout_video_)
      .PaintIntoRect(context, replaced_rect, visual_rect);
}

}  // namespace blink
