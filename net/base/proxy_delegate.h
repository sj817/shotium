// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_PROXY_DELEGATE_H_
#define NET_BASE_PROXY_DELEGATE_H_

#include <string>

#include "base/types/expected.h"
#include "base/types/optional_ref.h"
#include "net/base/completion_once_callback.h"
#include "net/base/net_errors.h"
#include "net/base/net_export.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/proxy_chain.h"
#include "net/proxy_resolution/proxy_retry_info.h"

class GURL;

namespace net {

class HttpRequestHeaders;
class HttpResponseHeaders;
class ProxyInfo;

// Delegate for setting up a connection.
class NET_EXPORT ProxyDelegate {
 public:
  ProxyDelegate() = default;
  ProxyDelegate(const ProxyDelegate&) = delete;
  ProxyDelegate& operator=(const ProxyDelegate&) = delete;
  virtual ~ProxyDelegate() = default;

  // Called when use of a proxy chain failed due to `net_error`. Allows
  // overriding whether the request should be retried using the next ProxyChain
  // in the fallback list. If not implemented, or if this returns std::nullopt,
  // no override will take place.
  virtual std::optional<bool> CanFalloverToNextProxyOverride(
      const ProxyChain& proxy_chain,
      int net_error);

  using OnBeforeTunnelRequestCallback =
      base::OnceCallback<void(base::expected<HttpRequestHeaders, Error>)>;

  // Called immediately before a proxy tunnel request is sent. Provides the
  // embedder an opportunity to add extra request headers to the request.
  // Returns:
  // - The headers, if they could be computed without blocking. If no headers
  //   should be added, an empty HttpRequestHeaders should be returned.
  // - ERR_IO_PENDING, if the implementor must block to compute the headers.
  // - Any error code, other than ERR_IO_PENDING and OK, if something went
  //   wrong.
  // If ERR_IO_PENDING is returned, `callback` will be called asynchronously.
  // The value passed to `callback` is to be interpreted in the same way as the
  // return value of this function. With the exception that ERR_IO_PENDING is
  // no longer an acceptable error code.
  // `proxy_index` identifies the proxy, within `proxy_chain`, to whom we will
  // be sending the extra headers`.
  virtual base::expected<HttpRequestHeaders, Error> OnBeforeTunnelRequest(
      const ProxyChain& proxy_chain,
      size_t proxy_index,
      OnBeforeTunnelRequestCallback callback) = 0;

  // Called when the response headers for the proxy tunnel request have been
  // received. Allows the delegate to override the net error code of the tunnel
  // request. Returning OK causes the standard tunnel response handling to be
  // performed. `proxy_index` identifies the proxy, within `proxy_chain`, that
  // we're receiving response headers from. Implementations should make sure
  // they can trust said proxy before making decisions based on
  // `response_headers`.
  virtual Error OnTunnelHeadersReceived(
      const ProxyChain& proxy_chain,
      size_t proxy_index,
      const HttpResponseHeaders& response_headers,
      CompletionOnceCallback callback) = 0;

  // Called after a stream creation succeeds or fails. `duration` indicates
  // how long the attempt took, from when the jobs started to when the attempt
  // succeeded or failed.
  virtual void OnStreamCreationAttempted(const ProxyChain& proxy_chain,
                                         base::TimeDelta duration,
                                         base::optional_ref<int> net_error) {}
};

}  // namespace net

#endif  // NET_BASE_PROXY_DELEGATE_H_
