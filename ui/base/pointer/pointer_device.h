// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_POINTER_POINTER_DEVICE_H_
#define UI_BASE_POINTER_POINTER_DEVICE_H_

namespace ui {

// Static bit fields used by Blink CSS pointer and hover media queries.
enum PointerType {
  POINTER_TYPE_NONE = 1 << 0,
  POINTER_TYPE_FIRST = POINTER_TYPE_NONE,
  POINTER_TYPE_COARSE = 1 << 1,
  POINTER_TYPE_FINE = 1 << 2,
  POINTER_TYPE_LAST = POINTER_TYPE_FINE
};

enum HoverType {
  HOVER_TYPE_NONE = 1 << 0,
  HOVER_TYPE_FIRST = HOVER_TYPE_NONE,
  HOVER_TYPE_HOVER = 1 << 1,
  HOVER_TYPE_LAST = HOVER_TYPE_HOVER
};

}  // namespace ui

#endif  // UI_BASE_POINTER_POINTER_DEVICE_H_
