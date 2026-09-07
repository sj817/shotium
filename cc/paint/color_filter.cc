// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/color_filter.h"

#include <algorithm>
#include <utility>

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/memory/values_equivalent.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/core/SkColorTable.h"
#include "third_party/skia/include/effects/SkHighContrastFilter.h"
#include "third_party/skia/include/effects/SkLumaColorFilter.h"

namespace cc {

namespace {

class MatrixColorFilter final : public ColorFilter {
 public:
  explicit MatrixColorFilter(const float matrix[20])
      : ColorFilter(Type::kMatrix, SkColorFilters::Matrix(matrix)) {}

 private:
};

class BlendColorFilter final : public ColorFilter {
 public:
  BlendColorFilter(const SkColor4f& color, SkBlendMode blend_mode)
      : ColorFilter(Type::kBlend,
                    SkColorFilters::Blend(color, nullptr, blend_mode)) {}

};

class SRGBToLinearGammaColorFilter final : public ColorFilter {
 public:
  SRGBToLinearGammaColorFilter()
      : ColorFilter(Type::kSRGBToLinearGamma,
                    SkColorFilters::SRGBToLinearGamma()) {}
};

class LinearToSRGBGammaColorFilter final : public ColorFilter {
 public:
  LinearToSRGBGammaColorFilter()
      : ColorFilter(Type::kLinearToSRGBGamma,
                    SkColorFilters::LinearToSRGBGamma()) {}
};

class LumaColorFilter final : public ColorFilter {
 public:
  LumaColorFilter() : ColorFilter(Type::kLuma, SkLumaColorFilter::Make()) {}
};

class TableColorFilter : public ColorFilter {
 public:
  explicit TableColorFilter(sk_sp<SkColorTable> table)
      : ColorFilter(Type::kTableARGB, SkColorFilters::Table(table)) {}

 private:

 private:
};

class HighContrastColorFilter final : public ColorFilter {
 public:
  explicit HighContrastColorFilter(const SkHighContrastConfig& config)
      : ColorFilter(Type::kHighContrast, SkHighContrastFilter::Make(config)) {}

};

}  // namespace

ColorFilter::~ColorFilter() = default;

ColorFilter::ColorFilter(Type type, sk_sp<SkColorFilter> sk_color_filter)
    : type_(type), sk_color_filter_(std::move(sk_color_filter)) {
  DCHECK_NE(type, Type::kNull);
}

sk_sp<ColorFilter> ColorFilter::MakeMatrix(const float matrix[20]) {
  return sk_make_sp<MatrixColorFilter>(matrix);
}

sk_sp<ColorFilter> ColorFilter::MakeBlend(const SkColor4f& color,
                                          SkBlendMode blend_mode) {
  return sk_make_sp<BlendColorFilter>(color, blend_mode);
}

sk_sp<ColorFilter> ColorFilter::MakeSRGBToLinearGamma() {
  return sk_make_sp<SRGBToLinearGammaColorFilter>();
}

sk_sp<ColorFilter> ColorFilter::MakeLinearToSRGBGamma() {
  return sk_make_sp<LinearToSRGBGammaColorFilter>();
}

sk_sp<ColorFilter> ColorFilter::MakeTableARGB(const uint8_t a_table[256],
                                              const uint8_t r_table[256],
                                              const uint8_t g_table[256],
                                              const uint8_t b_table[256]) {
  return MakeTable(SkColorTable::Make(a_table, r_table, g_table, b_table));
}

sk_sp<ColorFilter> ColorFilter::MakeTable(sk_sp<SkColorTable> table) {
  return sk_make_sp<TableColorFilter>(std::move(table));
}

sk_sp<ColorFilter> ColorFilter::MakeLuma() {
  return sk_make_sp<LumaColorFilter>();
}

sk_sp<ColorFilter> ColorFilter::MakeHighContrast(
    const SkHighContrastConfig& config) {
  return sk_make_sp<HighContrastColorFilter>(config);
}

SkColor4f ColorFilter::FilterColor(const SkColor4f& color) const {
  return sk_color_filter_
             ? sk_color_filter_->filterColor4f(color, nullptr, nullptr)
             : color;
}
bool ColorFilter::EqualsForTesting(const ColorFilter& other) const {
  return type_ == other.type_;
}

}  // namespace cc
