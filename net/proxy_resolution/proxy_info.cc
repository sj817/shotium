// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/proxy_resolution/proxy_info.h"

namespace net {

ProxyInfo ProxyInfo::Direct() {
  ProxyInfo proxy_info;
  proxy_info.UseDirect();
  return proxy_info;
}

ProxyInfo::ProxyInfo() = default;
ProxyInfo::ProxyInfo(const ProxyInfo& other) = default;
ProxyInfo::~ProxyInfo() = default;

void ProxyInfo::UseDirect() {
  proxy_chain_ = ProxyChain::Direct();
}

}  // namespace net
