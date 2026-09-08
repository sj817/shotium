// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/network_param_mojom_traits.h"

#include <string_view>

#include "base/compiler_specific.h"
#include "base/memory/scoped_refptr.h"
#include "base/notreached.h"
#include "base/strings/string_view_util.h"
#include "mojo/public/cpp/base/time_mojom_traits.h"
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/signed_certificate_timestamp.h"
#include "net/log/net_log_source_type.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "services/network/public/cpp/ct_policy_status_mojom_traits.h"
#include "services/network/public/cpp/http_response_headers_mojom_traits.h"
#include "services/network/public/cpp/ocsp_verify_result_mojom_traits.h"
#include "services/network/public/cpp/signed_certificate_timestamp_and_status_mojom_traits.h"
#include "services/network/public/cpp/x509_certificate_mojom_traits.h"
#include "third_party/boringssl/src/include/openssl/pool.h"

namespace mojo {

// static
bool StructTraits<network::mojom::HttpVersionDataView, net::HttpVersion>::Read(
    network::mojom::HttpVersionDataView data,
    net::HttpVersion* out) {
  *out = net::HttpVersion(data.major_value(), data.minor_value());
  return true;
}

// static
bool StructTraits<
    network::mojom::ResolveErrorInfoDataView,
    net::ResolveErrorInfo>::Read(network::mojom::ResolveErrorInfoDataView data,
                                 net::ResolveErrorInfo* out) {
  // There should not be a secure network error if the error code indicates no
  // error.
  if (data.error() == net::OK && data.is_secure_network_error()) {
    return false;
  }
  *out = net::ResolveErrorInfo(data.error(), data.is_secure_network_error());
  return true;
}

// static
bool StructTraits<network::mojom::HostPortPairDataView, net::HostPortPair>::
    Read(network::mojom::HostPortPairDataView data, net::HostPortPair* out) {
  std::string host;
  if (!data.ReadHost(&host)) {
    return false;
  }
  *out = net::HostPortPair(std::move(host), data.port());
  return true;
}

// static
bool StructTraits<network::mojom::SSLCertRequestInfoDataView,
                  scoped_refptr<net::SSLCertRequestInfo>>::
    Read(network::mojom::SSLCertRequestInfoDataView data,
         scoped_refptr<net::SSLCertRequestInfo>* out) {
  net::HostPortPair host_and_port;
  if (!data.ReadHostAndPort(&host_and_port)) {
    return false;
  }
  std::vector<std::string> cert_authorities;
  if (!data.ReadCertAuthorities(&cert_authorities)) {
    return false;
  }
  std::vector<uint16_t> signature_algorithms;
  if (!data.ReadSignatureAlgorithms(&signature_algorithms)) {
    return false;
  }

  auto ssl_cert_request_info = base::MakeRefCounted<net::SSLCertRequestInfo>();
  ssl_cert_request_info->host_and_port = std::move(host_and_port);
  ssl_cert_request_info->cert_authorities = std::move(cert_authorities);
  ssl_cert_request_info->signature_algorithms = std::move(signature_algorithms);

  *out = ssl_cert_request_info;

  return true;
}

}  // namespace mojo
