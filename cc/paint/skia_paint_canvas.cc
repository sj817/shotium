// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/skia_paint_canvas.h"

#include <cstdint>
#include <cstring>
#include <utility>

#include "base/containers/span.h"
#include "base/trace_event/trace_event.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/paint_filter.h"
#include "cc/paint/paint_flags.h"
#include "cc/paint/paint_op.h"
#include "cc/paint/paint_op_buffer.h"
#include "cc/paint/paint_recorder.h"
#include "cc/paint/scoped_raster_flags.h"
#include "skia/ext/legacy_display_globals.h"
#include "third_party/skia/include/core/SkAnnotation.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPoint.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "third_party/skia/include/core/SkTextBlob.h"
#include "third_party/skia/include/docs/SkPDFDocument.h"
#include "third_party/skia/src/core/SkCanvasPriv.h"

namespace cc {
SkiaPaintCanvas::SkiaPaintCanvas(SkCanvas* canvas,
                                 ImageProvider* image_provider)
    : canvas_(canvas),
      image_provider_(image_provider) {}

SkiaPaintCanvas::SkiaPaintCanvas(const SkBitmap& bitmap,
                                 ImageProvider* image_provider)
    : canvas_(new SkCanvas(bitmap,
                           skia::LegacyDisplayGlobals::GetSkSurfaceProps())),
      bitmap_(bitmap),
      owned_(canvas_),
      image_provider_(image_provider) {}

SkiaPaintCanvas::~SkiaPaintCanvas() {
  canvas_ = nullptr;
  image_provider_ = nullptr;
}

SkImageInfo SkiaPaintCanvas::imageInfo() const {
  return canvas_->imageInfo();
}

void* SkiaPaintCanvas::accessTopLayerPixels(SkImageInfo* info,
                                            size_t* rowBytes,
                                            SkIPoint* origin) {
  if (bitmap_.isNull() || bitmap_.isImmutable())
    return nullptr;
  return canvas_->accessTopLayerPixels(info, rowBytes, origin);
}

int SkiaPaintCanvas::save() {
  return canvas_->save();
}

int SkiaPaintCanvas::saveLayer(const PaintFlags& flags) {
  SkPaint paint = flags.ToSkPaint();
  return canvas_->saveLayer(nullptr, &paint);
}

int SkiaPaintCanvas::saveLayer(const SkRect& bounds, const PaintFlags& flags) {
  SkPaint paint = flags.ToSkPaint();
  return canvas_->saveLayer(&bounds, &paint);
}

int SkiaPaintCanvas::saveLayerAlphaf(float alpha) {
  return canvas_->saveLayerAlphaf(nullptr, alpha);
}

int SkiaPaintCanvas::saveLayerAlphaf(const SkRect& bounds, float alpha) {
  return canvas_->saveLayerAlphaf(&bounds, alpha);
}

int SkiaPaintCanvas::saveLayerFilters(
    base::span<const sk_sp<PaintFilter>> filters,
    const PaintFlags& flags) {
  SkPaint paint = flags.ToSkPaint();
  return canvas_->saveLayer(SkCanvasPriv::ScaledBackdropLayer(
      /*bounds=*/nullptr, &paint, /*backdrop=*/nullptr, /*backdropScale=*/1.0f,
      /*saveLayerFlags=*/0, PaintFilter::ToSkImageFilters(filters)));
}

void SkiaPaintCanvas::restore() {
  canvas_->restore();
}

int SkiaPaintCanvas::getSaveCount() const {
  return canvas_->getSaveCount();
}

void SkiaPaintCanvas::restoreToCount(int save_count) {
  canvas_->restoreToCount(save_count);
}

void SkiaPaintCanvas::translate(SkScalar dx, SkScalar dy) {
  canvas_->translate(dx, dy);
}

void SkiaPaintCanvas::scale(SkScalar sx, SkScalar sy) {
  canvas_->scale(sx, sy);
}

void SkiaPaintCanvas::rotate(SkScalar degrees) {
  canvas_->rotate(degrees);
}

void SkiaPaintCanvas::setMatrix(const SkM44& matrix) {
  canvas_->setMatrix(matrix);
}

void SkiaPaintCanvas::concat(const SkM44& matrix) {
  canvas_->concat(matrix);
}

void SkiaPaintCanvas::clipRect(const SkRect& rect,
                               SkClipOp op,
                               bool do_anti_alias) {
  canvas_->clipRect(rect, op, do_anti_alias);
}

void SkiaPaintCanvas::clipRRect(const SkRRect& rrect,
                                SkClipOp op,
                                bool do_anti_alias) {
  canvas_->clipRRect(rrect, op, do_anti_alias);
}

void SkiaPaintCanvas::clipPath(const SkPath& path,
                               SkClipOp op,
                               bool do_anti_alias,
                               UsePaintCache) {
  canvas_->clipPath(path, op, do_anti_alias);
}

bool SkiaPaintCanvas::getLocalClipBounds(SkRect* bounds) const {
  return canvas_->getLocalClipBounds(bounds);
}

bool SkiaPaintCanvas::getDeviceClipBounds(SkIRect* bounds) const {
  return canvas_->getDeviceClipBounds(bounds);
}

void SkiaPaintCanvas::drawColor(SkColor4f color, SkBlendMode mode) {
  canvas_->drawColor(color, mode);
}

void SkiaPaintCanvas::clear(SkColor4f color) {
  canvas_->clear(color);
}

void SkiaPaintCanvas::drawLine(SkScalar x0,
                               SkScalar y0,
                               SkScalar x1,
                               SkScalar y1,
                               const PaintFlags& flags) {
  ScopedRasterFlags raster_flags(&flags, image_provider_,
                                 canvas_->getTotalMatrix(), /*max_texture_size=*/0,
                                 1.0f);
  if (!raster_flags.flags())
    return;

  raster_flags.flags()->DrawToSk(
      canvas_, [x0, y0, x1, y1](SkCanvas* c, const SkPaint& p) {
        c->drawLine(x0, y0, x1, y1, p);
      });
}

void SkiaPaintCanvas::drawArc(const SkRect& oval,
                              SkScalar start_angle_degrees,
                              SkScalar sweep_angle_degrees,
                              const PaintFlags& flags) {
  ScopedRasterFlags raster_flags(&flags, image_provider_,
                                 canvas_->getTotalMatrix(), /*max_texture_size=*/0,
                                 1.0f);
  if (!raster_flags.flags()) {
    return;
  }

  DrawArcOp op(oval, start_angle_degrees, sweep_angle_degrees, flags);
  op.RasterWithFlagsImpl(raster_flags.flags(), canvas_);
}

void SkiaPaintCanvas::drawRect(const SkRect& rect, const PaintFlags& flags) {
  ScopedRasterFlags raster_flags(&flags, image_provider_,
                                 canvas_->getTotalMatrix(), /*max_texture_size=*/0,
                                 1.0f);
  if (!raster_flags.flags())
    return;
  raster_flags.flags()->DrawToSk(
      canvas_,
      [&rect](SkCanvas* c, const SkPaint& p) { c->drawRect(rect, p); });
}

void SkiaPaintCanvas::drawIRect(const SkIRect& rect, const PaintFlags& flags) {
  ScopedRasterFlags raster_flags(&flags, image_provider_,
                                 canvas_->getTotalMatrix(), /*max_texture_size=*/0,
                                 1.0f);
  if (!raster_flags.flags())
    return;
  raster_flags.flags()->DrawToSk(
      canvas_,
      [&rect](SkCanvas* c, const SkPaint& p) { c->drawIRect(rect, p); });
}

void SkiaPaintCanvas::drawOval(const SkRect& oval, const PaintFlags& flags) {
  ScopedRasterFlags raster_flags(&flags, image_provider_,
                                 canvas_->getTotalMatrix(), /*max_texture_size=*/0,
                                 1.0f);
  if (!raster_flags.flags())
    return;
  raster_flags.flags()->DrawToSk(
      canvas_,
      [&oval](SkCanvas* c, const SkPaint& p) { c->drawOval(oval, p); });
}

void SkiaPaintCanvas::drawRRect(const SkRRect& rrect, const PaintFlags& flags) {
  ScopedRasterFlags raster_flags(&flags, image_provider_,
                                 canvas_->getTotalMatrix(), /*max_texture_size=*/0,
                                 1.0f);
  if (!raster_flags.flags())
    return;
  raster_flags.flags()->DrawToSk(
      canvas_,
      [&rrect](SkCanvas* c, const SkPaint& p) { c->drawRRect(rrect, p); });
}

void SkiaPaintCanvas::drawDRRect(const SkRRect& outer,
                                 const SkRRect& inner,
                                 const PaintFlags& flags) {
  ScopedRasterFlags raster_flags(&flags, image_provider_,
                                 canvas_->getTotalMatrix(), /*max_texture_size=*/0,
                                 1.0f);
  if (!raster_flags.flags())
    return;
  raster_flags.flags()->DrawToSk(
      canvas_, [&outer, &inner](SkCanvas* c, const SkPaint& p) {
        c->drawDRRect(outer, inner, p);
      });
}

void SkiaPaintCanvas::drawRoundRect(const SkRect& rect,
                                    SkScalar rx,
                                    SkScalar ry,
                                    const PaintFlags& flags) {
  ScopedRasterFlags raster_flags(&flags, image_provider_,
                                 canvas_->getTotalMatrix(), /*max_texture_size=*/0,
                                 1.0f);
  if (!raster_flags.flags())
    return;
  raster_flags.flags()->DrawToSk(
      canvas_, [&rect, rx, ry](SkCanvas* c, const SkPaint& p) {
        c->drawRoundRect(rect, rx, ry, p);
      });
}

void SkiaPaintCanvas::drawPath(const SkPath& path,
                               const PaintFlags& flags,
                               UsePaintCache) {
  ScopedRasterFlags raster_flags(&flags, image_provider_,
                                 canvas_->getTotalMatrix(), /*max_texture_size=*/0,
                                 1.0f);
  if (!raster_flags.flags())
    return;
  raster_flags.flags()->DrawToSk(
      canvas_,
      [&path](SkCanvas* c, const SkPaint& p) { c->drawPath(path, p); });
}

void SkiaPaintCanvas::drawImage(const PaintImage& image,
                                SkScalar left,
                                SkScalar top,
                                const SkSamplingOptions& sampling,
                                const PaintFlags* flags) {
  std::optional<ScopedRasterFlags> scoped_flags;
  if (flags) {
    scoped_flags.emplace(flags, image_provider_, canvas_->getTotalMatrix(),
                         /*max_texture_size=*/0, 1.0f);
    if (!scoped_flags->flags())
      return;
  }

  const PaintFlags* raster_flags = scoped_flags ? scoped_flags->flags() : flags;
  PlaybackParams params(image_provider_, canvas_->getLocalToDevice());
  DrawImageOp draw_image_op(image, left, top, sampling, nullptr);
  DrawImageOp::RasterWithFlags(&draw_image_op, raster_flags, canvas_, params);
}

void SkiaPaintCanvas::drawImageRect(const PaintImage& image,
                                    const SkRect& src,
                                    const SkRect& dst,
                                    const SkSamplingOptions& sampling,
                                    const PaintFlags* flags,
                                    SkCanvas::SrcRectConstraint constraint) {
  std::optional<ScopedRasterFlags> scoped_flags;
  if (flags) {
    scoped_flags.emplace(flags, image_provider_, canvas_->getTotalMatrix(),
                         /*max_texture_size=*/0, 1.0f);
    if (!scoped_flags->flags())
      return;
  }

  const PaintFlags* raster_flags = scoped_flags ? scoped_flags->flags() : flags;
  PlaybackParams params(image_provider_, canvas_->getLocalToDevice());
  DrawImageRectOp draw_image_rect_op(image, src, dst, sampling, flags,
                                     constraint);
  DrawImageRectOp::RasterWithFlags(&draw_image_rect_op, raster_flags, canvas_,
                                   params);
}

void SkiaPaintCanvas::drawVertices(
    scoped_refptr<RefCountedBuffer<SkPoint>> vertices,
    scoped_refptr<RefCountedBuffer<SkPoint>> uvs,
    scoped_refptr<RefCountedBuffer<uint16_t>> indices,
    const PaintFlags& flags) {
  ScopedRasterFlags raster_flags(&flags, image_provider_,
                                 canvas_->getTotalMatrix(), /*max_texture_size=*/0,
                                 /*alpha=*/1.0f);
  DrawVerticesOp op(std::move(vertices), std::move(uvs), std::move(indices),
                    flags);
  if (!raster_flags.flags() || !op.IsValid()) {
    return;
  }

  PlaybackParams params(image_provider_);
  DrawVerticesOp::RasterWithFlags(&op, raster_flags.flags(), canvas_, params);

}

void SkiaPaintCanvas::drawTextBlob(sk_sp<SkTextBlob> blob,
                                   SkScalar x,
                                   SkScalar y,
                                   const PaintFlags& flags) {
  ScopedRasterFlags raster_flags(&flags, image_provider_,
                                 canvas_->getTotalMatrix(), /*max_texture_size=*/0,
                                 1.0f);
  if (!raster_flags.flags())
    return;
  raster_flags.flags()->DrawToSk(canvas_,
                                 [&blob, x, y](SkCanvas* c, const SkPaint& p) {
                                   c->drawTextBlob(blob, x, y, p);
                                 });
}

void SkiaPaintCanvas::drawTextBlob(sk_sp<SkTextBlob> blob,
                                   SkScalar x,
                                   SkScalar y,
                                   NodeId node_id,
                                   const PaintFlags& flags) {
  if (node_id)
    SkPDF::SetNodeId(canvas_, node_id);
  drawTextBlob(blob, x, y, flags);
  if (node_id)
    SkPDF::SetNodeId(canvas_, 0);
}

void SkiaPaintCanvas::drawPicture(PaintRecord record) {
  drawPicture(std::move(record), PlaybackCallbacks::CustomDataRasterCallback(),
              /*local_ctm=*/true);
}

void SkiaPaintCanvas::drawPicture(PaintRecord record, bool local_ctm) {
  drawPicture(std::move(record), PlaybackCallbacks::CustomDataRasterCallback(),
              local_ctm);
}

SkM44 SkiaPaintCanvas::getLocalToDevice() const {
  return canvas_->getLocalToDevice();
}

void SkiaPaintCanvas::Annotate(AnnotationType type,
                               const SkRect& rect,
                               sk_sp<SkData> data) {
  switch (type) {
    case AnnotationType::kUrl:
      SkAnnotateRectWithURL(canvas_, rect, data.get());
      break;
    case AnnotationType::kLinkToDestination:
      SkAnnotateLinkToDestination(canvas_, rect, data.get());
      break;
    case AnnotationType::kNameDestination: {
      SkPoint point = SkPoint::Make(rect.x(), rect.y());
      SkAnnotateNamedDestination(canvas_, point, data.get());
      break;
    }
  }
}

void SkiaPaintCanvas::setNodeId(int node_id) {
  SkPDF::SetNodeId(canvas_, node_id);
}

void SkiaPaintCanvas::drawPicture(
    PaintRecord record,
    PlaybackCallbacks::CustomDataRasterCallback custom_raster_callback,
    bool local_ctm) {
  PlaybackCallbacks callbacks;
  callbacks.custom_callback = custom_raster_callback;
  PlaybackParams params(image_provider_, canvas_->getLocalToDevice(),
                        callbacks);
  record.Playback(canvas_, params, local_ctm);
}

}  // namespace cc
