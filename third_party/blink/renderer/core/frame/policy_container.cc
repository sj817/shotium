// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/policy_container.h"

#include <tuple>

#include "services/network/public/cpp/web_sandbox_flags.h"
#include "services/network/public/mojom/integrity_policy.mojom-blink.h"
#include "third_party/blink/renderer/platform/loader/fetch/policy_container_utils.h"

namespace blink {

PolicyContainer::PolicyContainer(
    mojo::PendingAssociatedRemote<mojom::blink::PolicyContainerHost> remote,
    mojom::blink::PolicyContainerPoliciesPtr policies)
    : policies_(std::move(policies)),
      policy_container_host_remote_(std::move(remote)) {}

// static
std::unique_ptr<PolicyContainer> PolicyContainer::CreateEmpty() {
  // No host. Upstream binds a dummy PolicyContainerHost remote here -- a
  // message pipe with nothing on the far end -- so that every update could be
  // sent and ignored. The updates are skipped instead when there is nobody to
  // send them to (see UpdateReferrerPolicy), which is the same outcome
  // without a pipe per frame and a serialised message per policy.
  return std::make_unique<PolicyContainer>(
      mojo::NullAssociatedRemote(),
      mojom::blink::PolicyContainerPolicies::New());
}

// static
std::unique_ptr<PolicyContainer> PolicyContainer::CreateFromWebPolicyContainer(
    std::unique_ptr<WebPolicyContainer> container) {
  if (!container)
    return nullptr;

  return std::make_unique<PolicyContainer>(
      std::move(container->remote),
      FromWebPolicyContainerPolicies(container->policies));
}

network::mojom::blink::ReferrerPolicy PolicyContainer::GetReferrerPolicy()
    const {
  return policies_->referrer_policy;
}

void PolicyContainer::UpdateReferrerPolicy(
    network::mojom::blink::ReferrerPolicy policy,
    const InitiatorStateToken& initiator_state_token) {
  policies_->referrer_policy = policy;

  // The local policy is the one that matters here; the host is told when
  // there is one. An empty container (CreateEmpty) has none, and a frame in
  // a process with no browser -- shot's -- gets nothing but empty containers.
  if (policy_container_host_remote_.is_bound()) {
    policy_container_host_remote_->SetReferrerPolicy(policy,
                                                     initiator_state_token);
  }
}

const mojom::blink::PolicyContainerPolicies& PolicyContainer::GetPolicies()
    const {
  return *policies_;
}

void PolicyContainer::AddContentSecurityPolicies(
    Vector<network::mojom::blink::ContentSecurityPolicyPtr> policies,
    const InitiatorStateToken& initiator_state_token) {
  for (const auto& policy : policies) {
    policies_->content_security_policies.push_back(policy->Clone());
  }

  if (policy_container_host_remote_.is_bound()) {
    policy_container_host_remote_->AddContentSecurityPolicies(
        std::move(policies), initiator_state_token);
  }
}

}  // namespace blink
