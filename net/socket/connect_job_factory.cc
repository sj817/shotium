// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/connect_job_factory.h"

#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "base/check.h"
#include "base/memory/scoped_refptr.h"
#include "net/base/host_port_pair.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/network_handle.h"
#include "net/base/privacy_mode.h"
#include "net/base/proxy_chain.h"
#include "net/base/request_priority.h"
#include "net/dns/public/secure_dns_policy.h"
#include "net/socket/connect_job.h"
#include "net/socket/connect_job_params_factory.h"
#include "net/socket/socket_tag.h"
#include "net/socket/ssl_connect_job.h"
#include "net/socket/transport_connect_job.h"
#include "net/ssl/ssl_config.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/scheme_host_port.h"

namespace net {

namespace {

template <typename T>
std::unique_ptr<T> CreateFactoryIfNull(std::unique_ptr<T> in) {
  if (in) {
    return in;
  }
  return std::make_unique<T>();
}

}  // namespace

ConnectJobFactory::ConnectJobFactory(
    std::unique_ptr<SSLConnectJob::Factory> ssl_connect_job_factory,
    std::unique_ptr<TransportConnectJob::Factory> transport_connect_job_factory)
    : ssl_connect_job_factory_(
          CreateFactoryIfNull(std::move(ssl_connect_job_factory))),
      transport_connect_job_factory_(
          CreateFactoryIfNull(std::move(transport_connect_job_factory))) {}

ConnectJobFactory::~ConnectJobFactory() = default;

std::unique_ptr<ConnectJob> ConnectJobFactory::CreateConnectJob(
    url::SchemeHostPort endpoint,
    const ProxyChain& proxy_chain,
    const std::vector<SSLConfig::CertAndStatus>& allowed_bad_certs,
    ConnectJobFactory::AlpnMode alpn_mode,
    PrivacyMode privacy_mode,
    const OnHostResolutionCallback& resolution_callback,
    RequestPriority request_priority,
    SocketTag socket_tag,
    const NetworkAnonymizationKey& network_anonymization_key,
    SecureDnsPolicy secure_dns_policy,
    bool disable_cert_network_fetches,
    const CommonConnectJobParams* common_connect_job_params,
    handles::NetworkHandle target_network,
    ConnectJob::Delegate* delegate) const {
  return CreateConnectJob(Endpoint(std::move(endpoint)), proxy_chain,
                          allowed_bad_certs, alpn_mode,
                          privacy_mode, resolution_callback,
                          request_priority, socket_tag,
                          network_anonymization_key, secure_dns_policy,
                          disable_cert_network_fetches,
                          common_connect_job_params, target_network, delegate);
}

std::unique_ptr<ConnectJob> ConnectJobFactory::CreateConnectJob(
    bool using_ssl,
    HostPortPair endpoint,
    const ProxyChain& proxy_chain,
    PrivacyMode privacy_mode,
    const OnHostResolutionCallback& resolution_callback,
    RequestPriority request_priority,
    SocketTag socket_tag,
    const NetworkAnonymizationKey& network_anonymization_key,
    SecureDnsPolicy secure_dns_policy,
    const CommonConnectJobParams* common_connect_job_params,
    handles::NetworkHandle target_network,
    ConnectJob::Delegate* delegate) const {
  SchemelessEndpoint schemeless_endpoint{using_ssl, std::move(endpoint)};
  return CreateConnectJob(
      std::move(schemeless_endpoint), proxy_chain,
      /*allowed_bad_certs=*/{}, ConnectJobFactory::AlpnMode::kDisabled,
      privacy_mode, resolution_callback, request_priority,
      socket_tag, network_anonymization_key, secure_dns_policy,
      /*disable_cert_network_fetches=*/false, common_connect_job_params,
      target_network, delegate);
}

std::unique_ptr<ConnectJob> ConnectJobFactory::CreateConnectJob(
    Endpoint endpoint,
    const ProxyChain& proxy_chain,
    const std::vector<SSLConfig::CertAndStatus>& allowed_bad_certs,
    ConnectJobFactory::AlpnMode alpn_mode,
    PrivacyMode privacy_mode,
    const OnHostResolutionCallback& resolution_callback,
    RequestPriority request_priority,
    SocketTag socket_tag,
    const NetworkAnonymizationKey& network_anonymization_key,
    SecureDnsPolicy secure_dns_policy,
    bool disable_cert_network_fetches,
    const CommonConnectJobParams* common_connect_job_params,
    handles::NetworkHandle target_network,
    ConnectJob::Delegate* delegate) const {
  ConnectJobParams connect_job_params = ConstructConnectJobParams(
      endpoint, proxy_chain, allowed_bad_certs, alpn_mode,
      privacy_mode, resolution_callback,
      network_anonymization_key, secure_dns_policy,
      disable_cert_network_fetches, common_connect_job_params,
      target_network);

  if (connect_job_params.is_ssl()) {
    return ssl_connect_job_factory_->Create(
        request_priority, socket_tag, common_connect_job_params,
        connect_job_params.take_ssl(), delegate, /*net_log=*/nullptr);
  }

  CHECK(connect_job_params.is_transport());
  return transport_connect_job_factory_->Create(
      request_priority, socket_tag, common_connect_job_params,
      connect_job_params.take_transport(), delegate, /*net_log=*/nullptr);

}

}  // namespace net
