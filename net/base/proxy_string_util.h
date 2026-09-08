// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_PROXY_STRING_UTIL_H_
#define NET_BASE_PROXY_STRING_UTIL_H_

#include <string>
#include "net/base/net_export.h"
#include "net/base/proxy_server.h"

namespace net {

// Formats proxy values for existing stream and connection diagnostics.
NET_EXPORT std::string ProxyServerToPacResultElement(
    const ProxyServer& proxy_server);
NET_EXPORT std::string ProxyServerToProxyUri(const ProxyServer& proxy_server);

}  // namespace net

#endif  // NET_BASE_PROXY_STRING_UTIL_H_
