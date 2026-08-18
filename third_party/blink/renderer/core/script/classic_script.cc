// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/script/classic_script.h"

#include "third_party/blink/public/web/web_script_source.h"
#include "third_party/blink/renderer/bindings/core/v8/sanitize_script_errors.h"

namespace blink {

namespace {

ParkableString TreatNullSourceAsEmpty(const ParkableString& source) {
  // The following is the historical comment for this method, while this might
  // be already obsolete, because `TreatNullSourceAsEmpty()` has been applied in
  // all constructors since before.
  //
  // ScriptSourceCode allows for the representation of the null/not-there-really
  // ScriptSourceCode value.  Encoded by way of a source_.IsNull() being true,
  // with the nullary constructor to be used to construct such a value.
  //
  // Should the other constructors be passed a null string, that is interpreted
  // as representing the empty script. Consequently, we need to disambiguate
  // between such null string occurrences.  Do that by converting the latter
  // case's null strings into empty ones.
  if (source.IsNull())
    return ParkableString();

  return source;
}

KURL SanitizeBaseUrl(const KURL& raw_base_url,
                     SanitizeScriptErrors sanitize_script_errors) {
  // https://html.spec.whatwg.org/C/#creating-a-classic-script
  // 2. If muted errors is true, then set baseURL to about:blank.
  // [spec text]
  if (sanitize_script_errors == SanitizeScriptErrors::kSanitize) {
    return BlankUrl();
  }

  return raw_base_url;
}

}  // namespace

KURL ClassicScript::StripFragmentIdentifier(const KURL& url) {
  if (url.IsEmpty())
    return KURL();

  if (!url.HasFragmentIdentifier())
    return url;

  KURL copy = url;
  copy.RemoveFragmentIdentifier();
  return copy;
}

ClassicScript* ClassicScript::Create(
    const String& source_text,
    const KURL& source_url,
    const KURL& base_url,
    const ScriptFetchOptions& fetch_options,
    ScriptSourceLocationType source_location_type,
    SanitizeScriptErrors sanitize_script_errors,
    const TextPosition& start_position) {
  return MakeGarbageCollected<ClassicScript>(
      ParkableString(source_text.Impl()), source_url, base_url, fetch_options,
      source_location_type, sanitize_script_errors, start_position);
}

ClassicScript* ClassicScript::CreateUnspecifiedScript(
    const String& source_text,
    ScriptSourceLocationType source_location_type,
    SanitizeScriptErrors sanitize_script_errors) {
  return MakeGarbageCollected<ClassicScript>(
      ParkableString(source_text.Impl()), NullUrl(), NullUrl(),
      ScriptFetchOptions(), source_location_type, sanitize_script_errors);
}

ClassicScript* ClassicScript::CreateUnspecifiedScript(
    const WebScriptSource& source,
    SanitizeScriptErrors sanitize_script_errors) {
  return MakeGarbageCollected<ClassicScript>(
      ParkableString(String(source.code).Impl()),
      StripFragmentIdentifier(source.url), NullUrl() /* base_url */,
      ScriptFetchOptions(), ScriptSourceLocationType::kUnknown,
      sanitize_script_errors);
}

ClassicScript::ClassicScript(const ParkableString& source_text,
                             const KURL& source_url,
                             const KURL& base_url,
                             const ScriptFetchOptions& fetch_options,
                             ScriptSourceLocationType source_location_type,
                             SanitizeScriptErrors sanitize_script_errors,
                             const TextPosition& start_position,
                             const String& source_map_url)
    : Script(fetch_options,
             SanitizeBaseUrl(base_url, sanitize_script_errors),
             source_url,
             start_position),
      source_text_(TreatNullSourceAsEmpty(source_text)),
      source_location_type_(source_location_type),
      sanitize_script_errors_(sanitize_script_errors),
      source_map_url_(source_map_url) {}

void ClassicScript::Trace(Visitor* visitor) const {
  Script::Trace(visitor);
}

}  // namespace blink
