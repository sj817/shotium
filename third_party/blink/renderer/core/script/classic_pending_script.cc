// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/script/classic_pending_script.h"

#include "third_party/blink/public/mojom/loader/code_cache.mojom-blink.h"
#include "third_party/blink/public/mojom/script/script_type.mojom-blink-forward.h"
#include "third_party/blink/public/mojom/script/script_type.mojom-blink.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/script/cache_hint_attribute_value.h"
#include "third_party/blink/renderer/platform/bindings/parkable_string.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"

namespace blink {

// <specdef href="https://html.spec.whatwg.org/C/#fetch-a-classic-script">
ClassicPendingScript* ClassicPendingScript::Fetch(
    const KURL& url,
    Document& element_document,
    const ScriptFetchOptions& options,
    CrossOriginAttributeValue cross_origin,
    const TextEncoding& encoding,
    ScriptElementBase* element,
    FetchParameters::DeferOption defer,
    scheduler::TaskAttributionInfo* task_state,
    mojom::blink::ScriptType script_type) {
  // No request is issued for the script body. Fetching it would only be
  // useful in order to compile and run it, and there is no engine to do
  // either; the bytes would be downloaded and dropped. The resulting
  // PendingScript is therefore ready on creation, so a parser-blocking
  // external script releases the parser in the same task in which it blocked
  // it rather than on a network response that never arrives.
  ClassicPendingScript* pending_script =
      MakeGarbageCollected<ClassicPendingScript>(
          element, TextPosition::MinimumPosition(),
          ClassicScript::StripFragmentIdentifier(url), NullUrl(), String(),
          ScriptSourceLocationType::kExternalFile, options,
          /*is_external=*/true, task_state, CacheHintAttributeValue::kDefault,
          script_type);
  pending_script->CheckState();
  return pending_script;
}

ClassicPendingScript* ClassicPendingScript::CreateInline(
    ScriptElementBase* element,
    const TextPosition& starting_position,
    const KURL& source_url,
    const KURL& base_url,
    const String& source_text,
    ScriptSourceLocationType source_location_type,
    const ScriptFetchOptions& options,
    scheduler::TaskAttributionInfo* task_state,
    CacheHintAttributeValue cache_hint,
    mojom::blink::ScriptType script_type) {
  ClassicPendingScript* pending_script =
      MakeGarbageCollected<ClassicPendingScript>(
          element, starting_position, source_url, base_url, source_text,
          source_location_type, options, /*is_external=*/false, task_state,
          cache_hint, script_type);
  pending_script->CheckState();
  return pending_script;
}

ClassicPendingScript::ClassicPendingScript(
    ScriptElementBase* element,
    const TextPosition& starting_position,
    const KURL& source_url,
    const KURL& base_url_for_inline_script,
    const String& source_text_for_inline_script,
    ScriptSourceLocationType source_location_type,
    const ScriptFetchOptions& options,
    bool is_external,
    scheduler::TaskAttributionInfo* task_state,
    CacheHintAttributeValue cache_hint,
    mojom::blink::ScriptType script_type)
    : PendingScript(element, starting_position, task_state),
      options_(options),
      source_url_(source_url),
      base_url_for_inline_script_(base_url_for_inline_script),
      source_text_for_inline_script_(source_text_for_inline_script),
      source_location_type_(source_location_type),
      is_external_(is_external),
      // `cache_hint` (the script's `cachehint` content attribute) used to be
      // stored here and threaded into the V8 code-cache request this class
      // made when fetching an external script. As the class comment above
      // explains, no such fetch is made any more -- there is no V8 code
      // cache to hint at -- so the parameter is accepted for call-site
      // compatibility but no longer kept.
      script_type_(script_type) {
  CHECK(GetElement());

  if (is_external_) {
    DCHECK(base_url_for_inline_script_.IsNull());
    DCHECK(source_text_for_inline_script_.IsNull());
  } else {
    DCHECK(!base_url_for_inline_script_.IsNull());
    DCHECK(!source_text_for_inline_script_.IsNull());
  }
}

ClassicPendingScript::~ClassicPendingScript() = default;

NOINLINE void ClassicPendingScript::CheckState() const {
  DCHECK(GetElement());
}

void ClassicPendingScript::DisposeInternal() {}

void ClassicPendingScript::Trace(Visitor* visitor) const {
  PendingScript::Trace(visitor);
}

ClassicScript* ClassicPendingScript::GetSource() const {
  CheckState();

  TRACE_EVENT0("blink", "ClassicPendingScript::GetSource");

  if (!is_external_) {
    return ClassicScript::Create(
        source_text_for_inline_script_,
        ClassicScript::StripFragmentIdentifier(source_url_),
        base_url_for_inline_script_, options_, source_location_type_,
        SanitizeScriptErrors::kDoNotSanitize, StartingPosition());
  }

  // An external script was never fetched (see ClassicPendingScript::Fetch()),
  // so its source text is empty. It is deliberately not null: a null Script
  // means "an error occurred" to
  // PendingScript::ExecuteScriptBlockInternal(), which would then fire an
  // `error` event at the element. Nothing failed here, so the element gets its
  // `load` event and an empty script body.
  return ClassicScript::Create(g_empty_string, source_url_,
                               /*base_url=*/source_url_, options_,
                               ScriptSourceLocationType::kExternalFile,
                               SanitizeScriptErrors::kDoNotSanitize);
}

KURL ClassicPendingScript::UrlForTracing() const {
  if (!is_external_) {
    return NullUrl();
  }
  return source_url_;
}

}  // namespace blink
