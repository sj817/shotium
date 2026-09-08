// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_L10N_L10N_UTIL_MAC_H_
#define UI_BASE_L10N_L10N_UTIL_MAC_H_

#include <stddef.h>

#include <string>
#include <vector>

#include "base/component_export.h"

namespace l10n_util {

// Support the override of the locale with the value from Cocoa.
COMPONENT_EXPORT(UI_BASE) void OverrideLocaleWithCocoaLocale();
COMPONENT_EXPORT(UI_BASE) const std::string& GetLocaleOverride();

}  // namespace l10n_util

#endif  // UI_BASE_L10N_L10N_UTIL_MAC_H_
