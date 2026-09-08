// Copyright 2010 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/proxy_server.h"

#include <stdint.h>

#include <string>

#include "base/check_op.h"
#include "base/pickle.h"

namespace net {

namespace {

bool IsValidSchemeInt(int scheme_int) {
  switch (scheme_int) {
    case ProxyServer::SCHEME_INVALID:
    case ProxyServer::SCHEME_HTTP:
    case ProxyServer::SCHEME_SOCKS4:
    case ProxyServer::SCHEME_SOCKS5:
    case ProxyServer::SCHEME_HTTPS:
    case ProxyServer::SCHEME_QUIC:
      return true;
    default:
      return false;
  }
}

}  // namespace

ProxyServer::ProxyServer(Scheme scheme, const HostPortPair& host_port_pair)
      : scheme_(scheme), host_port_pair_(host_port_pair) {
  if (scheme_ == SCHEME_INVALID) {
    // |host_port_pair| isn't relevant for these special schemes, so none should
    // have been specified. It is important for this to be consistent since we
    // do raw field comparisons in the equality and comparison functions.
    DCHECK_EQ(host_port_pair, HostPortPair());
    host_port_pair_ = HostPortPair();
  }
}

// static
ProxyServer ProxyServer::CreateFromPickle(base::PickleIterator* pickle_iter) {
  Scheme scheme = SCHEME_INVALID;
  int scheme_int;
  if (pickle_iter->ReadInt(&scheme_int) && IsValidSchemeInt(scheme_int)) {
    scheme = static_cast<Scheme>(scheme_int);
  }

  HostPortPair host_port_pair;
  std::string host_port_pair_string;
  if (pickle_iter->ReadString(&host_port_pair_string)) {
    host_port_pair = HostPortPair::FromString(host_port_pair_string);
  }

  return ProxyServer(scheme, host_port_pair);
}

void ProxyServer::Persist(base::Pickle* pickle) const {
  pickle->WriteInt(static_cast<int>(scheme_));
  pickle->WriteString(host_port_pair_.ToString());
}

}  // namespace net
