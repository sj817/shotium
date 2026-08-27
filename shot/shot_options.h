// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_OPTIONS_H_
#define SHOT_SHOT_OPTIONS_H_

#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "url/gurl.h"

namespace shot {

inline constexpr int kSuccessExitCode = 0;
inline constexpr int kUsageExitCode = 2;
inline constexpr int kNavigationExitCode = 3;
inline constexpr int kCaptureExitCode = 4;

struct ShotOptions {
  bool show_help = false;
  bool verbose = false;
  // Resident worker mode: no input on the command line, requests arrive
  // on stdin. See shot_server.h for the framing.
  bool serve = false;
  // The default for a --serve request that does not set allowFileAccess. The
  // command line's own request always allows it -- pointing shot at a local
  // file is the decision -- so this only moves the default for requests that
  // arrive over the protocol, where the sender may not be the operator.
  bool allow_file_access = false;
  bool read_stdin = false;
  bool force_file_input = false;
  std::string input;
  int width = 1280;
  int height = 720;
  double scale = 1.0;
  base::FilePath output_path =
      base::FilePath(FILE_PATH_LITERAL("screenshot.png"));
  base::TimeDelta timeout = base::Seconds(30);

  // What to capture, mirroring ScreenshotOptions. Kept here rather than being
  // parsed straight into a ScreenshotRequest so that the command line can go on
  // reporting its own errors with its own flag names.
  bool full_page = false;
  std::string selector;
  std::string type = "png";
  std::string png_compression = "fast";
  bool png_compression_set = false;
  std::optional<int> quality;
  bool omit_background = false;
  std::string wait_until = "load";

  // Process-level, not per-shot: the network stack is built once, before any
  // request is read. See shot_network.h.
  base::FilePath cache_dir;
  // 0 lets the backend size itself from the volume's free space, which is what
  // it did unconditionally before there was a flag for this.
  int cache_max_bytes = 0;
  std::string user_agent;
};

// Owns any temporary file used to make stdin HTML navigable for as long as the
// browser and renderer processes may need it.
struct PreparedShot {
  ShotOptions options;
  GURL target_url;
  base::ScopedTempDir stdin_temp_dir;
};

base::expected<ShotOptions, std::string> ParseShotOptions(
    const std::vector<std::string>& argv);

base::expected<PreparedShot, std::string> PrepareShot(ShotOptions options);

std::string GetUsage();

}  // namespace shot

#endif  // SHOT_SHOT_OPTIONS_H_
