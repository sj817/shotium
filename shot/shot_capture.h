// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_CAPTURE_H_
#define SHOT_SHOT_CAPTURE_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/types/expected.h"
#include "shot/shot_request.h"

namespace shot {

// One request in, one encoded image out.
//
// Both callers go through here -- the command line, which serves one request
// and exits, and the resident worker, which serves many. Keeping the two on one
// path is the point: a difference between "what the CLI does" and "what the
// worker does" would be invisible until someone compared two pictures.
//
// Requires a live ShotRuntime on this thread.
base::expected<std::vector<uint8_t>, std::string> Capture(
    const ScreenshotRequest& request);

}  // namespace shot

#endif  // SHOT_SHOT_CAPTURE_H_
