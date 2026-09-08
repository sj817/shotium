// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/headless/headless_screen_manager.h"

namespace display {

// static
int64_t HeadlessScreenManager::GetNewDisplayId() {
  // Use larger than max int to catch overflow early.
  static int64_t headless_display_id = 2300000000LL;
  return headless_display_id++;
}

}  // namespace display
