// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/client_socket_pool_manager_impl.h"

#include "net/http/http_network_session.h"
#include "net/socket/transport_client_socket_pool.h"

namespace net {

ClientSocketPoolManagerImpl::ClientSocketPoolManagerImpl(
    const CommonConnectJobParams& common_connect_job_params,
    HttpNetworkSession::SocketPoolType pool_type,
    bool cleanup_on_ip_address_change)
    : common_connect_job_params_(common_connect_job_params),
      pool_type_(pool_type),
      cleanup_on_ip_address_change_(cleanup_on_ip_address_change) {}

ClientSocketPoolManagerImpl::~ClientSocketPoolManagerImpl() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void ClientSocketPoolManagerImpl::FlushSocketPoolsWithError(
    int net_error,
    const char* net_log_reason_utf8) {
  if (socket_pool_) {
    socket_pool_->FlushWithError(net_error, net_log_reason_utf8);
  }
}

void ClientSocketPoolManagerImpl::CloseIdleSockets(
    const char* net_log_reason_utf8) {
  if (socket_pool_) {
    socket_pool_->CloseIdleSockets(net_log_reason_utf8);
  }
}

ClientSocketPool* ClientSocketPoolManagerImpl::GetSocketPool() {
  if (!socket_pool_) {
    socket_pool_ = std::make_unique<TransportClientSocketPool>(
        socket_soft_cap_per_pool(pool_type_), max_sockets_per_group(pool_type_),
        unused_idle_socket_timeout(pool_type_), &common_connect_job_params_,
        cleanup_on_ip_address_change_);
  }
  return socket_pool_.get();
}

}  // namespace net
