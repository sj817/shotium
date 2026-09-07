// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_PROXY_FALLBACK_H_
#define NET_HTTP_PROXY_FALLBACK_H_

#include "net/base/net_export.h"

namespace net {

class ProxyChain;
class ProxyDelegate;

// Returns true if a failed request issued through a proxy chain should be
// re-tried using the next proxy chain in the fallback list.
//
// The proxy fallback logic is a compromise between compatibility and
// increasing odds of success, and may choose not to retry a request on the
// next proxy option, even though that could work.
//
//  - `proxy_chain` is the proxy chain that failed the request.
//  - `error` is the error for the request when it was sent through
//    `proxy_chain`.
//  - `final_error` is an out parameter that is set with the "final" error to
//    report to the caller. The error is only re-written in cases where
//    CanFalloverToNextProxy() returns false.
//  - `proxy_delegate` if present, is used to possibly override the return value
//    of this function. See ProxyDelegate::CanFalloverToNextProxyOverride
//    documentation.
NET_EXPORT bool CanFalloverToNextProxy(const ProxyChain& proxy_chain,
                                       int error,
                                       int* final_error,
                                       net::ProxyDelegate* proxy_delegate);

}  // namespace net

#endif  // NET_HTTP_PROXY_FALLBACK_H_
