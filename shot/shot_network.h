// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_NETWORK_H_
#define SHOT_SHOT_NETWORK_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/types/expected.h"

namespace net {
class NetworkChangeNotifier;
class URLRequestContext;
}  // namespace net

namespace shot {

// How the process-wide network stack is configured. Everything here is a
// process-level decision rather than a per-request one, which is why it comes
// off the command line and not out of a ScreenshotRequest.
struct NetworkConfig {
  // Where the HTTP cache lives. Empty disables caching entirely.
  //
  // One directory per worker: the Simple backend takes an exclusive lock on
  // its directory, so two workers pointed at the same path means the second
  // one silently runs without a cache.
  base::FilePath cache_dir;

  // 0 lets the backend size itself from the free space on the volume.
  int cache_max_bytes = 0;

  // Empty means the built-in string.
  std::string user_agent;
};

// The process's one net::URLRequestContext.
//
// This is //net used directly -- URLRequest, HttpCache, HttpNetworkSession,
// BoringSSL -- with no //services/network above it. That service is the mojo
// wrapper multi-process Chrome puts around exactly these objects so that a
// sandboxed renderer can reach them; a shot worker is already its own process
// and owns its own stack, so the wrapper would be a pipe to itself.
//
// It must be created on, and used from, the thread that runs blink, and that
// thread must have an IO message pump: net watches sockets through
// base::CurrentIOThread.
class ShotNetwork {
 public:
  static base::expected<std::unique_ptr<ShotNetwork>, std::string> Create(
      const NetworkConfig& config);

  ShotNetwork(const ShotNetwork&) = delete;
  ShotNetwork& operator=(const ShotNetwork&) = delete;
  ~ShotNetwork();

  // The live context, or null when networking was never brought up. Global
  // because the thing that needs it -- ShotURLLoader -- is constructed by
  // blink's ResourceFetcher, several layers below anything that could have
  // been handed a pointer.
  static net::URLRequestContext* Get();

  // The User-Agent every request carries. Exposed because blink also reports it
  // to the document (navigator.userAgent, and the UA client hints), and the two
  // disagreeing is the kind of thing that makes a server serve one page and the
  // screenshot show another.
  static const std::string& UserAgent();

 private:
  ShotNetwork();

  std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier_;
  std::unique_ptr<net::URLRequestContext> context_;
};

}  // namespace shot

#endif  // SHOT_SHOT_NETWORK_H_
