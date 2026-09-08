// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/connect_job_params.h"

#include "net/socket/ssl_connect_job.h"
#include "net/socket/transport_connect_job.h"

namespace net {

ConnectJobParams::ConnectJobParams() = default;
ConnectJobParams::ConnectJobParams(scoped_refptr<TransportSocketParams> params)
    : params_(params) {}
ConnectJobParams::ConnectJobParams(scoped_refptr<SSLSocketParams> params)
    : params_(params) {}

ConnectJobParams::~ConnectJobParams() = default;

ConnectJobParams::ConnectJobParams(ConnectJobParams&) = default;
ConnectJobParams& ConnectJobParams::operator=(ConnectJobParams&) = default;
ConnectJobParams::ConnectJobParams(ConnectJobParams&&) = default;
ConnectJobParams& ConnectJobParams::operator=(ConnectJobParams&&) = default;

}  // namespace net
