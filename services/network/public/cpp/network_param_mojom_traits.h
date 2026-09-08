// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_NETWORK_PARAM_MOJOM_TRAITS_H_
#define SERVICES_NETWORK_PUBLIC_CPP_NETWORK_PARAM_MOJOM_TRAITS_H_

#include <string>
#include <vector>

#include "base/component_export.h"
#include "mojo/public/cpp/base/string16_mojom_traits.h"
#include "mojo/public/cpp/bindings/enum_traits.h"
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "net/base/auth.h"
#include "net/base/host_port_pair.h"
#include "net/cert/signed_certificate_timestamp_and_status.h"
#include "net/dns/public/resolve_error_info.h"
#include "net/http/http_version.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "net/ssl/ssl_info.h"
#include "services/network/public/cpp/hash_value_mojom_traits.h"
#include "services/network/public/mojom/network_param.mojom-shared.h"
#include "url/mojom/scheme_host_port_mojom_traits.h"

namespace mojo {

template <>
class COMPONENT_EXPORT(NETWORK_CPP_NETWORK_PARAM)
    StructTraits<network::mojom::HttpVersionDataView, net::HttpVersion> {
 public:
  static uint16_t major_value(net::HttpVersion version) {
    return version.major_value();
  }
  static uint16_t minor_value(net::HttpVersion version) {
    return version.minor_value();
  }

  static bool Read(network::mojom::HttpVersionDataView data,
                   net::HttpVersion* out);
};

template <>
class COMPONENT_EXPORT(NETWORK_CPP_NETWORK_PARAM)
    StructTraits<network::mojom::ResolveErrorInfoDataView,
                 net::ResolveErrorInfo> {
 public:
  static int error(const net::ResolveErrorInfo& resolve_error_info) {
    return resolve_error_info.error;
  }

  static bool is_secure_network_error(
      const net::ResolveErrorInfo& resolve_error_info) {
    return resolve_error_info.is_secure_network_error;
  }

  static bool Read(network::mojom::ResolveErrorInfoDataView data,
                   net::ResolveErrorInfo* out);
};

template <>
class COMPONENT_EXPORT(NETWORK_CPP_NETWORK_PARAM)
    StructTraits<network::mojom::HostPortPairDataView, net::HostPortPair> {
 public:
  static const std::string& host(const net::HostPortPair& host_port_pair) {
    return host_port_pair.host();
  }

  static uint16_t port(const net::HostPortPair& host_port_pair) {
    return host_port_pair.port();
  }

  static bool Read(network::mojom::HostPortPairDataView data,
                   net::HostPortPair* out);
};

template <>
class COMPONENT_EXPORT(NETWORK_CPP_NETWORK_PARAM)
    StructTraits<network::mojom::SSLCertRequestInfoDataView,
                 scoped_refptr<net::SSLCertRequestInfo>> {
 public:
  static bool IsNull(const scoped_refptr<net::SSLCertRequestInfo>& r) {
    return !r;
  }

  static void SetToNull(scoped_refptr<net::SSLCertRequestInfo>* output) {
    *output = nullptr;
  }

  static const net::HostPortPair& host_and_port(
      const scoped_refptr<net::SSLCertRequestInfo>& s) {
    return s->host_and_port;
  }

  static bool is_proxy(const scoped_refptr<net::SSLCertRequestInfo>& s) {
    return s->is_proxy;
  }

  static const std::vector<std::string>& cert_authorities(
      const scoped_refptr<net::SSLCertRequestInfo>& s) {
    return s->cert_authorities;
  }

  static const std::vector<uint16_t>& signature_algorithms(
      const scoped_refptr<net::SSLCertRequestInfo>& s) {
    return s->signature_algorithms;
  }

  static bool Read(network::mojom::SSLCertRequestInfoDataView data,
                   scoped_refptr<net::SSLCertRequestInfo>* out);
};

}  // namespace mojo

#endif  // SERVICES_NETWORK_PUBLIC_CPP_NETWORK_PARAM_MOJOM_TRAITS_H_
