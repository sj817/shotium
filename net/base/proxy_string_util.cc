// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/proxy_string_util.h"

#include "base/check.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"

namespace net {
namespace {

std::string ConstructHostPortString(std::string_view hostname, uint16_t port) {
  DCHECK(!hostname.empty());
  DCHECK((hostname.front() == '[' && hostname.back() == ']') ||
         hostname.find(":") == std::string_view::npos);

  return base::StrCat({hostname, ":", base::NumberToString(port)});
}

}  // namespace

std::string ProxyServerToPacResultElement(const ProxyServer& proxy_server) {
  switch (proxy_server.scheme()) {
    case ProxyServer::SCHEME_HTTP:
      return base::StrCat(
          {"PROXY ", ConstructHostPortString(proxy_server.GetHost(),
                                             proxy_server.GetPort())});
    case ProxyServer::SCHEME_SOCKS4:
      // For compatibility send SOCKS instead of SOCKS4.
      return base::StrCat(
          {"SOCKS ", ConstructHostPortString(proxy_server.GetHost(),
                                             proxy_server.GetPort())});
    case ProxyServer::SCHEME_SOCKS5:
      return base::StrCat(
          {"SOCKS5 ", ConstructHostPortString(proxy_server.GetHost(),
                                              proxy_server.GetPort())});
    case ProxyServer::SCHEME_HTTPS:
      return base::StrCat(
          {"HTTPS ", ConstructHostPortString(proxy_server.GetHost(),
                                             proxy_server.GetPort())});
    case ProxyServer::SCHEME_QUIC:
      return base::StrCat(
          {"QUIC ", ConstructHostPortString(proxy_server.GetHost(),
                                            proxy_server.GetPort())});
    default:
      // Got called with an invalid scheme.
      NOTREACHED();
  }
}

std::string ProxyServerToProxyUri(const ProxyServer& proxy_server) {
  switch (proxy_server.scheme()) {
    case ProxyServer::SCHEME_HTTP:
      // Leave off "http://" since it is our default scheme.
      return ConstructHostPortString(proxy_server.GetHost(),
                                     proxy_server.GetPort());
    case ProxyServer::SCHEME_SOCKS4:
      return base::StrCat(
          {"socks4://", ConstructHostPortString(proxy_server.GetHost(),
                                                proxy_server.GetPort())});
    case ProxyServer::SCHEME_SOCKS5:
      return base::StrCat(
          {"socks5://", ConstructHostPortString(proxy_server.GetHost(),
                                                proxy_server.GetPort())});
    case ProxyServer::SCHEME_HTTPS:
      return base::StrCat(
          {"https://", ConstructHostPortString(proxy_server.GetHost(),
                                               proxy_server.GetPort())});
    case ProxyServer::SCHEME_QUIC:
      return base::StrCat(
          {"quic://", ConstructHostPortString(proxy_server.GetHost(),
                                              proxy_server.GetPort())});
    default:
      // Got called with an invalid scheme.
      NOTREACHED();
  }
}

}  // namespace net
