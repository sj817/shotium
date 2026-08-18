// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/win/hwnd_metrics.h"

#include "ui/display/screen.h"
#include "ui/display/win/screen_win.h"

namespace ui {

int GetResizableFrameThicknessFromMonitorInPixels(HMONITOR monitor,
                                                  bool has_caption) {
  const int resize_handle_thickness =
      display::win::GetScreenWin()->GetSystemMetricsForMonitor(monitor,
                                                               SM_CXSIZEFRAME);
  // SM_CXPADDEDBORDER is some extra padding not part of the resize handle.
  const int padding_thickness =
      display::win::GetScreenWin()->GetSystemMetricsForMonitor(
          monitor, SM_CXPADDEDBORDER);
  // If a window has WS_CAPTION set the frame thickness includes a 1px border.
  // This border must be removed if WS_CAPTION is not set.
  return resize_handle_thickness + padding_thickness - (has_caption ? 0 : 1);
}

int GetResizableFrameThicknessFromMonitorInDIP(HMONITOR monitor,
                                               bool has_caption) {
  return GetResizableFrameThicknessFromMonitorInPixels(monitor, has_caption) /
         display::win::GetScreenWin()->GetScaleFactorForMonitor(monitor);
}

int GetFrameThicknessFromWindow(HWND hwnd, DWORD default_options) {
  HMONITOR monitor = ::MonitorFromWindow(hwnd, default_options);
  return GetResizableFrameThicknessFromMonitorInPixels(
      monitor, GetWindowLong(hwnd, GWL_STYLE) & WS_CAPTION);
}

int GetFrameThicknessFromScreenRect(const gfx::Rect& screen_rect) {
  // Only ScreenWinHeadless could answer this, by mapping the rect to one of the
  // virtual displays it owned. That path went with //headless; shot brings its
  // own display::Screen and runs headful as far as this code is concerned, and
  // display::win::ScreenWin has never supported the query.
  NOTREACHED();
}

}  // namespace ui
