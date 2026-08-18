// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_CLASSIC_SCRIPT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_CLASSIC_SCRIPT_H_

#include "third_party/blink/renderer/bindings/core/v8/sanitize_script_errors.h"
#include "third_party/blink/renderer/bindings/core/v8/script_source_location_type.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/script/script.h"
#include "third_party/blink/renderer/platform/bindings/parkable_string.h"
#include "third_party/blink/renderer/platform/loader/fetch/script_fetch_options.h"

namespace blink {

struct WebScriptSource;

// https://html.spec.whatwg.org/C/#classic-script
//
// A ClassicScript is the source text of a classic script plus the metadata the
// HTML spec attaches to it. It used to also own everything needed to hand that
// source to V8 -- a ScriptStreamer, a ScriptCacheConsumer, a
// CachedMetadataHandler for the code cache, and CreateScriptOrigin() /
// RunScriptOnScriptStateAndReturnValue() to compile and run it. All of that is
// gone with V8; what is left is the source text and its provenance, which is
// what the rest of core/ (CSP reporting, DevTools-facing URLs, the parser's
// blocking rules) actually reads.
class CORE_EXPORT ClassicScript final : public Script {
 public:
  // For `source_url`.
  static KURL StripFragmentIdentifier(const KURL&);

  // For scripts specified in the HTML spec or for tests.
  // Please leave spec comments and spec links that explain given argument
  // values at non-test callers.
  static ClassicScript* Create(
      const String& source_text,
      const KURL& source_url,
      const KURL& base_url,
      const ScriptFetchOptions&,
      ScriptSourceLocationType = ScriptSourceLocationType::kUnknown,
      SanitizeScriptErrors = SanitizeScriptErrors::kSanitize,
      const TextPosition& start_position = TextPosition::MinimumPosition());

  // For scripts not specified in the HTML spec.
  static ClassicScript* CreateUnspecifiedScript(
      const String& source_text,
      ScriptSourceLocationType = ScriptSourceLocationType::kUnknown,
      SanitizeScriptErrors = SanitizeScriptErrors::kSanitize);
  static ClassicScript* CreateUnspecifiedScript(
      const WebScriptSource&,
      SanitizeScriptErrors = SanitizeScriptErrors::kSanitize);

  // Use Create*() helpers above.
  ClassicScript(
      const ParkableString& source_text,
      const KURL& source_url,
      const KURL& base_url,
      const ScriptFetchOptions&,
      ScriptSourceLocationType,
      SanitizeScriptErrors,
      const TextPosition& start_position = TextPosition::MinimumPosition(),
      const String& source_map_url = String());

  void Trace(Visitor*) const override;

  const ParkableString& SourceText() const { return source_text_; }

  ScriptSourceLocationType SourceLocationType() const {
    return source_location_type_;
  }

  SanitizeScriptErrors GetSanitizeScriptErrors() const {
    return sanitize_script_errors_;
  }

  const String& SourceMapUrl() const { return source_map_url_; }

 private:
  mojom::blink::ScriptType GetScriptType() const override {
    return mojom::blink::ScriptType::kClassic;
  }

  const ParkableString source_text_;

  const ScriptSourceLocationType source_location_type_;

  const SanitizeScriptErrors sanitize_script_errors_;

  const String source_map_url_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_CLASSIC_SCRIPT_H_
