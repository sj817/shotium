// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_GLOBAL_RULE_SET_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_GLOBAL_RULE_SET_H_

#include "third_party/blink/renderer/core/css/rule_feature_set.h"

namespace blink {

class Document;

// A per Document collection of CSS metadata used for style matching and
// invalidation. The data is aggregated from author rulesets from all TreeScopes
// in the whole Document as well as UA stylesheets.
//
// TODO(futhark@chromium.org): We would like to move as much of this data as
// possible to the ScopedStyleResolver as possible to avoid full reconstruction
// of these rulesets on shadow tree changes. See https://crbug.com/401359

class CSSGlobalRuleSet final : public GarbageCollected<CSSGlobalRuleSet> {
 public:
  CSSGlobalRuleSet() = default;
  CSSGlobalRuleSet(const CSSGlobalRuleSet&) = delete;
  CSSGlobalRuleSet& operator=(const CSSGlobalRuleSet&) = delete;

  void Dispose();
  void MarkDirty() { is_dirty_ = true; }
  bool IsDirty() const { return is_dirty_; }
  void Update(Document&);

  const RuleFeatureSet& GetRuleFeatureSet() const { return features_; }
  bool HasFullscreenUAStyle() const { return has_fullscreen_ua_style_; }

  void Trace(Visitor*) const {}

 private:
  // Constructed from rules in all TreeScopes including UA style and style
  // injected from extensions.
  RuleFeatureSet features_;

  bool has_fullscreen_ua_style_ = false;
  bool is_dirty_ = true;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_GLOBAL_RULE_SET_H_
