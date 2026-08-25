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
// Every caller goes through here -- the command line, which serves one request
// and exits, the resident worker, which serves many, and the shared library,
// which serves a host process. Keeping them on one path is the point: a
// difference between "what the CLI does" and "what the worker does" would be
// invisible until someone compared two pictures.
//
// Requires a live ShotRuntime on this thread.
base::expected<std::vector<uint8_t>, std::string> Capture(
    const ScreenshotRequest& request);

// What one answered request amounts to.
struct CaptureResult {
  // The encoded image, or empty when the request named a `path`: the bytes
  // went there instead and there is nothing left to hand back.
  std::vector<uint8_t> image;
  // How many bytes the image is either way, so that a caller reporting the
  // size does not have to know which of the two happened.
  size_t size = 0;
  bool wrote_path = false;
};

// Capture(), and then the one thing every caller would otherwise do for
// itself: honouring `path` by writing the file here rather than shipping the
// bytes back.
//
// It is worth a whole round trip to the worker's supervisor when the caller
// only wanted the image on disk, and worth nothing at all to a caller in this
// process -- but it is on the wire, so all three answer it, and answering it
// three times is how they would come to answer it differently.
//
// Requires a live ShotRuntime on this thread.
base::expected<CaptureResult, std::string> CaptureAndDeliver(
    const ScreenshotRequest& request);

}  // namespace shot

#endif  // SHOT_SHOT_CAPTURE_H_
