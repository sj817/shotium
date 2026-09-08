// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_LOG_NET_LOG_UTIL_H_
#define NET_LOG_NET_LOG_UTIL_H_

#include "base/trace_event/trace_event.h"  // IWYU pragma: export
#include "net/base/net_export.h"

namespace net {

class NetLogWithSource;

// Creates a trace Flow from a NetLogWithSource.
NET_EXPORT perfetto::Flow NetLogWithSourceToFlow(
    const NetLogWithSource& net_log);

}  // namespace net

#endif  // NET_LOG_NET_LOG_UTIL_H_
