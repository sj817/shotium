// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TRUSTEDTYPES_TRUSTED_HTML_PARSER_OPTIONS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TRUSTEDTYPES_TRUSTED_HTML_PARSER_OPTIONS_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"

namespace blink {

class CORE_EXPORT TrustedHTMLParserOptions final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit TrustedHTMLParserOptions(bool run_scripts) : run_scripts_(run_scripts) {}

  bool runScripts() const { return run_scripts_; }

 private:
  bool run_scripts_;
};
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TRUSTEDTYPES_TRUSTED_HTML_PARSER_OPTIONS_H_
