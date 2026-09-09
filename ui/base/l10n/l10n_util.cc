// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/l10n/l10n_util.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "base/check_op.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/i18n/file_util_icu.h"
#include "base/i18n/icu4c_tag_converter.h"
#include "base/i18n/icubridge/default_icu_locale.h"
#include "base/i18n/language_tag.h"
#include "base/i18n/language_tag_matcher.h"
#include "base/i18n/rtl.h"
#include "base/i18n/tag_converters.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "build/build_config.h"
#include "ui/base/l10n/chromium_language_matcher.h"
#include "ui/base/resource/resource_bundle.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/android/locale_utils.h"
#include "ui/base/l10n/l10n_util_android.h"
#endif

#if BUILDFLAG(IS_IOS)
#include "ui/base/l10n/l10n_util_ios.h"
#endif

#if BUILDFLAG(IS_WIN)
#include "base/logging.h"
#include "ui/base/l10n/l10n_util_win.h"
#endif  // BUILDFLAG(IS_WIN)

namespace l10n_util {
namespace {

using ::base::i18n::GetKnownLanguageTag;
using ::base::i18n::GetLanguageTagFromString;
using ::base::i18n::IcuLocaleConverter;
using ::base::i18n::LanguageTag;
using ::base::i18n::LanguageTagMatcher;

bool IsResourceBundleLocale(const LanguageTag& locale) {
  return ui::ResourceBundle::LocaleDataPakExists(
      locale, ui::ResourceBundle::Gender::kDefault);
}

// On Linux, the text layout engine Pango determines paragraph directionality
// by looking at the first strongly-directional character in the text. This
// means text such as "Google Chrome foo bar..." will be layed out LTR even
// if "foo bar" is RTL. So this function prepends the necessary RLM in such
// cases.
void AdjustParagraphDirectionality(std::u16string* paragraph) {
#if BUILDFLAG(IS_POSIX) && !BUILDFLAG(IS_APPLE) && !BUILDFLAG(IS_ANDROID)
  if (base::i18n::IsRTL() &&
      base::i18n::StringContainsStrongRTLChars(*paragraph)) {
    paragraph->insert(0, 1, char16_t{base::i18n::kRightToLeftMark});
  }
#endif
}

#if !BUILDFLAG(IS_APPLE)
// Use --lang and the app pref on Windows.  On Linux, only look at the LC_*/LANG
// environment variables.
std::vector<LanguageTag> GetCandidates() {
  std::vector<LanguageTag> candidates;
#if BUILDFLAG(IS_WIN)
  // Try the overridden locale.
  candidates = l10n_util::GetLocaleOverrides();
  if (candidates.empty()) {
    // If no override was set, defer to ICU
    return {base::i18n::IcuLocaleConverter::GetInstance().ToLanguageTag(
        icu::Locale::getDefault())};
  }
#elif BUILDFLAG(IS_ANDROID)
  // On Android, query java.util.Locale for the default locale.
  if (std::optional<LanguageTag> language_tag =
          GetLanguageTagFromString(base::android::GetDefaultLocaleString())) {
    candidates.push_back(*std::move(language_tag));
  }
#elif defined(USE_GLIB) && !BUILDFLAG(IS_CHROMEOS)
  for (const char* var_name : {"LANGUAGE", "LC_ALL", "LC_MESSAGES", "LANG"}) {
    const char* val = std::getenv(var_name);
    if (!val || *val == '\0') {
      continue;
    }
    for (std::string_view candidate : base::SplitStringPiece(
             val, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
      if (std::optional<LanguageTag> language_tag =
              GetLanguageTagFromString(candidate)) {
        candidates.push_back(*std::move(language_tag));
      }
    }
  }
#endif  // BUILDFLAG(IS_WIN)
  return candidates;
}

#endif  // !BUILDFLAG(IS_APPLE)

// Preferred locales are enabled everywhere but Linux systems with GLib. This
// function parses `preferred_locale` into a `LanguageTag` if they are enabled.
std::optional<LanguageTag> GetPreferredTag(std::string_view preferred_locale) {
#if BUILDFLAG(IS_APPLE)
  std::string_view locale_override = l10n_util::GetLocaleOverride();
  return GetLanguageTagFromString(locale_override.empty() ? preferred_locale
                                                          : locale_override);
#elif defined(USE_GLIB) && !BUILDFLAG(IS_CHROMEOS)
  return std::nullopt;
#else
  return GetLanguageTagFromString(preferred_locale);
#endif
}

}  // namespace

std::optional<LanguageTag> CheckAndResolveLocale(const LanguageTag& locale,
                                                 CheckLocaleMode mode) {
  if (mode == CheckLocaleMode::kVerifyLocalizationDataExists &&
      IsResourceBundleLocale(locale)) {
    return locale;
  }

  std::optional<LanguageTag> matched =
      ui_l10n::GetPlatformLanguageMatcher().Match(locale);
  if (!matched || (mode == CheckLocaleMode::kVerifyLocalizationDataExists &&
                   !IsResourceBundleLocale(*matched))) {
    return std::nullopt;
  }
  return matched;
}

std::optional<std::string> CheckAndResolveLocale(std::string_view locale,
                                                 CheckLocaleMode mode) {
  return GetLanguageTagFromString(locale).and_then(
      [mode](const LanguageTag& language_tag) {
        return CheckAndResolveLocale(language_tag, mode)
            .transform([](const LanguageTag& resolved) {
              return std::string(resolved.tag_string());
            });
      });
}

#if BUILDFLAG(IS_APPLE)
std::string GetApplicationLocaleInternalMac(std::string_view pref_locale) {
  std::optional<LanguageTag> preferred_locale_tag =
      GetPreferredTag(pref_locale);

  // The above should handle all of the cases Chrome normally hits, but for some
  // unit tests, fallback is needed too.
  if (!preferred_locale_tag) {
    return "en-US";
  }

  return std::string(preferred_locale_tag->tag_string());
}
#endif

#if !BUILDFLAG(IS_APPLE)
std::string GetApplicationLocaleInternalNonMac(std::string_view pref_locale) {
  // The `preferred_tag` is separated from the other candidates.
  std::optional<LanguageTag> preferred_tag = GetPreferredTag(pref_locale);
  // If `preferred_tag`, it attempts to get a match for it, even if it is not
  // exact.
  if (preferred_tag) {
    if (std::optional<LanguageTag> resolved = CheckAndResolveLocale(
            *preferred_tag, CheckLocaleMode::kVerifyLocalizationDataExists)) {
      return std::string(resolved->tag_string());
    }
  }

  std::vector<LanguageTag> candidates = GetCandidates();
  std::optional<LanguageTag> matched_candidate;
  for (const LanguageTag& candidate : candidates) {
    // If an exact match is found and resource-bundle data on-disk is found, it
    // is returned immediately.
    if (ui_l10n::GetPlatformLanguageMatcher().HasExactMatch(candidate) &&
        IsResourceBundleLocale(candidate)) {
      return std::string(candidate.tag_string());
    }

    if (matched_candidate) {
      continue;
    }
    // If there was a match using `CheckAndResolveLocale`, it is stored but not
    // returned yet because the priority is to find a candidate that has an
    // exact match with a `ResourceBundle` locale.
    if (std::optional<LanguageTag> resolved = CheckAndResolveLocale(
            candidate, CheckLocaleMode::kVerifyLocalizationDataExists);
        resolved) {
      matched_candidate = *resolved;
    }
  }

  if (matched_candidate) {
    return std::string(matched_candidate->tag_string());
  }

  // Fallback to "en-US"
  return IsResourceBundleLocale(GetKnownLanguageTag("en-US")) ? "en-US" : "";
}
#endif  // !BUILDFLAG(IS_APPLE)

std::string GetApplicationLocaleInternal(std::string_view pref_locale) {
#if BUILDFLAG(IS_APPLE)
  return GetApplicationLocaleInternalMac(pref_locale);
#else
  return GetApplicationLocaleInternalNonMac(pref_locale);
#endif
}

std::string GetApplicationLocale(std::string_view pref_locale,
                                 bool set_icu_locale) {
  const std::string locale = GetApplicationLocaleInternal(pref_locale);
  if (set_icu_locale && !locale.empty()) {
    std::optional<LanguageTag> language_tag = GetLanguageTagFromString(locale);
    if (language_tag) {
      base::i18n::SetDefaultIcuLocale(base::i18n::DefaultIcuLocaleSetterKey(),
                                      *language_tag);
    }
  }
  return locale;
}

std::u16string GetStringUTF16(int message_id) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  std::u16string str = rb.GetLocalizedString(message_id);
  AdjustParagraphDirectionality(&str);

  return str;
}

}  // namespace l10n_util
