// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/decoded_draw_image.h"

#include <utility>

#include "cc/paint/color_filter.h"

namespace cc {

DecodedDrawImage::DecodedDrawImage(sk_sp<SkImage> image,
                                   sk_sp<ColorFilter> dark_mode_color_filter,
                                   const SkSize& src_rect_offset,
                                   const SkSize& scale_adjustment,
                                   PaintFlags::FilterQuality filter_quality,
                                   bool is_budgeted)
    : DecodedDrawImage(std::move(image),
                       nullptr,
                       std::move(dark_mode_color_filter),
                       src_rect_offset,
                       scale_adjustment,
                       filter_quality,
                       is_budgeted) {}

DecodedDrawImage::DecodedDrawImage(sk_sp<SkImage> image,
                                   sk_sp<SkImage> gainmap_image,
                                   sk_sp<ColorFilter> dark_mode_color_filter,
                                   const SkSize& src_rect_offset,
                                   const SkSize& scale_adjustment,
                                   PaintFlags::FilterQuality filter_quality,
                                   bool is_budgeted)
    : image_(std::move(image)),
      gainmap_image_(std::move(gainmap_image)),
      dark_mode_color_filter_(std::move(dark_mode_color_filter)),
      src_rect_offset_(src_rect_offset),
      scale_adjustment_(scale_adjustment),
      filter_quality_(filter_quality),
      is_budgeted_(is_budgeted) {}

DecodedDrawImage::DecodedDrawImage()
    : DecodedDrawImage(nullptr,
                       nullptr,
                       SkSize::MakeEmpty(),
                       SkSize::Make(1.f, 1.f),
                       PaintFlags::FilterQuality::kNone,
                       true) {}

DecodedDrawImage::DecodedDrawImage(const DecodedDrawImage&) = default;
DecodedDrawImage::DecodedDrawImage(DecodedDrawImage&&) = default;
DecodedDrawImage& DecodedDrawImage::operator=(const DecodedDrawImage&) =
    default;
DecodedDrawImage& DecodedDrawImage::operator=(DecodedDrawImage&&) = default;

DecodedDrawImage::~DecodedDrawImage() = default;

}  // namespace cc
