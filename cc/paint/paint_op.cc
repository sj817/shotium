// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include "cc/paint/paint_op.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/feature_list.h"
#include "base/functional/function_ref.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/values_equivalent.h"
#include "base/notreached.h"
#include "base/numerics/safe_conversions.h"
#include "base/types/optional_util.h"
#include "cc/paint/decoded_draw_image.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/image_provider.h"
#include "cc/paint/paint_filter.h"
#include "cc/paint/paint_flags.h"
#include "cc/paint/paint_image_builder.h"
#include "cc/paint/paint_record.h"
#include "cc/paint/tone_map_util.h"
#include "skia/ext/draw_gainmap_image.h"
#include "third_party/skia/include/core/SkAnnotation.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkPathBuilder.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "third_party/skia/include/core/SkSerialProcs.h"
#include "third_party/skia/include/core/SkTextBlob.h"
#include "third_party/skia/include/core/SkTiledImageUtils.h"
#include "third_party/skia/include/core/SkVertices.h"
#include "third_party/skia/include/effects/SkImageFilters.h"
#include "third_party/skia/src/core/SkCanvasPriv.h"
#include "ui/gfx/geometry/skia_conversions.h"

namespace cc {
namespace {

BASE_FEATURE(kUseLitePaintOps, base::FEATURE_ENABLED_BY_DEFAULT);

// In a future CL, convert DrawImage to explicitly take sampling instead of
// quality
PaintFlags::FilterQuality sampling_to_quality(
    const SkSamplingOptions& sampling) {
  if (sampling.useCubic) {
    return PaintFlags::FilterQuality::kHigh;
  }
  if (sampling.mipmap != SkMipmapMode::kNone) {
    return PaintFlags::FilterQuality::kMedium;
  }
  return sampling.filter == SkFilterMode::kLinear
             ? PaintFlags::FilterQuality::kLow
             : PaintFlags::FilterQuality::kNone;
}

DrawImage CreateDrawImage(const PaintImage& image,
                          const PaintFlags* flags,
                          const PaintFlags::FilterQuality& quality,
                          const SkM44& matrix) {
  if (!image)
    return DrawImage();
  return DrawImage(image, flags->useDarkModeForImage(),
                   SkIRect::MakeWH(image.width(), image.height()), quality,
                   matrix);
}

bool IsScaleAdjustmentIdentity(const SkSize& scale_adjustment) {
  return std::abs(scale_adjustment.width() - 1.f) < FLT_EPSILON &&
         std::abs(scale_adjustment.height() - 1.f) < FLT_EPSILON;
}

SkRect AdjustSrcRectForScale(SkRect original, SkSize scale_adjustment) {
  if (IsScaleAdjustmentIdentity(scale_adjustment))
    return original;

  float x_scale = scale_adjustment.width();
  float y_scale = scale_adjustment.height();
  return SkRect::MakeXYWH(original.x() * x_scale, original.y() * y_scale,
                          original.width() * x_scale,
                          original.height() * y_scale);
}

SkRect MapRect(const SkMatrix& matrix, const SkRect& src) {
  SkRect dst;
  matrix.mapRect(&dst, src);
  return dst;
}

void DrawImageRect(SkCanvas* canvas,
                   const SkImage* image,
                   const SkRect& src,
                   const SkRect& dst,
                   const SkSamplingOptions& options,
                   const SkPaint* paint,
                   SkCanvas::SrcRectConstraint constraint) {
  if (!image)
    return;
  if (constraint == SkCanvas::kStrict_SrcRectConstraint &&
      options.mipmap != SkMipmapMode::kNone &&
      src.contains(SkRect::Make(image->dimensions()))) {
    SkMatrix m;
    m.setRectToRect(src, dst, SkMatrix::ScaleToFit::kFill_ScaleToFit);
    canvas->save();
    canvas->concat(m);
    SkTiledImageUtils::DrawImage(canvas, image, 0, 0, options, paint);
    canvas->restore();
    return;
  }
  SkTiledImageUtils::DrawImageRect(canvas, image, src, dst, options, paint,
                                   constraint);
}

PaintFlags::ScalingOperation MatrixToScalingOperation(SkMatrix m) {
  SkSize scale;
  if (m.decomposeScale(&scale)) {
    return (scale.width() > 1 && scale.height() > 1)
               ? PaintFlags::ScalingOperation::kUpscale
               : PaintFlags::ScalingOperation::kUnknown;
  }
  return PaintFlags::ScalingOperation::kUnknown;
}

#define TYPES(M)             \
  M(AnnotateOp)              \
  M(ClipPathOp)              \
  M(ClipRectOp)              \
  M(ClipRRectOp)             \
  M(ConcatOp)                \
  M(CustomDataOp)            \
  M(DrawArcOp)               \
  M(DrawArcLiteOp)           \
  M(DrawColorOp)             \
  M(DrawDRRectOp)            \
  M(DrawImageOp)             \
  M(DrawImageRectOp)         \
  M(DrawIRectOp)             \
  M(DrawLineOp)              \
  M(DrawLineLiteOp)          \
  M(DrawOvalOp)              \
  M(DrawPathOp)              \
  M(DrawRecordOp)            \
  M(DrawRectOp)              \
  M(DrawRRectOp)             \
  M(DrawScrollingContentsOp) \
  M(DrawTextBlobOp)          \
  M(DrawVerticesOp)          \
  M(NoopOp)                  \
  M(RestoreOp)               \
  M(RotateOp)                \
  M(SaveOp)                  \
  M(SaveLayerOp)             \
  M(SaveLayerAlphaOp)        \
  M(SaveLayerFiltersOp)      \
  M(ScaleOp)                 \
  M(SetMatrixOp)             \
  M(TranslateOp)

static constexpr size_t kNumOpTypes = PaintOp::kNumOpTypes;

// Verify that every op is in the TYPES macro.
#define M(T) +1
static_assert(kNumOpTypes == TYPES(M), "Missing op in list");
#undef M

template <typename T, bool HasFlags>
struct Rasterizer {
  static void RasterWithFlags(const T* op,
                              const PaintFlags* flags,
                              SkCanvas* canvas,
                              const PlaybackParams& params) {
    static_assert(
        !T::kHasPaintFlags,
        "This function should not be used for a PaintOp that has PaintFlags");
    DCHECK(op->IsValid());
    NOTREACHED();
  }
  static void Raster(const T* op,
                     SkCanvas* canvas,
                     const PlaybackParams& params) {
    static_assert(
        !T::kHasPaintFlags,
        "This function should not be used for a PaintOp that has PaintFlags");
    DCHECK(op->IsValid());
    T::Raster(op, canvas, params);
  }
};

template <typename T>
struct Rasterizer<T, true> {
  static void RasterWithFlags(const T* op,
                              const PaintFlags* flags,
                              SkCanvas* canvas,
                              const PlaybackParams& params) {
    static_assert(T::kHasPaintFlags,
                  "This function expects the PaintOp to have PaintFlags");
    DCHECK(op->IsValid());
    T::RasterWithFlags(op, flags, canvas, params);
  }

