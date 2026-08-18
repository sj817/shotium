// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/markers/glic_marker.h"

#include "base/time/time.h"
#include "ui/gfx/geometry/cubic_bezier.h"

namespace blink {

constexpr static base::TimeDelta kAnimationDuration = base::Seconds(1);

float GetOpacity(float progress) {
  constexpr static float kOpacityStart = 0.f;
  constexpr static float kOpacityFinish = 1.f;
  double y = gfx::CubicBezier(0.2f, 0.f, 0.f, 1.f).Solve(progress);
  float opacity = kOpacityStart + (kOpacityFinish - kOpacityStart) * y;
  return opacity;
}

GlicMarker::GlicMarker(wtf_size_t start_offset, wtf_size_t end_offset)
    : DocumentMarker(start_offset, end_offset) {}

DocumentMarker::MarkerType GlicMarker::GetType() const {
  return DocumentMarker::kGlic;
}

Color GlicMarker::BackgroundColor() const {
  // Used to pull this color from shared_highlighting::
  // kFragmentTextBackgroundColorARGB (components/shared_highlighting), but
  // that component was deleted wholesale along with the browser-side text-
  // fragment "copy link to highlight" feature it served. The constant itself
  // (0xFFE9D2FD, a pale purple) is just a color value with no dependency on
  // the deleted machinery, so it is inlined here.
  Color color = Color::FromRGBA32(0xFFE9D2FD);
  color.SetAlpha(opacity_);
  return color;
}

bool GlicMarker::UpdateOpacityForDuration(base::TimeDelta duration) {
  double progress = duration / kAnimationDuration;
  bool is_last_frame = progress >= 1;
  opacity_ = is_last_frame ? GetOpacity(1.0) : GetOpacity(progress);
  return is_last_frame;
}
}  // namespace blink
