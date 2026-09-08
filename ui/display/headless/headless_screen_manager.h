// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_HEADLESS_HEADLESS_SCREEN_MANAGER_H_
#define UI_DISPLAY_HEADLESS_HEADLESS_SCREEN_MANAGER_H_

#include <stdint.h>

#include "ui/display/display_export.h"

namespace display {
// Allocates IDs for initial headless displays.
class DISPLAY_EXPORT HeadlessScreenManager {
 public:
  static int64_t GetNewDisplayId();

 private:
  HeadlessScreenManager() = delete;
};

}  // namespace display

#endif  // UI_DISPLAY_HEADLESS_HEADLESS_SCREEN_MANAGER_H_
