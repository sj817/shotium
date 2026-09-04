// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_CAPTURE_H_
#define SHOT_SHOT_CAPTURE_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/types/expected.h"
#include "shot/shot_capture_context.h"
#include "shot/shot_bytes.h"
#include "shot/shot_request.h"
#include "ui/gfx/geometry/rect.h"

namespace shot {

class ShotRuntime;

// One request in, one encoded image out.
//
// Every caller goes through here -- the command line, which serves one request
// and exits, the resident worker, which serves many, and the shared library,
// which serves a host process. Keeping them on one path is the point: a
// difference between "what the CLI does" and "what the worker does" would be
// invisible until someone compared two pictures.
//
// `out_stats` is optional and costs nothing to omit: the counters are
// collected either way, by whatever is already touching each resource, and
// this only decides whether the total is handed back. The command line has no
// use for it.
//
// What one answered request amounts to.
struct CaptureResult {
  // The encoded image, or empty when the request named a `path`: the engine
  // streamed the bytes into that file as it encoded them, and there is
  // nothing left to hand back.
  Bytes image;
  // How many bytes the image is either way, so that a caller reporting the
  // size does not have to know which of the two happened.
  size_t size = 0;
  bool wrote_path = false;
};

// Requires a live ShotRuntime on this thread.
base::expected<CaptureResult, std::string> Capture(
    ShotRuntime& runtime,
    const ScreenshotRequest& request,
    CaptureStats* out_stats = nullptr);

// Capture(), and then the one thing every caller would otherwise do for
// itself: honouring `path` by writing the file here rather than shipping the
// bytes back.
//
// It is worth a whole round trip to the worker's supervisor when the caller
// only wanted the image on disk, and worth nothing at all to a caller in this
// process -- but it is on the wire, so all three answer it, and answering it
// three times is how they would come to answer it differently.
//
// `out_stats` is filled in on the way out whether or not the capture
// succeeded, which is the case it is most worth having: a request that timed
// out after fetching forty subresources has already said why in its counters,
// and the error string alone cannot.
//
// Requires a live ShotRuntime on this thread.
base::expected<CaptureResult, std::string> CaptureAndDeliver(
    ShotRuntime& runtime,
    const ScreenshotRequest& request,
    CaptureStats* out_stats = nullptr);

// One tile of a tiles capture, as handed to a caller.
struct DeliveredTile {
  // Where it came from: CSS pixels, document coordinates.
  gfx::Rect region;
  // The encoded image, or empty when it went to `path`.
  Bytes image;
  size_t size = 0;
  // Where it was written, when the request named a `path`.
  std::string path;
};

// A request with `tile` set: the region in slices of at most `tile.height`
// CSS pixels, each encoded on its own, in document order. One load and layout
// serve all of them.
//
// Only the bitmap workspace is one tile at a time. Without `path`, the return
// value owns every completed encoded image, so those bytes accumulate until
// the caller destroys them. With `path`, image-data memory stays bounded by
// the tile being encoded; the returned vector contains metadata only.
//
// Honours `path` the way CaptureAndDeliver does, with one rule of its own:
// the path has to contain `{n}`, which becomes the tile's 1-based index, so
// that `page-{n}.png` writes `page-1.png`, `page-2.png` and so on. A path
// without it would have every tile overwrite the last.
//
// Requires a live ShotRuntime on this thread.
base::expected<std::vector<DeliveredTile>, std::string> CaptureTiles(
    ShotRuntime& runtime,
    const ScreenshotRequest& request,
    CaptureStats* out_stats = nullptr);

}  // namespace shot

#endif  // SHOT_SHOT_CAPTURE_H_
