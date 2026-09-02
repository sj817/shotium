/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/graphics/generated_image.h"

#include <cmath>
#include <optional>
#include <utility>

#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context_state_saver.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_image.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_record.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_shader.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/skia_conversions.h"

namespace blink {

namespace {

// A generated image has no pixels of its own: `CreateShader()` below records it
// into a picture and hands that to skia as a repeating shader. Skia rasterises
// such a shader by materialising one tile as a bitmap and sampling it, at the
// size the tile occupies on the destination -- which is fine when the tile is a
// swatch repeated many times, and is not fine when the tile is the size of the
// page.
//
// The page-sized case is reached more easily than it looks. The background of
// the root element is painted over the whole canvas but positioned as if on the
// root box, so a document that is wider than the viewport by a single pixel --
// one overflowing element is enough -- turns a full-page gradient into a
// full-page tile repeated twice, and 1920x12000 pixels get rastered into an
// intermediate before anything is drawn. Measured on such a page, that is 2.9x
// the raster time of the same gradient with `no-repeat`, and a 25% larger PNG,
// because the sampled tile dithers differently from a gradient drawn directly.
//
// Drawing the tiles instead costs one draw each and no intermediate at all. It
// is only worth the loop when the tile is large, and only correct to do so
// simply when the lattice is the plain one -- no spacing between tiles, and the
// tile's own origin at zero, which is what a background lays out. Anything else
// keeps the shader.
constexpr float kMinTilePixelsToDrawDirectly = 4000000.0f;
constexpr int kMaxDirectlyDrawnTiles = 4;

// The half-open range of tile indices along one axis whose tiles intersect
// [dest_min, dest_max), for tiles of width `step` starting at `origin`.
// Returns nullopt if the range is not one that can be cast to int, which the
// size checks in the caller make unreachable but which is undefined behaviour
// rather than a large number if it ever stops being.
std::optional<std::pair<int, int>> TileIndexRange(float dest_min,
                                                  float dest_max,
                                                  float origin,
                                                  float step) {
  const float first = std::floor((dest_min - origin) / step);
  const float last = std::ceil((dest_max - origin) / step);
  constexpr float kLimit = 1 << 20;
  if (!std::isfinite(first) || !std::isfinite(last) || first < -kLimit ||
      last > kLimit) {
    return std::nullopt;
  }
  return std::make_pair(static_cast<int>(first), static_cast<int>(last));
}

}  // namespace

bool GeneratedImage::DrawPatternAsTiles(GraphicsContext& dest_context,
                                        const cc::PaintFlags& base_flags,
                                        const gfx::RectF& dest_rect,
                                        const ImageTilingInfo& tiling_info,
                                        const ImageDrawOptions& draw_options) {
  if (!tiling_info.spacing.IsZero() ||
      tiling_info.image_rect.origin() != gfx::PointF()) {
    return false;
  }
  const float step_x = tiling_info.image_rect.width() * tiling_info.scale.x();
  const float step_y = tiling_info.image_rect.height() * tiling_info.scale.y();
  if (!(step_x > 0.0f) || !(step_y > 0.0f)) {
    return false;
  }
  if (step_x * step_y < kMinTilePixelsToDrawDirectly) {
    return false;
  }
  const std::optional<std::pair<int, int>> range_x = TileIndexRange(
      dest_rect.x(), dest_rect.right(), tiling_info.phase.x(), step_x);
  const std::optional<std::pair<int, int>> range_y = TileIndexRange(
      dest_rect.y(), dest_rect.bottom(), tiling_info.phase.y(), step_y);
  if (!range_x || !range_y) {
    return false;
  }
  const auto [first_x, last_x] = *range_x;
  const auto [first_y, last_y] = *range_y;
  const int64_t count = static_cast<int64_t>(last_x - first_x) *
                        static_cast<int64_t>(last_y - first_y);
  if (count <= 0 || count > kMaxDirectlyDrawnTiles) {
    return false;
  }

  GraphicsContextStateSaver saver(dest_context);
  dest_context.Clip(dest_rect);
  for (int j = first_y; j < last_y; ++j) {
    for (int i = first_x; i < last_x; ++i) {
      const gfx::RectF tile_dest(tiling_info.phase.x() + i * step_x,
                                 tiling_info.phase.y() + j * step_y, step_x,
                                 step_y);
      if (!tile_dest.Intersects(dest_rect)) {
        continue;
      }
      Draw(dest_context.Canvas(), base_flags, tile_dest,
           tiling_info.image_rect, draw_options);
    }
  }
  return true;
}

void GeneratedImage::DrawPattern(GraphicsContext& dest_context,
                                 const cc::PaintFlags& base_flags,
                                 const gfx::RectF& dest_rect,
                                 const ImageTilingInfo& tiling_info,
                                 const ImageDrawOptions& options) {
  gfx::RectF tile_rect = tiling_info.image_rect;
  tile_rect.set_size(tile_rect.size() + tiling_info.spacing);

  SkMatrix pattern_matrix =
      SkMatrix::Translate(tiling_info.phase.x(), tiling_info.phase.y());
  pattern_matrix.preScale(tiling_info.scale.x(), tiling_info.scale.y());
  pattern_matrix.preTranslate(tile_rect.x(), tile_rect.y());

  ImageDrawOptions draw_options(options);
  // TODO(fs): Computing sampling options using `size_` and the tile source
  // rect doesn't seem all too useful since they should be in the same space.
  // Should probably be using the tile source mapped to destination space
  // (instead of `size_`).
  draw_options.sampling_options = dest_context.ComputeSamplingOptions(
      *this, gfx::RectF(size_), tiling_info.image_rect);

  if (DrawPatternAsTiles(dest_context, base_flags, dest_rect, tiling_info,
                         draw_options)) {
    return;
  }

  sk_sp<PaintShader> tile_shader = CreateShader(
      tile_rect, &pattern_matrix, tiling_info.image_rect, draw_options);

  cc::PaintFlags fill_flags(base_flags);
  fill_flags.setShader(std::move(tile_shader));
  fill_flags.setColor(SK_ColorBLACK);

  dest_context.DrawRect(gfx::RectFToSkRect(dest_rect), fill_flags,
                        AutoDarkMode(draw_options));
}

sk_sp<PaintShader> GeneratedImage::CreateShader(
    const gfx::RectF& tile_rect,
    const SkMatrix* pattern_matrix,
    const gfx::RectF& src_rect,
    const ImageDrawOptions& draw_options) {
  PaintRecorder recorder;
  DrawTile(recorder.beginRecording(), src_rect, draw_options);
  return PaintShader::MakePaintRecord(
      recorder.finishRecordingAsPicture(), gfx::RectFToSkRect(tile_rect),
      SkTileMode::kRepeat, SkTileMode::kRepeat, pattern_matrix);
}

PaintImage GeneratedImage::PaintImageForCurrentFrame() {
  return PaintImage();
}

}  // namespace blink
