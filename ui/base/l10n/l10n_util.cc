// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/l10n/l10n_util.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>

#include "base/check_op.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/i18n/file_util_icu.h"
#include "base/i18n/language_tag.h"
#include "base/i18n/language_tag_matcher.h"
#include "base/i18n/rtl.h"
#include "base/i18n/tag_converters.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
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

#if defined(USE_GLIB)
#include <glib.h>
#endif

#if BUILDFLAG(IS_WIN)
#include "base/logging.h"
#include "ui/base/l10n/l10n_util_win.h"
#endif  // BUILDFLAG(IS_WIN)

namespace l10n_util {
namespace {

using ::base::i18n::GetKnownLanguageTag;
using ::base::i18n::GetLanguageTagFromString;
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
  // Use any override (Cocoa for the browser), otherwise use the preference
  // passed to the function.
  std::string app_locale = l10n_util::GetLocaleOverride();
  if (app_locale.empty())
    app_locale = pref_locale;

  // The above should handle all of the cases Chrome normally hits, but for some
  // unit tests, fallback is needed too.
  if (app_locale.empty())
    app_locale = "en-US";

  return app_locale;
}
#endif

#if !BUILDFLAG(IS_APPLE)
std::string GetApplicationLocaleInternalNonMac(std::string_view pref_locale) {
  std::vector<std::optional<LanguageTag>> candidates;
  // The `prefered_tag` is separated from the other candidates.
  std::optional<LanguageTag> prefered_tag = std::nullopt;
  // Use --lang and the app pref on Windows.  On Linux, only
  // look at the LC_*/LANG environment variables.  However, passing --lang
  // to renderer and plugin processes is common, so they know what language the
  // parent process decided to use.

#if BUILDFLAG(IS_WIN)
  // First, try the preference value.
  if (!pref_locale.empty()) {
    prefered_tag = GetLanguageTagFromString(pref_locale);
  }

  // Next, try the overridden locale.
  const std::vector<std::string>& languages = l10n_util::GetLocaleOverrides();
  if (!languages.empty()) {
    candidates.reserve(candidates.size() + languages.size());
    std::ranges::transform(languages, std::back_inserter(candidates),
                           [](const std::string& language) {
                             return GetLanguageTagFromString(language);
                           });
  } else {
    // If no override was set, defer to ICU
    candidates.push_back(
        base::i18n::LanguageTagConverter::GetInstance().FromIcuLocale(
            icu::Locale::getDefault()));
  }
#elif BUILDFLAG(IS_ANDROID)
  // Try pref_locale first.
  if (!pref_locale.empty()) {
    prefered_tag = GetLanguageTagFromString(pref_locale);
  }

  // On Android, query java.util.Locale for the default locale.
  candidates.push_back(
      GetLanguageTagFromString(base::android::GetDefaultLocaleString()));
#elif defined(USE_GLIB) && !BUILDFLAG(IS_CHROMEOS)
  // GLib implements correct environment variable parsing with
  // the precedence order: LANGUAGE, LC_ALL, LC_MESSAGES and LANG.
  const char* const* languages = g_get_language_names();
  DCHECK(languages);  // A valid pointer is guaranteed.
  DCHECK(*languages);  // At least one entry, "C", is guaranteed.

  // SAFETY: g_get_language_names returns a valid NULL-terminated array.
  // See: https://docs.gtk.org/glib/func.get_language_names.html
  for (; *languages; UNSAFE_BUFFERS(++languages)) {
    if (std::optional<LanguageTag> language_tag =
            GetLanguageTagFromString(*languages);
        language_tag) {
      candidates.push_back(std::move(language_tag));
    }
  }
#else
  // By default, use the application locale preference. This applies to ChromeOS
  // and linux systems without glib.
  if (!pref_locale.empty()) {
    prefered_tag = GetLanguageTagFromString(pref_locale);
  }
#endif  // BUILDFLAG(IS_WIN)

  // If `prefered_tag`, it is attempt to get a match for it, even if it is not
  // exact.
  if (prefered_tag) {
    if (std::optional<LanguageTag> resolved = CheckAndResolveLocale(
            *prefered_tag, CheckLocaleMode::kVerifyLocalizationDataExists)) {
      return std::string(resolved->tag_string());
    }
  }

  std::optional<LanguageTag> matched_candidate;
  for (const std::optional<LanguageTag>& candidate : candidates) {
    if (!candidate) {
      continue;
    }

    // If a exact match with a resource-bundle locale on-disk is found, it is
    // returned.
    if (IsResourceBundleLocale(*candidate)) {
      return std::string(candidate->tag_string());
    }

    if (matched_candidate) {
      continue;
    }
    // If there was a match using `CheckAndResolveLocale`, it is stored but not
    // returned yet because the priority is to find a candidate that has an
    // exact match with a `ResourceBundle` locale.
    if (std::optional<LanguageTag> resolved = CheckAndResolveLocale(
            *candidate, CheckLocaleMode::kVerifyLocalizationDataExists);
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
    base::i18n::SetICUDefaultLocale(locale);
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
