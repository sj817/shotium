// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/permissions_policy/dom_feature_policy.h"

#include "services/network/public/cpp/permissions_policy/permissions_policy.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

DOMFeaturePolicy::DOMFeaturePolicy(ExecutionContext* context)
    : context_(context) {}

void DOMFeaturePolicy::Trace(Visitor* visitor) const {
  ScriptWrappable::Trace(visitor);
  visitor->Trace(context_);
}

}  // namespace blink
