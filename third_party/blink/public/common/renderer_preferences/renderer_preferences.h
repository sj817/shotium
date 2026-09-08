// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_RENDERER_PREFERENCES_RENDERER_PREFERENCES_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_RENDERER_PREFERENCES_RENDERER_PREFERENCES_H_

#include <stdint.h>

namespace blink {

// Default selection colors used by the layout theme.
constexpr uint32_t kDefaultActiveSelectionBgColor = 0xFF1967D2;
constexpr uint32_t kDefaultActiveSelectionFgColor = 0xFFFFFFFF;
constexpr uint32_t kDefaultInactiveSelectionBgColor = 0xFFC8C8C8;
constexpr uint32_t kDefaultInactiveSelectionFgColor = 0xFF323232;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_RENDERER_PREFERENCES_RENDERER_PREFERENCES_H_
