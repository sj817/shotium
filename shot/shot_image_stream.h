// Copyright 2026 The Shot Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_IMAGE_STREAM_H_
#define SHOT_SHOT_IMAGE_STREAM_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/files/file.h"
#include "base/memory/scoped_refptr.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "cc/paint/paint_image.h"
#include "shot/shot_bytes.h"
#include "third_party/skia/include/core/SkSurfaceProps.h"
#include "ui/gfx/geometry/rect.h"

namespace cc {
class DisplayItemList;
}

namespace shot {

struct ScreenshotRequest;

// What one image cost, for the capture's statistics and for tuning.
struct ImageStreamStats {
  // Wall time spent producing strips (waiting on the raster threads counts).
  base::TimeDelta raster;
  // Wall time spent inside the encoder.
  base::TimeDelta encode;
  // Time spent decoding images, summed over threads.
  base::TimeDelta decode;
  // The encoded image's size after Finish(), whether it was handed back or
  // streamed into a file.
  size_t encoded_bytes = 0;
  int strips = 0;
  int threads = 0;
  // The most bitmap memory held at once: committed rows plus decoded images.
  size_t peak_window_bytes = 0;
  size_t peak_decoded_bytes = 0;
};

// One output image, rastered in horizontal strips from the top down and
// encoded as each strip completes.
//
// The image is never in memory whole. Its pixels live in a reserved address
// range of which only the rows between the raster threads and the encoder are
// committed; a row is handed to the OS the moment the encoder is done with it.
// Images the document draws are decoded when a strip first needs them, at the
// scale they are drawn at, and dropped once the last strip that draws them has
// been encoded. So a 1440 x 40000 page costs a few strips of bitmap and the
// images currently in view of those strips, not the 236 MB of the image.
class ImageStream {
 public:
  // `opaque` promises that every pixel will end up opaque, which lets PNG and
  // WebP drop the alpha channel before a single row exists.
  static base::expected<std::unique_ptr<ImageStream>, std::string> Create(
      int width,
      int height,
      const ScreenshotRequest& request,
      bool opaque,
      const SkSurfaceProps& props,
      base::File output);

  ImageStream(const ImageStream&) = delete;
  ImageStream& operator=(const ImageStream&) = delete;
  ~ImageStream();

  // Rasters image rows [device_top, device_top + rows) from `list`, whose ops
  // are in the painted document's CSS pixels: pixel (0, 0) of the image is
  // `cull_rect.origin()` at `scale`. Slices are added top to bottom and must
  // tile the image exactly.
  //
  // An image's compressed bytes are released the moment the last strip that
  // draws it is encoded, except for the images in `keep_encoded`: those are
  // drawn again by a later slice (of this stream or another) and must stay
  // decodable.
  base::expected<void, std::string> AddSlice(
      scoped_refptr<const cc::DisplayItemList> list,
      const gfx::Rect& cull_rect,
      double scale,
      int device_top,
      int rows,
      const base::flat_set<cc::PaintImage::Id>& keep_encoded);

  // Finishes the encoding and returns the file. Every row must have been
  // added.
  base::expected<Bytes, std::string> Finish();

  const ImageStreamStats& stats() const { return stats_; }

 private:
  class Impl;
  explicit ImageStream(std::unique_ptr<Impl> impl);

  std::unique_ptr<Impl> impl_;
  ImageStreamStats stats_;
};

}  // namespace shot

#endif  // SHOT_SHOT_IMAGE_STREAM_H_