  static void Raster(const T* op,
                     SkCanvas* canvas,
                     const PlaybackParams& params) {
    static_assert(T::kHasPaintFlags,
                  "This function expects the PaintOp to have PaintFlags");
    DCHECK(op->IsValid());
    T::RasterWithFlags(op, &op->flags, canvas, params);
  }
};

using RasterFunction = void (*)(const PaintOp* op,
                                SkCanvas* canvas,
                                const PlaybackParams& params);
#define M(T)                                                              \
  [](const PaintOp* op, SkCanvas* canvas, const PlaybackParams& params) { \
    Rasterizer<T, T::kHasPaintFlags>::Raster(static_cast<const T*>(op),   \
                                             canvas, params);             \
  },
constexpr std::array<RasterFunction, kNumOpTypes> g_raster_functions = {
    TYPES(M)};
#undef M

using RasterWithFlagsFunction = void (*)(const PaintOp* op,
                                         const PaintFlags* flags,
                                         SkCanvas* canvas,
                                         const PlaybackParams& params);
#define M(T)                                                       \
  [](const PaintOp* op, const PaintFlags* flags, SkCanvas* canvas, \
     const PlaybackParams& params) {                               \
    Rasterizer<T, T::kHasPaintFlags>::RasterWithFlags(             \
        static_cast<const T*>(op), flags, canvas, params);         \
  },
constexpr std::array<RasterWithFlagsFunction, kNumOpTypes>
    g_raster_with_flags_functions = {TYPES(M)};
#undef M

using AreEqualForTestingFunction = bool (*)(const PaintOp&, const PaintOp&);
template <typename T>
bool AreEqualForTesting(const PaintOp& a, const PaintOp& b) {
  return static_cast<const T&>(a).EqualsForTesting(  // IN-TEST
      static_cast<const T&>(b));
}
#define M(T) &AreEqualForTesting<T>,
constexpr std::array<AreEqualForTestingFunction, kNumOpTypes>
    g_equal_for_testing_functions = {TYPES(M)};
#undef M

// Most state ops (matrix, clip, save, restore) have a trivial destructor.
// TODO(enne): evaluate if we need the nullptr optimization or if
// we even need to differentiate trivial destructors here.
using VoidFunction = void (*)(PaintOp* op);
#define M(T)                                           \
  !std::is_trivially_destructible<T>::value            \
      ? [](PaintOp* op) { static_cast<T*>(op)->~T(); } \
      : static_cast<VoidFunction>(nullptr),
constexpr std::array<VoidFunction, kNumOpTypes> g_destructor_functions = {
    TYPES(M)};
#undef M

#define M(T)                                         \
  static_assert(sizeof(T) <= sizeof(LargestPaintOp), \
                #T " must be no bigger than LargestPaintOp");
TYPES(M)
#undef M

#define M(T)                                                \
  static_assert(alignof(T) <= PaintOpBuffer::kPaintOpAlign, \
                #T " must have alignment no bigger than PaintOpAlign");
TYPES(M)
#undef M

using AnalyzeOpFunc = void (*)(PaintOpBuffer*, const PaintOp*);
#define M(T)                                           \
  [](PaintOpBuffer* buffer, const PaintOp* op) {       \
    buffer->AnalyzeAddedOp(static_cast<const T*>(op)); \
  },
constexpr std::array<AnalyzeOpFunc, kNumOpTypes> g_analyze_op_functions = {
    TYPES(M)};
#undef M

}  // namespace

#define M(T) PaintOpBuffer::ComputeOpAlignedSize<T>(),
const std::array<uint16_t, kNumOpTypes> PaintOp::g_type_to_aligned_size = {
    TYPES(M)};
#undef M

#define M(T) T::kIsDrawOp,
const std::array<bool, kNumOpTypes> PaintOp::g_is_draw_op = {TYPES(M)};
#undef M

#define M(T) T::kHasPaintFlags,
const std::array<bool, kNumOpTypes> PaintOp::g_has_paint_flags = {TYPES(M)};
#undef M

const SkRect PaintOp::kUnsetRect = {SK_ScalarInfinity, 0, 0, 0};

std::string PaintOpTypeToString(PaintOpType type) {
  switch (type) {
#define M(T)     \
  case T::kType: \
    return #T;

  TYPES(M)
#undef M
  }
  NOTREACHED();
}

bool IsDiscardableImage(const PaintImage& image,
                        gfx::ContentColorUsage* content_color_usage) {
  if (!image || image.IsTextureBacked()) {
    return false;
  }
  if (content_color_usage) {
    *content_color_usage =
        std::max(*content_color_usage, image.GetContentColorUsage());
  }
  return true;
}

bool OpHasDiscardableImagesImpl(const PaintOp& op) {
  gfx::ContentColorUsage* const unused_content_color_usage = nullptr;
  if (op.IsPaintOpWithFlags() &&
      static_cast<const PaintOpWithFlags&>(op).HasDiscardableImagesFromFlags(
          unused_content_color_usage)) {
    return true;
  }
  switch (op.GetType()) {
#define M(T)                                               \
  case T::kType:                                           \
    return static_cast<const T&>(op).HasDiscardableImages( \
        unused_content_color_usage);

    TYPES(M)
#undef M
  }
}

bool OpHasDrawTextOpsImpl(const PaintOp& op) {
  switch (op.GetType()) {
#define M(T)     \
  case T::kType: \
    return static_cast<const T&>(op).HasDrawTextOps();

    TYPES(M)
#undef M
  }
}

size_t OpAdditionalOpCountImpl(const PaintOp& op) {
  switch (op.GetType()) {
#define M(T)     \
  case T::kType: \
    return static_cast<const T&>(op).AdditionalOpCount();

    TYPES(M)
#undef M
  }
}

#undef TYPES

std::ostream& operator<<(std::ostream& os, PaintOpType type) {
  return os << PaintOpTypeToString(type);
}

void AnnotateOp::Raster(const AnnotateOp* op,
                        SkCanvas* canvas,
                        const PlaybackParams& params) {
  switch (op->annotation_type) {
    case PaintCanvas::AnnotationType::kUrl:
      SkAnnotateRectWithURL(canvas, op->rect, op->data.get());
      break;
    case PaintCanvas::AnnotationType::kLinkToDestination:
      SkAnnotateLinkToDestination(canvas, op->rect, op->data.get());
      break;
    case PaintCanvas::AnnotationType::kNameDestination: {
      SkPoint point = SkPoint::Make(op->rect.x(), op->rect.y());
      SkAnnotateNamedDestination(canvas, point, op->data.get());
      break;
    }
  }
}

void ClipPathOp::Raster(const ClipPathOp* op,
                        SkCanvas* canvas,
                        const PlaybackParams& params) {
  canvas->clipPath(op->path, op->op, op->antialias);
}

void ClipRectOp::Raster(const ClipRectOp* op,
                        SkCanvas* canvas,
                        const PlaybackParams& params) {
  canvas->clipRect(op->rect, op->op, op->antialias);
}

void ClipRRectOp::Raster(const ClipRRectOp* op,
                         SkCanvas* canvas,
                         const PlaybackParams& params) {
  canvas->clipRRect(op->rrect, op->op, op->antialias);
}

void ConcatOp::Raster(const ConcatOp* op,
                      SkCanvas* canvas,
                      const PlaybackParams& params) {
  canvas->concat(op->matrix);
}

void CustomDataOp::Raster(const CustomDataOp* op,
                          SkCanvas* canvas,
                          const PlaybackParams& params) {
  if (params.callbacks.custom_callback) {
    params.callbacks.custom_callback.Run(canvas, op->id);
  }
}

void DrawColorOp::Raster(const DrawColorOp* op,
                         SkCanvas* canvas,
                         const PlaybackParams& params) {
  canvas->drawColor(op->color, op->mode);
}

void DrawDRRectOp::RasterWithFlags(const DrawDRRectOp* op,
                                   const PaintFlags* flags,
                                   SkCanvas* canvas,
                                   const PlaybackParams& params) {
  flags->DrawToSk(canvas, [op](SkCanvas* c, const SkPaint& p) {
    c->drawDRRect(op->outer, op->inner, p);
  });
}

static float ComputeEffectiveHdrHeadroom(const PaintFlags* flags,
                                         const PlaybackParams& params) {
  if (!flags) {
    return params.destination_hdr_headroom;
  }
  // The effective HDR headroom should not be computed when doing no tone
  // mapping.
  DCHECK_NE(flags->getTargetedHdrHeadroom(),
            PaintFlags::TargetedHdrHeadroom::kDisableEverything);
  const float targeted_hdr_headroom =
      flags->getTargetedHdrHeadroom() ==
              PaintFlags::TargetedHdrHeadroom::kFromPlaybackParams
          ? params.destination_hdr_headroom
          : flags->getTargetedHdrHeadroom();
  return flags->getDynamicRangeLimit().ComputeEffectiveHdrHeadroom(
      targeted_hdr_headroom);
}

void DrawImageOp::RasterWithFlags(const DrawImageOp* op,
                                  const PaintFlags* flags,
                                  SkCanvas* canvas,
                                  const PlaybackParams& params) {
  SkPaint paint = flags ? flags->ToSkPaint() : SkPaint();

  // Retrieve the SkImages and sampling.
  sk_sp<SkImage> sk_image;
  sk_sp<SkImage> gainmap_sk_image;
  SkSamplingOptions sampling = op->sampling;
  // If the SkImages are from an ImageProvider, keep them in scope.
  ImageProvider::ScopedResult scoped_result;
  // If scaling is performed, then this will be set to restore after the draw.
  std::optional<SkAutoCanvasRestore> save_restore;
  if (params.image_provider) {
    DrawImage draw_image(op->image, false,
                         SkIRect::MakeWH(op->image.width(), op->image.height()),
                         op->GetImageQuality(), canvas->getLocalToDevice());
    scoped_result = params.image_provider->GetRasterContent(draw_image);
    if (!scoped_result) {
      return;
    }
    const auto& decoded_image = scoped_result.decoded_image();
    DCHECK(decoded_image.image());
    DCHECK_EQ(0, static_cast<int>(decoded_image.src_rect_offset().width()));
    DCHECK_EQ(0, static_cast<int>(decoded_image.src_rect_offset().height()));

    sk_image = decoded_image.image();
    gainmap_sk_image = decoded_image.gainmap_image();
    SkSize scale_adjustment = SkSize::Make(
        op->scale_adjustment.width() * decoded_image.scale_adjustment().width(),
        op->scale_adjustment.height() *
            decoded_image.scale_adjustment().height());
    if (!IsScaleAdjustmentIdentity(scale_adjustment)) {
      save_restore.emplace(canvas, /*doSave=*/true);
      canvas->scale(1.f / scale_adjustment.width(),
                    1.f / scale_adjustment.height());
    }
    sampling = PaintFlags::FilterQualityToSkSamplingOptions(
        decoded_image.filter_quality(),
        MatrixToScalingOperation(canvas->getLocalToDeviceAs3x3()));
  } else {
    if (op->image.IsTextureBacked()) {
      sk_image = op->image.GetAcceleratedSkImage();
    }
    if (!sk_image) {
      sk_image = op->image.GetSwSkImage();
    }
    gainmap_sk_image = op->image.gainmap_sk_image_;
    if (!IsScaleAdjustmentIdentity(op->scale_adjustment)) {
      save_restore.emplace(canvas, /*doSave=*/true);
      canvas->scale(1.f / op->scale_adjustment.width(),
                    1.f / op->scale_adjustment.height());
    }
  }
  if (!sk_image) {
    return;
  }

  const bool disable_tone_mapping =
      flags && flags->getTargetedHdrHeadroom() ==
                   PaintFlags::TargetedHdrHeadroom::kDisableEverything;
  if (!disable_tone_mapping) {
    // If this uses a gainmap shader, then replace DrawImage with a shader.
    if (op->image.HasGainmapInfo() && gainmap_sk_image) {
      skia::DrawGainmapImage(
          canvas, sk_image, gainmap_sk_image, op->image.gainmap_info_.value(),
          std::exp2(ComputeEffectiveHdrHeadroom(flags, params)), op->left,
          op->top, sampling, paint);
      return;
    }

    // Add a tone mapping filter to `paint` if needed.
    if (ToneMapUtil::UseGlobalToneMapFilter(sk_image.get(),
                                            op->image.hdr_metadata_,
                                            canvas->imageInfo().colorSpace())) {
      ToneMapUtil::AddGlobalToneMapFilterToPaint(
          paint, sk_image.get(), op->image.hdr_metadata_,
          ComputeEffectiveHdrHeadroom(flags, params));
    }
  }
  SkTiledImageUtils::DrawImage(canvas, sk_image.get(), op->left, op->top,
                               sampling, &paint);
}

void DrawImageRectOp::RasterWithFlags(const DrawImageRectOp* op,
                                      const PaintFlags* flags,
                                      SkCanvas* canvas,
                                      const PlaybackParams& params) {
  // Retrieve the SkImages, adjusted source rect, and sampling.
  sk_sp<SkImage> sk_image;
  sk_sp<SkImage> gainmap_sk_image;
  SkRect adjusted_src;
  SkSamplingOptions sampling;
  // If the SkImages are from an ImageProvider, keep them in scope.
  ImageProvider::ScopedResult scoped_result;
  if (params.image_provider) {
    SkM44 matrix = canvas->getLocalToDevice() *
                   SkM44(SkMatrix::RectToRect(op->src, op->dst));

    SkIRect int_src_rect;
    op->src.roundOut(&int_src_rect);

    // Dark mode is applied only for GPU raster during serialization.
    DrawImage draw_image(op->image, false, int_src_rect, op->GetImageQuality(),
                         matrix);
    scoped_result = params.image_provider->GetRasterContent(draw_image);
    if (!scoped_result) {
      return;
    }

    const auto& decoded_image = scoped_result.decoded_image();
    DCHECK(decoded_image.image());

    SkSize scale_adjustment = SkSize::Make(
        op->scale_adjustment.width() * decoded_image.scale_adjustment().width(),
        op->scale_adjustment.height() *
            decoded_image.scale_adjustment().height());
    adjusted_src = op->src.makeOffset(decoded_image.src_rect_offset().width(),
                                      decoded_image.src_rect_offset().height());
    adjusted_src = AdjustSrcRectForScale(adjusted_src, scale_adjustment);
    PaintFlags::ScalingOperation scale =
        MatrixToScalingOperation(matrix.asM33());
    sampling = PaintFlags::FilterQualityToSkSamplingOptions(
        decoded_image.filter_quality(), scale);
    sk_image = decoded_image.image();
    gainmap_sk_image = decoded_image.gainmap_image();
  } else {
    adjusted_src = AdjustSrcRectForScale(op->src, op->scale_adjustment);
    SkM44 matrix = canvas->getLocalToDevice() *
                   SkM44(SkMatrix::RectToRect(adjusted_src, op->dst));
    PaintFlags::ScalingOperation scale =
        MatrixToScalingOperation(matrix.asM33());
    PaintFlags::FilterQuality quality = sampling_to_quality(op->sampling);
    sampling = PaintFlags::FilterQualityToSkSamplingOptions(quality, scale);

    if (op->image.IsTextureBacked()) {
      sk_image = op->image.GetAcceleratedSkImage();
    }
    if (!sk_image) {
      sk_image = op->image.GetSwSkImage();
    }
    gainmap_sk_image = op->image.gainmap_sk_image_;
  }
  if (!sk_image) {
    return;
  }

  auto draw_proc = [op, adjusted_src, sampling, sk_image, gainmap_sk_image,
                    flags, params](SkCanvas* c, const SkPaint& p) {
    const bool disable_tone_mapping =
        flags && flags->getTargetedHdrHeadroom() ==
                     PaintFlags::TargetedHdrHeadroom::kDisableEverything;
    if (!disable_tone_mapping) {
      // If the PaintImage uses a gainmap shader, then replace DrawImage with
      // a shader.
      if (op->image.HasGainmapInfo() && gainmap_sk_image) {
        skia::DrawGainmapImageRect(
            c, sk_image, gainmap_sk_image, op->image.gainmap_info_.value(),
            std::exp2(ComputeEffectiveHdrHeadroom(flags, params)), adjusted_src,
            op->dst, sampling, p);
        return;
      }

      // If this uses a global tone map filter, then incorporate that filter
      // into the paint.
      if (ToneMapUtil::UseGlobalToneMapFilter(sk_image.get(),
                                              op->image.hdr_metadata_,
                                              c->imageInfo().colorSpace())) {
        SkPaint tonemap_paint = p;
        ToneMapUtil::AddGlobalToneMapFilterToPaint(
            tonemap_paint, sk_image.get(), op->image.hdr_metadata_,
            ComputeEffectiveHdrHeadroom(flags, params));
        DrawImageRect(c, sk_image.get(), adjusted_src, op->dst, sampling,
                      &tonemap_paint, op->constraint);
        return;
      }
    }

    DrawImageRect(c, sk_image.get(), adjusted_src, op->dst, sampling, &p,
                  op->constraint);
  };
  flags->DrawToSk(canvas, draw_proc);
}

void DrawIRectOp::RasterWithFlags(const DrawIRectOp* op,
                                  const PaintFlags* flags,
                                  SkCanvas* canvas,
                                  const PlaybackParams& params) {
  flags->DrawToSk(canvas, [op](SkCanvas* c, const SkPaint& p) {
    c->drawIRect(op->rect, p);
  });
}

void DrawLineOp::RasterWithFlags(const DrawLineOp* op,
                                 const PaintFlags* flags,
                                 SkCanvas* canvas,
                                 const PlaybackParams& params) {
  flags->DrawToSk(canvas, [op](SkCanvas* c, const SkPaint& p) {
    if (op->draw_as_path) {
      c->drawPath(SkPath::Line({op->x0, op->y0}, {op->x1, op->y1}), p);
    } else {
      c->drawLine(op->x0, op->y0, op->x1, op->y1, p);
    }
  });
}

void DrawLineLiteOp::Raster(const DrawLineLiteOp* op,
                            SkCanvas* canvas,
                            const PlaybackParams& params) {
  PaintFlags flags(op->core_paint_flags);
  flags.DrawToSk(canvas, [op](SkCanvas* c, const SkPaint& p) {
    c->drawLine(op->x0, op->y0, op->x1, op->y1, p);
  });
}

void DrawArcImpl(SkCanvas* canvas,
                 const SkRect& oval,
                 float start_angle_degrees,
                 float sweep_angle_degrees,
                 const SkPaint& paint,
                 const PaintFlags& flags) {
  if (!flags.isArcClosed()) {
    // drawArc can only handle open arcs.
    canvas->drawArc(oval, start_angle_degrees, sweep_angle_degrees, false,
                    paint);
    return;
  }

  if (SkScalarNearlyEqual(std::abs(sweep_angle_degrees), 360)) {
    // Closed ellipses can be rendered using drawOval.
    canvas->drawOval(oval, paint);
  } else {
    // Closed partial arcs -> general SkPath.
    const SkPath path =
        SkPathBuilder()
            .arcTo(oval, start_angle_degrees, sweep_angle_degrees, false)
            .close()
            .detach();
    canvas->drawPath(path, paint);
  }
}

void DrawArcOp::RasterWithFlags(const DrawArcOp* op,
                                const PaintFlags* flags,
                                SkCanvas* canvas,
                                const PlaybackParams& params) {
  op->RasterWithFlagsImpl(flags, canvas);
}

void DrawArcOp::RasterWithFlagsImpl(const PaintFlags* flags,
                                    SkCanvas* canvas) const {
  flags->DrawToSk(canvas, [this, flags](SkCanvas* c, const SkPaint& p) {
    DrawArcImpl(c, oval, start_angle_degrees, sweep_angle_degrees, p, *flags);
  });
}

void DrawArcLiteOp::Raster(const DrawArcLiteOp* op,
                           SkCanvas* canvas,
                           const PlaybackParams& params) {
  PaintFlags flags(op->core_paint_flags);
  flags.DrawToSk(canvas, [op, &flags](SkCanvas* c, const SkPaint& p) {
    DrawArcImpl(c, op->oval, op->start_angle_degrees, op->sweep_angle_degrees,
                p, flags);
  });
}

void DrawOvalOp::RasterWithFlags(const DrawOvalOp* op,
                                 const PaintFlags* flags,
                                 SkCanvas* canvas,
                                 const PlaybackParams& params) {
  flags->DrawToSk(canvas, [op](SkCanvas* c, const SkPaint& p) {
    c->drawOval(op->oval, p);
  });
}

void DrawPathOp::RasterWithFlags(const DrawPathOp* op,
                                 const PaintFlags* flags,
                                 SkCanvas* canvas,
                                 const PlaybackParams& params) {
  flags->DrawToSk(canvas, [op](SkCanvas* c, const SkPaint& p) {
    c->drawPath(op->path, p);
  });
}

void DrawRecordOp::Raster(const DrawRecordOp* op,
                          SkCanvas* canvas,
                          const PlaybackParams& params) {
  // Don't use drawPicture here, as it adds an implicit clip.
  op->record.Playback(canvas, params, op->local_ctm);
}

void DrawRectOp::RasterWithFlags(const DrawRectOp* op,
                                 const PaintFlags* flags,
                                 SkCanvas* canvas,
                                 const PlaybackParams& params) {
  flags->DrawToSk(canvas, [op](SkCanvas* c, const SkPaint& p) {
    c->drawRect(op->rect, p);
  });
}

void DrawRRectOp::RasterWithFlags(const DrawRRectOp* op,
                                  const PaintFlags* flags,
                                  SkCanvas* canvas,
                                  const PlaybackParams& params) {
  flags->DrawToSk(canvas, [op](SkCanvas* c, const SkPaint& p) {
    c->drawRRect(op->rrect, p);
  });
}

void DrawScrollingContentsOp::Raster(const DrawScrollingContentsOp* op,
                                     SkCanvas* canvas,
                                     const PlaybackParams& params) {
  canvas->save();
  CHECK(params.raster_inducing_scroll_offsets);
  gfx::PointF scroll_offset =
      params.raster_inducing_scroll_offsets->at(op->scroll_element_id);
  canvas->translate(-scroll_offset.x(), -scroll_offset.y());
  op->display_item_list->Raster(canvas, params);
  canvas->restore();
}

void DrawVerticesOp::RasterWithFlags(const DrawVerticesOp* op,
                                     const PaintFlags* flags,
                                     SkCanvas* canvas,
                                     const PlaybackParams& params) {
  CHECK_EQ(op->vertices->data().size(), op->uvs->data().size());

  const sk_sp<SkVertices> skverts = SkVertices::MakeCopy(
      SkVertices::kTriangles_VertexMode,
      base::checked_cast<int>(op->vertices->data().size()),
      op->vertices->data().data(), op->uvs->data().data(), nullptr,
      base::checked_cast<int>(op->indices->data().size()),
      op->indices->data().data());

  flags->DrawToSk(canvas, [&skverts](SkCanvas* c, const SkPaint& p) {
    c->drawVertices(skverts, SkBlendMode::kSrcOver, p);
  });
}

void DrawTextBlobOp::RasterWithFlags(const DrawTextBlobOp* op,
                                     const PaintFlags* flags,
                                     SkCanvas* canvas,
                                     const PlaybackParams& params) {
  flags->DrawToSk(canvas, [op](SkCanvas* c, const SkPaint& p) {
    DCHECK(op->blob);
    c->drawTextBlob(op->blob.get(), op->x, op->y, p);
  });
}

void RestoreOp::Raster(const RestoreOp* op,
                       SkCanvas* canvas,
                       const PlaybackParams& params) {
  canvas->restore();
}

void RotateOp::Raster(const RotateOp* op,
                      SkCanvas* canvas,
                      const PlaybackParams& params) {
  canvas->rotate(op->degrees);
}

void SaveOp::Raster(const SaveOp* op,
                    SkCanvas* canvas,
                    const PlaybackParams& params) {
  canvas->save();
}

void SaveLayerOp::RasterWithFlags(const SaveLayerOp* op,
                                  const PaintFlags* flags,
                                  SkCanvas* canvas,
                                  const PlaybackParams& params) {
  // See PaintOp::kUnsetRect
  SkPaint paint = flags->ToSkPaint();
  bool unset = op->bounds.left() == SK_ScalarInfinity;
  canvas->saveLayer(unset ? nullptr : &op->bounds, &paint);
}

void SaveLayerAlphaOp::Raster(const SaveLayerAlphaOp* op,
                              SkCanvas* canvas,
                              const PlaybackParams& params) {
  // See PaintOp::kUnsetRect
  bool unset = op->bounds.left() == SK_ScalarInfinity;
  std::optional<SkPaint> paint;
  if (op->alpha != 1.0f) {
    paint.emplace();
    paint->setAlphaf(op->alpha);
  }
  SkCanvas::SaveLayerRec rec(unset ? nullptr : &op->bounds,
                             base::OptionalToPtr(paint));
  if (params.save_layer_alpha_should_preserve_lcd_text.has_value() &&
      *params.save_layer_alpha_should_preserve_lcd_text) {
    rec.fSaveLayerFlags = SkCanvas::kPreserveLCDText_SaveLayerFlag |
                          SkCanvas::kInitWithPrevious_SaveLayerFlag;
  }
  canvas->saveLayer(rec);
}

void SaveLayerFiltersOp::RasterWithFlags(const SaveLayerFiltersOp* op,
                                         const PaintFlags* flags,
                                         SkCanvas* canvas,
                                         const PlaybackParams& params) {
  SkPaint paint = flags->ToSkPaint();

  // Backdrop filter is the only thing using `op->bounds`, but Skia does not use
  // the bounds when a backdrop filter is present. Instead, clip the filter to
  // the bounds.
  PaintFilter* backdrop_filter = op->backdrop_filter.get();
  sk_sp<SkImageFilter> sk_backdrop_filter =
      PaintFilter::GetSkFilter(backdrop_filter);
  if (sk_backdrop_filter && !backdrop_filter->GetCropRect() &&
      op->bounds.left() != SK_ScalarInfinity) {
    // Bound what the filter reads, not only what it writes.
    // A blur built without a crop rect uses skia's legacy tiling, which takes
    // the whole source image as its content -- and the source of a backdrop
    // filter is the page. Resolving the source through a clamped crop of the
    // rows and columns the kernel can actually reach makes the snapshot and
    // the blur proportional to the element instead of to the page. The
    // output is unchanged: no output pixel inside `bounds` reads beyond the
    // kernel's reach.
    const SkIRect output = op->bounds.roundOut();
    const SkIRect input = sk_backdrop_filter->filterBounds(
        output, SkMatrix::I(), SkImageFilter::kReverse_MapDirection, &output);
    if (!input.isEmpty() && input.width() < (1 << 20) &&
        input.height() < (1 << 20)) {
      sk_backdrop_filter = SkImageFilters::Compose(
          std::move(sk_backdrop_filter),
          SkImageFilters::Crop(SkRect::Make(input), SkTileMode::kClamp,
                               nullptr));
    }
    sk_backdrop_filter =
        SkImageFilters::Crop(op->bounds, std::move(sk_backdrop_filter));
  }

  // Hand skia the bounds. With no bounds the layer -- and
  // the backdrop it copies -- is the size of the clip, which in a one-shot
  // capture is the whole page; every backdrop-filter element then costs a
  // page-sized copy. Skia expands the bounds by the filter's input itself.
  const bool has_bounds = op->bounds.left() != SK_ScalarInfinity;
  canvas->saveLayer(SkCanvasPriv::ScaledBackdropLayer(
      has_bounds ? &op->bounds : nullptr, &paint, sk_backdrop_filter.get(),
      /*backdropScale=*/1.0f, /*saveLayerFlags=*/0,
      PaintFilter::ToSkImageFilters(op->filters)));
}

void ScaleOp::Raster(const ScaleOp* op,
                     SkCanvas* canvas,
                     const PlaybackParams& params) {
  canvas->scale(op->sx, op->sy);
}

void SetMatrixOp::Raster(const SetMatrixOp* op,
                         SkCanvas* canvas,
                         const PlaybackParams& params) {
  canvas->setMatrix(params.original_ctm * op->matrix);
}

void TranslateOp::Raster(const TranslateOp* op,
                         SkCanvas* canvas,
                         const PlaybackParams& params) {
  canvas->translate(op->dx, op->dy);
}

bool AnnotateOp::EqualsForTesting(const AnnotateOp& other) const {
  return annotation_type == other.annotation_type && rect == other.rect &&
         !data == !other.data && (!data || data->equals(other.data.get()));
}

bool ClipPathOp::EqualsForTesting(const ClipPathOp& other) const {
  return path == other.path && op == other.op && antialias == other.antialias;
}

bool ClipRectOp::EqualsForTesting(const ClipRectOp& other) const {
  return rect == other.rect && op == other.op && antialias == other.antialias;
}

bool ClipRRectOp::EqualsForTesting(const ClipRRectOp& other) const {
  return rrect == other.rrect && op == other.op && antialias == other.antialias;
}

bool ConcatOp::EqualsForTesting(const ConcatOp& other) const {
  return matrix == other.matrix;
}

bool CustomDataOp::EqualsForTesting(const CustomDataOp& other) const {
  return id == other.id;
}

bool DrawColorOp::EqualsForTesting(const DrawColorOp& other) const {
  return color == other.color;
}

bool DrawDRRectOp::EqualsForTesting(const DrawDRRectOp& other) const {
  return flags.EqualsForTesting(other.flags) &&  // IN-TEST
         outer == other.outer && inner == other.inner;
}

bool DrawImageOp::EqualsForTesting(const DrawImageOp& other) const {
  // For now image, sampling and constraint are not compared.
  // scale_adjustment intentionally omitted because it is added during
  // serialization based on raster scale.
  return flags.EqualsForTesting(other.flags) &&  // IN-TEST
         top == other.top && left == other.left;
}

bool DrawImageRectOp::EqualsForTesting(const DrawImageRectOp& other) const {
  // For now image, sampling and constraint are not compared.
  // scale_adjustment intentionally omitted because it is added during
  // serialization based on raster scale.
  return flags.EqualsForTesting(other.flags) &&  // IN-TEST
         src == other.src && dst == other.dst;
}

bool DrawIRectOp::EqualsForTesting(const DrawIRectOp& other) const {
  return flags.EqualsForTesting(other.flags) && rect == other.rect;  // IN-TEST
}

bool DrawLineOp::EqualsForTesting(const DrawLineOp& other) const {
  return flags.EqualsForTesting(other.flags) &&  // IN-TEST
         x0 == other.x0 && y0 == other.y0 && x1 == other.x1 && y1 == other.y1;
}

bool DrawLineLiteOp::EqualsForTesting(const DrawLineLiteOp& other) const {
  return x0 == other.x0 && y0 == other.y0 && x1 == other.x1 && y1 == other.y1 &&
         core_paint_flags == other.core_paint_flags;
}

bool DrawArcOp::EqualsForTesting(const DrawArcOp& other) const {
  return flags.EqualsForTesting(other.flags) &&  // IN-TEST
         oval == other.oval &&
         start_angle_degrees == other.start_angle_degrees &&
         sweep_angle_degrees == other.sweep_angle_degrees;
}

bool DrawArcLiteOp::EqualsForTesting(const DrawArcLiteOp& other) const {
  return oval == other.oval &&
         start_angle_degrees == other.start_angle_degrees &&
         sweep_angle_degrees == other.sweep_angle_degrees &&
         core_paint_flags == other.core_paint_flags;
}

bool DrawOvalOp::EqualsForTesting(const DrawOvalOp& other) const {
  return flags.EqualsForTesting(other.flags) && oval == other.oval;  // IN-TEST
}

bool DrawPathOp::EqualsForTesting(const DrawPathOp& other) const {
  return flags.EqualsForTesting(other.flags) && path == other.path;  // IN-TEST
}

bool DrawRecordOp::EqualsForTesting(const DrawRecordOp& other) const {
  return record.EqualsForTesting(other.record);  // IN-TEST
}

bool DrawRectOp::EqualsForTesting(const DrawRectOp& other) const {
  return flags.EqualsForTesting(other.flags) && rect == other.rect;  // IN-TEST
}

bool DrawRRectOp::EqualsForTesting(const DrawRRectOp& other) const {
  return flags.EqualsForTesting(other.flags) &&  // IN-TEST
         rrect == other.rrect;
}

bool DrawScrollingContentsOp::EqualsForTesting(
    const DrawScrollingContentsOp& other) const {
  return scroll_element_id == other.scroll_element_id &&
         display_item_list == other.display_item_list;
}

bool DrawVerticesOp::EqualsForTesting(const DrawVerticesOp& other) const {
  return flags.EqualsForTesting(other.flags) &&  // IN-TEST
         *vertices == *other.vertices && *uvs == *other.uvs &&
         *indices == *other.indices;
}

bool DrawTextBlobOp::EqualsForTesting(const DrawTextBlobOp& other) const {
  return flags.EqualsForTesting(other.flags) &&  // IN-TEST
         x == other.x && y == other.y;
}

bool NoopOp::EqualsForTesting(const NoopOp& other) const {
  return true;
}

bool RestoreOp::EqualsForTesting(const RestoreOp& other) const {
  return true;
}

bool RotateOp::EqualsForTesting(const RotateOp& other) const {
  return degrees == other.degrees;
}

bool SaveOp::EqualsForTesting(const SaveOp& other) const {
  return true;
}

bool SaveLayerOp::EqualsForTesting(const SaveLayerOp& other) const {
  return flags.EqualsForTesting(other.flags) &&  // IN-TEST
         bounds == other.bounds;
}

bool SaveLayerAlphaOp::EqualsForTesting(const SaveLayerAlphaOp& other) const {
  return bounds == other.bounds && alpha == other.alpha;
}

bool SaveLayerFiltersOp::EqualsForTesting(
    const SaveLayerFiltersOp& other) const {
  return flags.EqualsForTesting(other.flags) &&  // IN-TEST
         bounds == other.bounds &&
         std::ranges::equal(
             filters, other.filters,
             [](const sk_sp<PaintFilter>& lhs, const sk_sp<PaintFilter>& rhs) {
               return base::ValuesEquivalent(
                   lhs, rhs, [](const PaintFilter& x, const PaintFilter& y) {
                     return x.EqualsForTesting(y);  // IN-TEST
                   });
             }) &&
         ((!backdrop_filter && !other.backdrop_filter) ||
          ((backdrop_filter && other.backdrop_filter) &&
           backdrop_filter->EqualsForTesting(  // IN-TEST
               *other.backdrop_filter)));
  ;
}

bool ScaleOp::EqualsForTesting(const ScaleOp& other) const {
  return sx == other.sx && sy == other.sy;
}

bool SetMatrixOp::EqualsForTesting(const SetMatrixOp& other) const {
  return matrix == other.matrix;
}

bool TranslateOp::EqualsForTesting(const TranslateOp& other) const {
  return dx == other.dx && dy == other.dy;
}

bool PaintOp::EqualsForTesting(const PaintOp& other) const {
  if (GetType() != other.GetType())
    return false;
  return g_equal_for_testing_functions[type](*this, other);
}

// static
bool PaintOp::TypeHasFlags(PaintOpType type) {
  return g_has_paint_flags[static_cast<uint8_t>(type)];
}

void PaintOp::Raster(SkCanvas* canvas, const PlaybackParams& params) const {
  g_raster_functions[type](this, canvas, params);
}

// static
bool PaintOp::GetBounds(const PaintOp& op, SkRect* rect) {
  switch (op.GetType()) {
    case PaintOpType::kAnnotate:
      return false;
    case PaintOpType::kClipPath:
      return false;
    case PaintOpType::kClipRect:
      return false;
    case PaintOpType::kClipRRect:
      return false;
    case PaintOpType::kConcat:
      return false;
    case PaintOpType::kCustomData:
      return false;
    case PaintOpType::kDrawColor:
      return false;
    case PaintOpType::kDrawDRRect: {
      const auto& rect_op = static_cast<const DrawDRRectOp&>(op);
      *rect = rect_op.outer.getBounds();
      rect->sort();
      return true;
    }
    case PaintOpType::kDrawImage: {
      const auto& image_op = static_cast<const DrawImageOp&>(op);
      *rect = SkRect::MakeXYWH(image_op.left, image_op.top,
                               image_op.image.width(), image_op.image.height());
      rect->sort();
      return true;
    }
    case PaintOpType::kDrawImageRect: {
      const auto& image_rect_op = static_cast<const DrawImageRectOp&>(op);
      *rect = image_rect_op.dst;
      rect->sort();
      return true;
    }
    case PaintOpType::kDrawIRect: {
      const auto& rect_op = static_cast<const DrawIRectOp&>(op);
      *rect = SkRect::Make(rect_op.rect);
      rect->sort();
      return true;
    }
    case PaintOpType::kDrawLine: {
      const auto& line_op = static_cast<const DrawLineOp&>(op);
      rect->setLTRB(line_op.x0, line_op.y0, line_op.x1, line_op.y1);
      rect->sort();
      return true;
    }
    case PaintOpType::kDrawLineLite: {
      const auto& line_op = static_cast<const DrawLineLiteOp&>(op);
      rect->setLTRB(line_op.x0, line_op.y0, line_op.x1, line_op.y1);
      rect->sort();
      return true;
    }
    case PaintOpType::kDrawArc: {
      const auto& arc_op = static_cast<const DrawArcOp&>(op);
      *rect = arc_op.oval;
      rect->sort();
      return true;
    }
    case PaintOpType::kDrawArcLite: {
      const auto& arc_op = static_cast<const DrawArcLiteOp&>(op);
      *rect = arc_op.oval;
      rect->sort();
      return true;
    }
    case PaintOpType::kDrawOval: {
      const auto& oval_op = static_cast<const DrawOvalOp&>(op);
      *rect = oval_op.oval;
      rect->sort();
      return true;
    }
    case PaintOpType::kDrawPath: {
      const auto& path_op = static_cast<const DrawPathOp&>(op);
      *rect = path_op.path.getBounds();
      rect->sort();
      return true;
    }
    case PaintOpType::kDrawRecord:
      return false;
    case PaintOpType::kDrawRect: {
      const auto& rect_op = static_cast<const DrawRectOp&>(op);
      *rect = rect_op.rect;
      rect->sort();
      return true;
    }
    case PaintOpType::kDrawRRect: {
      const auto& rect_op = static_cast<const DrawRRectOp&>(op);
      *rect = rect_op.rrect.rect();
      rect->sort();
      return true;
    }
    case PaintOpType::kDrawScrollingContents:
      return false;
    case PaintOpType::kDrawTextBlob: {
      const auto& text_op = static_cast<const DrawTextBlobOp&>(op);
      *rect = text_op.blob->bounds().makeOffset(text_op.x, text_op.y);
      rect->sort();
      return true;
    }
    case PaintOpType::kDrawVertices: {
      const auto& vertices_op = static_cast<const DrawVerticesOp&>(op);
      rect->setBounds(vertices_op.vertices->data());
      return true;
    }
    case PaintOpType::kNoop:
      return false;
    case PaintOpType::kRestore:
      return false;
    case PaintOpType::kRotate:
      return false;
    case PaintOpType::kSave:
      return false;
    case PaintOpType::kSaveLayer:
      return false;
    case PaintOpType::kSaveLayerAlpha:
      return false;
    case PaintOpType::kSaveLayerFilters:
      return false;
    case PaintOpType::kScale:
      return false;
    case PaintOpType::kSetMatrix:
      return false;
    case PaintOpType::kTranslate:
      return false;
  }
  return false;
}

// static
gfx::Rect PaintOp::ComputePaintRect(const PaintOp& op,
                                    const SkRect& clip_rect,
                                    const SkMatrix& ctm) {
  gfx::Rect transformed_rect;
  SkRect op_rect;
  if (!PaintOp::GetBounds(op, &op_rect)) {
    // If we can't provide a conservative bounding rect for the op, assume it
    // covers the complete current clip.
    // TODO(khushalsagar): See if we can do something better for non-draw ops.
    transformed_rect = gfx::ToEnclosingRect(gfx::SkRectToRectF(clip_rect));
  } else {
    const PaintFlags* flags =
        op.IsPaintOpWithFlags()
            ? &(static_cast<const PaintOpWithFlags&>(op).flags)
            : nullptr;
    SkRect paint_rect = MapRect(ctm, op_rect);
    if (flags) {
      SkPaint paint = flags->ToSkPaint();
      paint_rect = paint.canComputeFastBounds() && paint_rect.isFinite()
                       ? paint.computeFastBounds(paint_rect, &paint_rect)
                       : clip_rect;
    }
    // Clamp the image rect by the current clip rect.
    if (!paint_rect.intersect(clip_rect))
      return gfx::Rect();

    transformed_rect = gfx::ToEnclosingRect(gfx::SkRectToRectF(paint_rect));
  }

  // During raster, we use the device clip bounds on the canvas, which outsets
  // the actual clip by 1 due to the possibility of antialiasing. Account for
  // this here by outsetting the image rect by 1. Note that this only affects
  // queries into the rtree, which will now return images that only touch the
  // bounds of the query rect.
  //
  // Note that it's not sufficient for us to inset the device clip bounds at
  // raster time, since we might be sending a larger-than-one-item display
  // item to skia, which means that skia will internally determine whether to
  // raster the picture (using device clip bounds that are outset).
  transformed_rect.Outset(1);
  return transformed_rect;
}

// static
bool PaintOp::QuickRejectDraw(const PaintOp& op, const SkCanvas* canvas) {
  if (!op.IsDrawOp())
    return false;

  SkRect rect;
  if (!PaintOp::GetBounds(op, &rect) || !rect.isFinite()) {
    return false;
  }

  if (op.IsPaintOpWithFlags()) {
    SkPaint paint = static_cast<const PaintOpWithFlags&>(op).flags.ToSkPaint();
    if (!paint.canComputeFastBounds())
      return false;
    // canvas->quickReject tried to be very fast, and sometimes give a false
    // but conservative result. That's why we need the additional check for
    // |local_op_rect| because it could quickReject could return false even if
    // |local_op_rect| is empty.
    const SkRect& clip_rect = SkRect::Make(canvas->getDeviceClipBounds());
    const SkMatrix& ctm = canvas->getTotalMatrix();
    gfx::Rect local_op_rect = PaintOp::ComputePaintRect(op, clip_rect, ctm);
    if (local_op_rect.IsEmpty())
      return true;
    paint.computeFastBounds(rect, &rect);
  }

  return canvas->quickReject(rect);
}

// static
bool PaintOp::OpHasDiscardableImages(const PaintOp& op) {
  return OpHasDiscardableImagesImpl(op);
}

// static
bool PaintOp::OpHasDrawTextOps(const PaintOp& op) {
  return OpHasDrawTextOpsImpl(op);
}

// static
size_t PaintOp::OpAdditionalOpCount(const PaintOp& op) {
  return OpAdditionalOpCountImpl(op);
}

void PaintOp::DestroyThis() {
  auto func = g_destructor_functions[type];
  if (func)
    func(this);
}

bool PaintOpWithFlags::HasDiscardableImagesFromFlags(
    gfx::ContentColorUsage* content_color_usage) const {
  return flags.HasDiscardableImages(content_color_usage);
}

void PaintOpWithFlags::RasterWithFlags(SkCanvas* canvas,
                                       const PaintFlags* raster_flags,
                                       const PlaybackParams& params) const {
  g_raster_with_flags_functions[type](this, raster_flags, canvas, params);
}

int ClipPathOp::CountSlowPaths() const {
  return antialias && !path.isConvex() ? 1 : 0;
}

int DrawLineOp::CountSlowPaths() const {
  if (const PathEffect* effect = flags.getPathEffect().get()) {
    if (flags.getStrokeCap() != PaintFlags::kRound_Cap &&
        effect->dash_interval_count() == 2) {
      // The PaintFlags will count this as 1, so uncount that here as
      // this kind of line is special cased and not slow.
      return -1;
    }
  }
  return 0;
}

int DrawPathOp::CountSlowPaths() const {
  // This logic is copied from SkPathCounter instead of attempting to expose
  // that from Skia.
  if (!flags.isAntiAlias() || path.isConvex())
    return 0;

  PaintFlags::Style paintStyle = flags.getStyle();
  const SkRect& pathBounds = path.getBounds();
  if (paintStyle == PaintFlags::kStroke_Style && flags.getStrokeWidth() == 0) {
    // AA hairline concave path is not slow.
    return 0;
  } else if (paintStyle == PaintFlags::kFill_Style &&
             pathBounds.width() < 64.f && pathBounds.height() < 64.f &&
             !path.isVolatile()) {
    // AADF eligible concave path is not slow.
    return 0;
  } else {
    return 1;
  }
}

int DrawRecordOp::CountSlowPaths() const {
  return record.num_slow_paths_up_to_min_for_MSAA();
}

bool DrawRecordOp::HasNonAAPaint() const {
  return record.has_non_aa_paint();
}

bool DrawRecordOp::HasDrawTextOps() const {
  return record.has_draw_text_ops();
}

bool DrawRecordOp::HasSaveLayerOps() const {
  return record.has_save_layer_ops();
}

bool DrawRecordOp::HasSaveLayerAlphaOps() const {
  return record.has_save_layer_alpha_ops();
}

bool DrawRecordOp::HasEffectsPreventingLCDTextForSaveLayerAlpha() const {
  return record.has_effects_preventing_lcd_text_for_save_layer_alpha();
}

bool DrawRecordOp::HasDiscardableImages(
    gfx::ContentColorUsage* content_color_usage) const {
  bool has_discardable_images = record.has_discardable_images();
  if (has_discardable_images && content_color_usage) {
    *content_color_usage =
        std::max(*content_color_usage, record.content_color_usage());
  }
  return has_discardable_images;
}

int DrawScrollingContentsOp::CountSlowPaths() const {
  return display_item_list->num_slow_paths_up_to_min_for_MSAA();
}

bool DrawScrollingContentsOp::HasNonAAPaint() const {
  return display_item_list->has_non_aa_paint();
}

bool DrawScrollingContentsOp::HasDrawTextOps() const {
  return display_item_list->has_draw_text_ops();
}

bool DrawScrollingContentsOp::HasSaveLayerOps() const {
  return display_item_list->has_save_layer_ops();
}

bool DrawScrollingContentsOp::HasSaveLayerAlphaOps() const {
  return display_item_list->has_save_layer_alpha_ops();
}

bool DrawScrollingContentsOp::HasEffectsPreventingLCDTextForSaveLayerAlpha()
    const {
  return display_item_list
      ->has_effects_preventing_lcd_text_for_save_layer_alpha();
}

bool DrawScrollingContentsOp::HasDiscardableImages(
    gfx::ContentColorUsage* content_color_usage) const {
  bool has_discardable_images = display_item_list->has_discardable_images();
  if (has_discardable_images && content_color_usage) {
    *content_color_usage = std::max(*content_color_usage,
                                    display_item_list->content_color_usage());
  }
  return has_discardable_images;
}

AnnotateOp::AnnotateOp() : PaintOpBaseInternal(kType) {}

AnnotateOp::AnnotateOp(PaintCanvas::AnnotationType annotation_type,
                       const SkRect& rect,
                       sk_sp<SkData> data)
    : PaintOpBaseInternal(kType),
      annotation_type(annotation_type),
      rect(rect),
      data(std::move(data)) {}

AnnotateOp::~AnnotateOp() = default;

DrawImageOp::DrawImageOp() : PaintOpWithFlagsBaseInternal(kType) {}

DrawImageOp::DrawImageOp(const PaintImage& image, SkScalar left, SkScalar top)
    : PaintOpWithFlagsBaseInternal(kType, PaintFlags()),
      image(image),
      left(left),
      top(top) {}

DrawImageOp::DrawImageOp(const PaintImage& image,
                         SkScalar left,
                         SkScalar top,
                         const SkSamplingOptions& sampling,
                         const PaintFlags* flags)
    : PaintOpWithFlagsBaseInternal(kType, flags ? *flags : PaintFlags()),
      image(image),
      left(left),
      top(top),
      sampling(sampling) {}

bool DrawImageOp::HasDiscardableImages(
    gfx::ContentColorUsage* content_color_usage) const {
  return IsDiscardableImage(image, content_color_usage);
}

PaintFlags::FilterQuality DrawImageOp::GetImageQuality() const {
  return sampling_to_quality(sampling);
}

DrawImageOp::~DrawImageOp() = default;

DrawImageRectOp::DrawImageRectOp() : PaintOpWithFlagsBaseInternal(kType) {}

DrawImageRectOp::DrawImageRectOp(const PaintImage& image,
                                 const SkRect& src,
                                 const SkRect& dst,
                                 SkCanvas::SrcRectConstraint constraint)
    : PaintOpWithFlagsBaseInternal(kType, PaintFlags()),
      image(image),
      src(src),
      dst(dst),
      constraint(constraint) {}

DrawImageRectOp::DrawImageRectOp(const PaintImage& image,
                                 const SkRect& src,
                                 const SkRect& dst,
                                 const SkSamplingOptions& sampling,
                                 const PaintFlags* flags,
                                 SkCanvas::SrcRectConstraint constraint)
    : PaintOpWithFlagsBaseInternal(kType, flags ? *flags : PaintFlags()),
      image(image),
      src(src),
      dst(dst),
      sampling(sampling),
      constraint(constraint) {}

bool DrawImageRectOp::HasDiscardableImages(
    gfx::ContentColorUsage* content_color_usage) const {
  return IsDiscardableImage(image, content_color_usage);
}

PaintFlags::FilterQuality DrawImageRectOp::GetImageQuality() const {
  return sampling_to_quality(sampling);
}

DrawImageRectOp::~DrawImageRectOp() = default;

DrawRecordOp::DrawRecordOp(PaintRecord record, bool local_ctm)
    : PaintOpBaseInternal(kType),
      record(std::move(record)),
      local_ctm(local_ctm) {}

DrawRecordOp::~DrawRecordOp() = default;

size_t DrawRecordOp::AdditionalBytesUsed() const {
  return record.bytes_used();
}

size_t DrawRecordOp::AdditionalOpCount() const {
  return record.total_op_count();
}

DrawScrollingContentsOp::DrawScrollingContentsOp(
    ElementId scroll_element_id,
    scoped_refptr<DisplayItemList> display_item_list)
    : PaintOpBaseInternal(kType),
      scroll_element_id(scroll_element_id),
      display_item_list(std::move(display_item_list)) {}

DrawScrollingContentsOp::~DrawScrollingContentsOp() = default;

size_t DrawScrollingContentsOp::AdditionalBytesUsed() const {
  return display_item_list->BytesUsed();
}

size_t DrawScrollingContentsOp::AdditionalOpCount() const {
  return display_item_list->TotalOpCount();
}

DrawVerticesOp::DrawVerticesOp() : PaintOpWithFlagsBaseInternal(kType) {}

DrawVerticesOp::DrawVerticesOp(
    scoped_refptr<RefCountedBuffer<SkPoint>> vertices,
    scoped_refptr<RefCountedBuffer<SkPoint>> uvs,
    scoped_refptr<RefCountedBuffer<uint16_t>> indices,
    const PaintFlags& paint_flags)
    : PaintOpWithFlagsBaseInternal(kType, paint_flags),
      vertices(std::move(vertices)),
      uvs(std::move(uvs)),
      indices(std::move(indices)) {}

DrawVerticesOp::~DrawVerticesOp() = default;

DrawTextBlobOp::DrawTextBlobOp() : PaintOpWithFlagsBaseInternal(kType) {}

DrawTextBlobOp::DrawTextBlobOp(sk_sp<SkTextBlob> blob,
                               SkScalar x,
                               SkScalar y,
                               const PaintFlags& paint_flags)
    : PaintOpWithFlagsBaseInternal(kType, paint_flags),
      blob(std::move(blob)),
      x(x),
      y(y) {}

DrawTextBlobOp::~DrawTextBlobOp() = default;

SaveLayerFiltersOp::SaveLayerFiltersOp(
    base::span<const sk_sp<PaintFilter>> filters,
    const sk_sp<PaintFilter> backdrop_filter,
    const PaintFlags& paint_flags)
    : PaintOpWithFlagsBaseInternal(kType, paint_flags),
      bounds(kUnsetRect),
      filters(filters.begin(), filters.end()),
      backdrop_filter(backdrop_filter) {}

SaveLayerFiltersOp::SaveLayerFiltersOp(
    const SkRect& bounds,
    base::span<const sk_sp<PaintFilter>> filters,
    const sk_sp<PaintFilter> backdrop_filter,
    const PaintFlags& paint_flags)
    : PaintOpWithFlagsBaseInternal(kType, paint_flags),
      bounds(bounds),
      filters(filters.begin(), filters.end()),
      backdrop_filter(backdrop_filter) {}

SaveLayerFiltersOp::SaveLayerFiltersOp()
    : PaintOpWithFlagsBaseInternal(kType), bounds(kUnsetRect) {}

SaveLayerFiltersOp::~SaveLayerFiltersOp() = default;

bool AreLiteOpsEnabled() {
  static const bool enabled = base::FeatureList::IsEnabled(kUseLitePaintOps);
  return enabled;
}

}  // namespace cc
