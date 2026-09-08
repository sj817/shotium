// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/headless/headless_screen_util.h"

#include "ui/display/display.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/insets_conversions.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/gfx/geometry/size_f.h"

using display::Display;

namespace headless {

void SetDisplayGeometry(Display& display,
                        const gfx::Rect& bounds_in_pixels,
                        const gfx::Insets& work_area_insets_pixels,
                        float device_pixel_ratio) {
  display.SetScale(device_pixel_ratio);
  display.set_size_in_pixels(bounds_in_pixels.size());
  display.set_native_origin(bounds_in_pixels.origin());

  gfx::SizeF size(bounds_in_pixels.size());
  size.InvScale(display.device_scale_factor());
  gfx::Rect bounds(bounds_in_pixels.origin(), gfx::ToCeiledSize(size));
  display.set_bounds(bounds);

  gfx::Rect work_area = bounds;
  if (!work_area_insets_pixels.IsEmpty()) {
    work_area.Inset(gfx::ScaleToCeiledInsets(
        work_area_insets_pixels, 1.0f / display.device_scale_factor()));
  }
  display.set_work_area(work_area);
}

}  // namespace headless
