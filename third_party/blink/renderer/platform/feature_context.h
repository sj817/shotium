// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_FEATURE_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_FEATURE_CONTEXT_H_

#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

class RuntimeFeatureStateOverrideContext;

// A pure virtual interface for checking a context's runtime feature overrides.
class PLATFORM_EXPORT FeatureContext {
 public:
  virtual RuntimeFeatureStateOverrideContext*
  GetRuntimeFeatureStateOverrideContext() const = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_FEATURE_CONTEXT_H_
