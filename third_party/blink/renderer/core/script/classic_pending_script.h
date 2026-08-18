// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_CLASSIC_PENDING_SCRIPT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_CLASSIC_PENDING_SCRIPT_H_

#include "third_party/blink/renderer/bindings/core/v8/script_source_location_type.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/script/cache_hint_attribute_value.h"
#include "third_party/blink/renderer/core/script/classic_script.h"
#include "third_party/blink/renderer/core/script/pending_script.h"
#include "third_party/blink/public/mojom/script/script_type.mojom-blink-forward.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"

namespace blink {

// PendingScript for a classic script
// https://html.spec.whatwg.org/C/#classic-script.
//
// An *inline* classic script is unchanged: its source text is captured at
// "prepare a script" time and is available immediately, so the script is ready
// as soon as it is created.
//
// An *external* classic script used to start a ScriptResource fetch here and
// stay "not ready" -- blocking the parser for a parser-blocking script -- until
// the response arrived, integrity/nosniff checks passed and (when a V8 code
// cache entry existed) the ScriptCacheConsumer finished off-thread
// deserialization. ScriptResource, ScriptStreamer and ScriptCacheConsumer were
// all V8 plumbing and are gone, and there is no engine that could run the
// bytes even if they were fetched. So no request is made: the script is
// created already ready, with empty source text, and the parser continues
// immediately. `load` is still fired on the element and render-blocking is
// still released, exactly as for a script that fetched successfully and
// executed nothing.
class CORE_EXPORT ClassicPendingScript final : public PendingScript {
 public:
  // https://html.spec.whatwg.org/C/#fetch-a-classic-script
  //
  // For a script from an external file. No network request is issued; see the
  // class comment.
  static ClassicPendingScript* Fetch(
      const KURL&,
      Document&,
      const ScriptFetchOptions&,
      CrossOriginAttributeValue,
      const TextEncoding&,
      ScriptElementBase*,
      FetchParameters::DeferOption,
      scheduler::TaskAttributionInfo* task_state,
      mojom::blink::ScriptType script_type =
          mojom::blink::ScriptType::kClassic);

  // For an inline script.
  static ClassicPendingScript* CreateInline(ScriptElementBase*,
                                            const TextPosition&,
                                            const KURL& source_url,
                                            const KURL& base_url,
                                            const String& source_text,
                                            ScriptSourceLocationType,
                                            const ScriptFetchOptions&,
                                            scheduler::TaskAttributionInfo*,
                                            CacheHintAttributeValue,
                                            mojom::blink::ScriptType =
                                                mojom::blink::ScriptType::kClassic);

  ClassicPendingScript(ScriptElementBase*,
                       const TextPosition&,
                       const KURL& source_url,
                       const KURL& base_url_for_inline_script,
                       const String& source_text_for_inline_script,
                       ScriptSourceLocationType,
                       const ScriptFetchOptions&,
                       bool is_external,
                       scheduler::TaskAttributionInfo* task_state,
                       CacheHintAttributeValue cache_hint,
                       mojom::blink::ScriptType script_type);
  ~ClassicPendingScript() override;

  void Trace(Visitor*) const override;

  // A `<script type="module">` element is still reported as a module script
  // here, even though it is now backed by this class: the spec's
  // module-specific bookkeeping in
  // `PendingScript::ExecuteScriptBlockInternal()` (currentScript stays null,
  // the ignore-destructive-writes counters are incremented) and the parser's
  // "external or module" invariants still apply to it.
  mojom::blink::ScriptType GetScriptType() const override {
    return script_type_;
  }

  ClassicScript* GetSource() const override;
  bool IsReady() const override { return true; }
  bool IsExternal() const override { return is_external_; }
  bool WasCanceled() const override { return false; }
  KURL UrlForTracing() const override;
  void DisposeInternal() override;

 private:
  ClassicPendingScript() = delete;

  void CheckState() const override;

  const ScriptFetchOptions options_;

  // For an external script this is the script's URL; for an inline script it
  // is the source URL recorded at "prepare a script" time.
  const KURL source_url_;

  // "base url" snapshot taken at #prepare-a-script timing.
  // https://html.spec.whatwg.org/C/#prepare-a-script
  // which will eventually be used as #concept-script-base-url.
  // https://html.spec.whatwg.org/C/#concept-script-base-url
  // This is a null URL for external scripts and is not used.
  const KURL base_url_for_inline_script_;

  // "element's child text content" snapshot taken at
  // #prepare-a-script (Step 4).
  // This is a null string for external scripts and is not used.
  const String source_text_for_inline_script_;

  const ScriptSourceLocationType source_location_type_;
  const bool is_external_;
  const mojom::blink::ScriptType script_type_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_CLASSIC_PENDING_SCRIPT_H_
