// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/proxy_chain.h"

#include <vector>

#include "base/check.h"
#include "base/pickle.h"
#include "build/buildflag.h"
#include "net/base/proxy_server.h"
#include "net/net_buildflags.h"

namespace net {

namespace {
bool ShouldAllowQuicForAllChains() {
  bool should_allow = false;

#if BUILDFLAG(ENABLE_QUIC_PROXY_SUPPORT)
  should_allow = true;
#endif  // BUILDFLAG(ENABLE_QUIC_PROXY_SUPPORT)

  return should_allow;
}
}  // namespace

ProxyChain::ProxyChain() = default;

ProxyChain::ProxyChain(const ProxyChain& other) = default;
ProxyChain::ProxyChain(ProxyChain&& other) noexcept = default;

ProxyChain& ProxyChain::operator=(const ProxyChain& other) = default;
ProxyChain& ProxyChain::operator=(ProxyChain&& other) noexcept = default;
ProxyChain::~ProxyChain() = default;

// static
std::optional<ProxyChain> ProxyChain::InitFromPickle(
    base::PickleIterator& pickle_iter) {
  int ip_protection_chain_id;
  if (!pickle_iter.ReadInt(&ip_protection_chain_id)) {
    return std::nullopt;
  }
  size_t chain_length = 0;
  if (!pickle_iter.ReadLength(&chain_length)) {
    return std::nullopt;
  }

  std::vector<ProxyServer> proxy_server_list;
  for (size_t i = 0; i < chain_length; ++i) {
    proxy_server_list.push_back(ProxyServer::CreateFromPickle(&pickle_iter));
  }

  ProxyChain chain =
      ProxyChain(std::move(proxy_server_list), ip_protection_chain_id);
  if (!chain.IsValid()) {
    return std::nullopt;
  }
  return chain;
}

void ProxyChain::Persist(base::Pickle* pickle) const {
  DCHECK(IsValid());
  pickle->WriteInt(ip_protection_chain_id_);
  if (length() > static_cast<size_t>(INT_MAX) - 1) {
    pickle->WriteInt(0);
    return;
  }
  pickle->WriteInt(static_cast<int>(length()));
  for (const auto& proxy_server : proxy_server_list_.value()) {
    proxy_server.Persist(pickle);
  }
}

ProxyChain::ProxyChain(std::vector<ProxyServer> proxy_server_list,
                       int ip_protection_chain_id)
    : proxy_server_list_(std::move(proxy_server_list)),
      ip_protection_chain_id_(ip_protection_chain_id) {
  if (!IsValidInternal()) {
    *this = ProxyChain();
  }
}

bool ProxyChain::IsValidInternal() const {
  if (!proxy_server_list_.has_value()) {
    return false;
  }
  if (is_direct()) {
    return true;
  }
  bool should_allow_quic =
      is_for_ip_protection() || ShouldAllowQuicForAllChains();
  if (is_single_proxy()) {
    bool is_valid = proxy_server_list_.value().at(0).is_valid();
    if (proxy_server_list_.value().at(0).is_quic()) {
      is_valid = is_valid && should_allow_quic;
    }
    return is_valid;
  }
  DCHECK(is_multi_proxy());

#if !BUILDFLAG(ENABLE_BRACKETED_PROXY_URIS)
  // A chain can only be multi-proxy in release builds if it is for ip
  // protection.
  if (!is_for_ip_protection() && is_multi_proxy()) {
    return false;
  }
#endif  // !BUILDFLAG(ENABLE_BRACKETED_PROXY_URIS)

  // Verify that the chain is zero or more SCHEME_QUIC servers followed by zero
  // or more SCHEME_HTTPS servers.
  bool seen_quic = false;
  bool seen_https = false;
  for (const auto& proxy_server : proxy_server_list_.value()) {
    if (proxy_server.is_quic()) {
      if (seen_https) {
        // SCHEME_QUIC cannot follow SCHEME_HTTPS.
        return false;
      }
      seen_quic = true;
    } else if (proxy_server.is_https()) {
      seen_https = true;
    } else {
      return false;
    }
  }

  // QUIC is only allowed for IP protection unless in debug builds where it is
  // generally available.
  return !seen_quic || should_allow_quic;
}

}  // namespace net
