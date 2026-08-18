// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_SCRIPT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_SCRIPT_H_

#include <optional>

#include "third_party/blink/public/mojom/script/script_type.mojom-blink.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/loader/fetch/script_fetch_options.h"
#include "third_party/blink/renderer/platform/wtf/text/text_position.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

// https://html.spec.whatwg.org/C/#concept-script
//
// This used to additionally be the entry point for *running* a script:
// RunScript()/RunScriptAndReturnValue()/RunScriptOnScriptState() compiled the
// source with V8 and returned a ScriptEvaluationResult. There is no engine to
// compile or run anything in, so those methods are gone and this is now purely
// the "script" record itself: the source (in ClassicScript), the fetch
// options, the base/source URLs and the start position. Everything that
// consumed those -- the parser's blocking rules, `load`/`error` events on the
// element, DocumentParserTiming, the ignore-destructive-writes counter --
// still runs against this record exactly as before.
class CORE_EXPORT Script : public GarbageCollected<Script> {
 public:
  virtual void Trace(Visitor* visitor) const {}

  virtual ~Script() {}

  virtual mojom::blink::ScriptType GetScriptType() const = 0;

  const ScriptFetchOptions& FetchOptions() const { return fetch_options_; }
  const KURL& BaseUrl() const { return base_url_; }
  const KURL& SourceUrl() const { return source_url_; }
  const TextPosition& StartPosition() const { return start_position_; }

 protected:
  explicit Script(const ScriptFetchOptions& fetch_options,
                  const KURL& base_url,
                  const KURL& source_url,
                  const TextPosition& start_position)
      : fetch_options_(fetch_options),
        base_url_(base_url),
        source_url_(source_url),
        start_position_(start_position) {}

 private:
  // https://html.spec.whatwg.org/C/#concept-script-script-fetch-options
  const ScriptFetchOptions fetch_options_;

  // https://html.spec.whatwg.org/C/#concept-script-base-url
  const KURL base_url_;

  // The URL of the script. Observable as the 'source-file' in CSP violation
  // reports.
  //
  // The fragment is stripped due to https://crbug.com/306239 (except for worker
  // top-level scripts), at the callers of Create(), or inside
  // CreateUnspecifiedScript() in ClassicScript.
  //
  // Note that this can be different from the script's base URL
  // (`Script::BaseUrl()`, #concept-script-base-url).
  const KURL source_url_;

  const TextPosition start_position_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_SCRIPT_H_
