// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/css_global_rule_set.h"

#include "third_party/blink/renderer/core/css/css_default_style_sheets.h"
#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/dom/document.h"

namespace blink {

void CSSGlobalRuleSet::Update(Document& document) {
  if (!is_dirty_) {
    return;
  }

  is_dirty_ = false;
  features_.Clear();

  CSSDefaultStyleSheets& default_style_sheets =
      CSSDefaultStyleSheets::Instance();

  has_fullscreen_ua_style_ = default_style_sheets.FullscreenStyleSheet();

  default_style_sheets.CollectFeaturesTo(document, features_);

  document.GetStyleEngine().CollectFeaturesTo(features_);
}

void CSSGlobalRuleSet::Dispose() {
  features_.Clear();
  has_fullscreen_ua_style_ = false;
  is_dirty_ = true;
}

}  // namespace blink
