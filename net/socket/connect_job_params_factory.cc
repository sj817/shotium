// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/connect_job_params_factory.h"

#include <vector>

#include "base/check.h"
#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_util.h"
#include "net/base/features.h"
#include "net/base/host_port_pair.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/privacy_mode.h"
#include "net/base/proxy_chain.h"
#include "net/base/proxy_server.h"
#include "net/base/request_priority.h"
#include "net/dns/public/secure_dns_policy.h"
#include "net/socket/connect_job_params.h"
#include "net/socket/next_proto.h"
#include "net/socket/socket_tag.h"
#include "net/socket/ssl_connect_job.h"
#include "net/socket/transport_connect_job.h"
#include "net/ssl/ssl_config.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"

namespace net {

namespace {

// Configure ALPN and retain HttpServerProperties HTTP/1.1 overrides.
void ConfigureAlpn(const url::SchemeHostPort& endpoint,
                   ConnectJobFactory::AlpnMode alpn_mode,
                   const NetworkAnonymizationKey& network_anonymization_key,
                   const CommonConnectJobParams& common_connect_job_params,
                   SSLConfig& ssl_config,
                   bool renego_allowed) {
  DCHECK_EQ(alpn_mode, ConnectJobFactory::AlpnMode::kHttpAll);
  ssl_config.alpn_protos = *common_connect_job_params.alpn_protos;
  ssl_config.application_settings =
      *common_connect_job_params.application_settings;
  if (common_connect_job_params.http_server_properties) {
    common_connect_job_params.http_server_properties->MaybeForceHTTP11(
        endpoint, network_anonymization_key, &ssl_config);
  }

  // Prior to HTTP/2 and SPDY, some servers used TLS renegotiation to request
  // TLS client authentication after the HTTP request was sent. Allow
  // renegotiation for only those connections.
  //
  // Note that this does NOT implement the provision in
  // https://http2.github.io/http2-spec/#rfc.section.9.2.1 which allows the
  // server to request a renegotiation immediately before sending the
  // connection preface as waiting for the preface would cost the round trip
  // that False Start otherwise saves.
  ssl_config.renego_allowed_default = renego_allowed;
  if (renego_allowed) {
    ssl_config.renego_allowed_for_protos = {NextProto::kProtoHTTP11};
  }
}

base::flat_set<std::string> SupportedProtocolsFromSSLConfig(
    const SSLConfig& config) {
  // We convert because `SSLConfig` uses `NextProto` for ALPN protocols while
  // `TransportConnectJob` and DNS logic needs `std::string`. See
  // https://crbug.com/1286835.
  return base::MakeFlatSet<std::string>(config.alpn_protos, /*comp=*/{},
                                        NextProtoToString);
}

bool UsingSsl(const url::SchemeHostPort& endpoint) {
  return GURL::SchemeIsCryptographic(base::ToLowerASCII(endpoint.scheme()));
}

ConnectJobParams MakeSSLSocketParams(
    ConnectJobParams params,
    const HostPortPair& host_and_port,
    const SSLConfig& ssl_config,
    const NetworkAnonymizationKey& network_anonymization_key) {
  return ConnectJobParams(base::MakeRefCounted<SSLSocketParams>(
      std::move(params), host_and_port, ssl_config, network_anonymization_key));
}

}  // namespace

ConnectJobParams ConstructConnectJobParams(
    const url::SchemeHostPort& endpoint,
    const ProxyChain& proxy_chain,
    const std::vector<SSLConfig::CertAndStatus>& allowed_bad_certs,
    ConnectJobFactory::AlpnMode alpn_mode,
    PrivacyMode privacy_mode,
    const OnHostResolutionCallback& resolution_callback,
    const NetworkAnonymizationKey& endpoint_network_anonymization_key,
    SecureDnsPolicy secure_dns_policy,
    bool disable_cert_network_fetches,
    const CommonConnectJobParams* common_connect_job_params,
    handles::NetworkHandle target_network) {
  CHECK(proxy_chain.is_direct());

  // Set up `ssl_config` if using SSL to the endpoint.
  SSLConfig ssl_config;
  if (UsingSsl(endpoint)) {
    ssl_config.allowed_bad_certs = allowed_bad_certs;
    ssl_config.privacy_mode = privacy_mode;

    ConfigureAlpn(endpoint, alpn_mode, endpoint_network_anonymization_key,
                  *common_connect_job_params, ssl_config,
                  /*renego_allowed=*/true);

    ssl_config.disable_cert_verification_network_fetches =
        disable_cert_network_fetches;

    // TODO(crbug.com/41459647): Also enable 0-RTT for TLS proxies.
    ssl_config.early_data_enabled =
        *common_connect_job_params->enable_early_data;

    ssl_config.proxy_chain = proxy_chain;
    ssl_config.proxy_chain_index = proxy_chain.length();
    ssl_config.session_usage = SessionUsage::kDestination;
  }

  // Create the nested parameters over which the connection to the endpoint
  // will be made.
  ConnectJobParams params(base::MakeRefCounted<TransportSocketParams>(
      endpoint, endpoint_network_anonymization_key,
      secure_dns_policy, target_network, resolution_callback,
      SupportedProtocolsFromSSLConfig(ssl_config)));

  if (UsingSsl(endpoint)) {
    // Wrap the direct transport in SSLSocketParams for the endpoint.
    // TODO(crbug.com/40181080): Pass `endpoint` directly (preserving scheme
    // when available)?
    params =
        MakeSSLSocketParams(std::move(params), HostPortPair::FromSchemeHostPort(endpoint),
                            ssl_config, endpoint_network_anonymization_key);
  }

  return params;
}

}  // namespace net
