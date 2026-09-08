// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_PROXY_RESOLUTION_PROXY_INFO_H_
#define NET_PROXY_RESOLUTION_PROXY_INFO_H_

#include "base/check.h"
#include "net/base/net_export.h"
#include "net/base/proxy_chain.h"

namespace net {

// Records whether a stream has selected the direct connection.
class NET_EXPORT ProxyInfo {
 public:
  static ProxyInfo Direct();
  ProxyInfo();
  ProxyInfo(const ProxyInfo& other);
  ~ProxyInfo();

  void UseDirect();
  bool is_direct() const { return !is_empty() && proxy_chain_.is_direct(); }
  bool is_direct_only() const { return is_direct(); }
  bool is_empty() const { return !proxy_chain_.IsValid(); }

  template <class Predicate>
  bool AnyProxyInChain(Predicate p) const {
    return !is_empty() && proxy_chain_.AnyProxy(p);
  }

  bool is_for_ip_protection() const {
    return !is_empty() && proxy_chain_.is_for_ip_protection();
  }
  const ProxyChain& proxy_chain() const {
    CHECK(!is_empty());
    return proxy_chain_;
  }

 private:
  ProxyChain proxy_chain_;
};

}  // namespace net

#endif  // NET_PROXY_RESOLUTION_PROXY_INFO_H_
