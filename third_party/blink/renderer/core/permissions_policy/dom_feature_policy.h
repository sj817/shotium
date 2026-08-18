// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PERMISSIONS_POLICY_DOM_FEATURE_POLICY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PERMISSIONS_POLICY_DOM_FEATURE_POLICY_H_

#include "services/network/public/cpp/permissions_policy/permissions_policy_declaration.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

class SecurityOrigin;

// DOMFeaturePolicy provides an interface for permissions policy introspection
// of a document (DocumentPolicy) or an iframe (IFramePolicy).
class CORE_EXPORT DOMFeaturePolicy : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit DOMFeaturePolicy(ExecutionContext*);
  ~DOMFeaturePolicy() override = default;

  // The introspection methods of the `FeaturePolicy` IDL interface
  // (allowsFeature/features/allowedFeatures/getAllowlistForFeature) used to
  // live here. They were reachable only from `document.featurePolicy` /
  // `iframe.featurePolicy` in JavaScript and had no C++ callers, so they are
  // gone. The policy itself is still built and enforced; only the JS-facing
  // introspection of it is removed.

  // Inform the DOMFeaturePolicy object when the container policy on its frame
  // element has changed.
  virtual void UpdateContainerPolicy(
      const network::ParsedPermissionsPolicy& container_policy,
      const SecurityOrigin& src_origin) {}

  void Trace(Visitor*) const override;

 protected:
  virtual const network::PermissionsPolicy* GetPolicy() const {
    return context_->GetSecurityContext().GetPermissionsPolicy();
  }

  virtual bool IsIFramePolicy() const { return false; }

  Member<ExecutionContext> context_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PERMISSIONS_POLICY_DOM_FEATURE_POLICY_H_
