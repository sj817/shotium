// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/log/net_log_util.h"

#include <stdint.h>

#include "net/log/net_log_with_source.h"

namespace net {

perfetto::Flow NetLogWithSourceToFlow(const NetLogWithSource& net_log) {
  const uint64_t flow_id =
      (reinterpret_cast<uint64_t>(net_log.net_log()) << 32) |
      net_log.source().id;
  return perfetto::Flow::ProcessScoped(flow_id);
}

}  // namespace net
