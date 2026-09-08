// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/l10n/l10n_util_mac.h"

#import <Foundation/Foundation.h>

#import "base/apple/bundle_locations.h"
#import "base/check.h"
#import "base/lazy_instance.h"
#import "base/strings/sys_string_conversions.h"
#import "ui/base/l10n/l10n_util.h"

namespace {

base::LazyInstance<std::string>::DestructorAtExit g_overridden_locale =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

namespace l10n_util {

// Thread-safety:
//
// This function returns a reference to a global variable that is mutated by
// OverrideLocaleWithCocoaLocale(), is called from multiple thread and doesn't
// include locking.
//
// This may appear thread-unsafe. However, OverrideLocaleWithCocoaLocale() is
// only called once during the application startup before the creation of the
// Chromium threads, thus for the threads this can be considered as a constant
// and can be safely be accessed without synchronisation nor memory barrier.
//
// This is only true as long as no new usage of OverrideLocaleWithCocoaLocale()
// is added to the code base.
const std::string& GetLocaleOverride() {
  return g_overridden_locale.Get();
}

void OverrideLocaleWithCocoaLocale() {
  // NSBundle really should only be called on the main thread.
  DCHECK(NSThread.isMainThread);

  // Chrome really only has one concept of locale, but macOS has locale and
  // language that can be set independently.  After talking with Chrome UX folks
  // (Cole), the best path from an experience point of view is to map the macOS
  // language into the Chrome locale.  This way strings like "Yesterday" and
  // "Today" are in the same language as raw dates like "March 20, 1999" (Chrome
  // strings resources vs ICU generated strings).  This also makes the Mac act
  // like other Chrome platforms.
  NSArray* language_list = base::apple::OuterBundle().preferredLocalizations;
  NSString* first_locale = language_list[0];
  // macOS uses "_" instead of "-", so swap to get a real locale value.
  std::string locale_value = base::SysNSStringToUTF8(
      [first_locale stringByReplacingOccurrencesOfString:@"_" withString:@"-"]);

  // On disk the "en-US" resources are just "en" (http://crbug.com/25578), so
  // the reverse mapping is done here to continue to feed Chrome the same values
  // in all cases on all platforms.  (l10n_util maps en to en-US if it gets
  // passed this on the command line)
  if (locale_value == "en")
    locale_value = "en-US";

  // The outer bundle can advertise a localization the framework has no
  // locale.pak for (e.g. de-DE.lproj beside de.lproj); resolve to one that
  // does, falling back to en-US like GetApplicationLocaleInternalNonMac().
  locale_value =
      l10n_util::CheckAndResolveLocale(locale_value).value_or("en-US");

  g_overridden_locale.Get() = locale_value;
}

}  // namespace l10n_util
