// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains utility functions for dealing with localized
// content.

#ifndef UI_BASE_L10N_L10N_UTIL_H_
#define UI_BASE_L10N_L10N_UTIL_H_

#include <optional>

#include <string>
#include <string_view>
#include <vector>

#include "base/component_export.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_APPLE)
#include "ui/base/l10n/l10n_util_mac.h"
#endif  // BUILDFLAG(IS_APPLE)

namespace l10n_util {

enum class CheckLocaleMode {
  // Checks that the localization data is present on disk. It is the default,
  // but potentially costly.
  kVerifyLocalizationDataExists,
  // Checks that the locale is in the list of known locales. It may lead to
  // false positives on platforms where localization is downloaded on-demand
  // - i.e., Android and iOS. See the `kPlatformLocales` documentation in
  // l10n_util.cc for more information.
  kUseKnownLocalesList,
};

// Translates a generic locale name to one of the locally defined ones or
// `std::nullopt` if the resolution is unsuccessful.
COMPONENT_EXPORT(UI_BASE)
std::optional<std::string> CheckAndResolveLocale(
    std::string_view locale,
    CheckLocaleMode mode = CheckLocaleMode::kVerifyLocalizationDataExists);

// This method is responsible for determining the locale as defined below. In
// nearly all cases you shouldn't call this, rather use GetApplicationLocale
// defined on browser_process.
//
// Returns the locale used by the Application.  The algorithm follows this list
// of preferences to find a suitable locale:
// - First the value from the command line (--lang);
// - Second the value in the prefs file (passed in as `pref_locale`);
// - Finally, fallback on the system locale.
//
// A value is returned if it has a corresponding resource on-disk. Otherwise,
// "en-US" is used as the last fallback. `set_icu_locale` determines whether
// the resulting locale is set as the default ICU locale before returning it.
COMPONENT_EXPORT(UI_BASE)
std::string GetApplicationLocale(std::string_view pref_locale,
                                 bool set_icu_locale = true);

// Pulls a resource string from the string bundle.
COMPONENT_EXPORT(UI_BASE) std::u16string GetStringUTF16(int message_id);

}  // namespace l10n_util

#endif  // UI_BASE_L10N_L10N_UTIL_H_
