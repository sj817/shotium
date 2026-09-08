// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_PROXY_SERVER_H_
#define NET_BASE_PROXY_SERVER_H_

#include <stdint.h>

#include "net/base/host_port_pair.h"
#include "net/base/net_export.h"

namespace base {
class Pickle;
class PickleIterator;
}  // namespace base

namespace net {

// ProxyServer encodes the {type, host, port} of a proxy server.
// ProxyServer is immutable.
class NET_EXPORT ProxyServer {
 public:
  // The type of proxy. Existing bit values are retained.
  enum Scheme {
    SCHEME_INVALID = 1 << 0,
    // SCHEME_DIRECT (value = 1 << 1) is no longer used or supported.
    SCHEME_HTTP = 1 << 2,
    SCHEME_SOCKS4 = 1 << 3,
    SCHEME_SOCKS5 = 1 << 4,
    SCHEME_HTTPS = 1 << 5,
    // A QUIC proxy is an HTTP proxy in which QUIC is used as the transport,
    // instead of TCP.
    SCHEME_QUIC = 1 << 6,
  };

  // Default copy-constructor and assignment operator are OK!

  // Constructs an invalid ProxyServer.
  ProxyServer() = default;

  ProxyServer(Scheme scheme, const HostPortPair& host_port_pair);

  static ProxyServer CreateFromPickle(base::PickleIterator* pickle_iter);

  void Persist(base::Pickle* pickle) const;

  bool is_valid() const { return scheme_ != SCHEME_INVALID; }

  // Returns true if this ProxyServer is an HTTPS proxy. Note this
  // does not include proxies matched by |is_quic()|.
  //
  bool is_https() const { return scheme_ == SCHEME_HTTPS; }

  // Returns true if this ProxyServer is a QUIC proxy.
  bool is_quic() const { return scheme_ == SCHEME_QUIC; }

 private:
  Scheme scheme_ = SCHEME_INVALID;
  HostPortPair host_port_pair_;
};

}  // namespace net

#endif  // NET_BASE_PROXY_SERVER_H_
