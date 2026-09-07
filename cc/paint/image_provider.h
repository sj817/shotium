// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_IMAGE_PROVIDER_H_
#define CC_PAINT_IMAGE_PROVIDER_H_

#include <utility>

#include "base/functional/callback.h"
#include "cc/paint/decoded_draw_image.h"
#include "cc/paint/draw_image.h"
#include "cc/paint/paint_export.h"

namespace cc {
class PaintImage;

// Used to replace lazy generated PaintImages with decoded images for
// rasterization.
class CC_PAINT_EXPORT ImageProvider {
 public:
  class CC_PAINT_EXPORT ScopedResult {
   public:
    using DestructionCallback = base::OnceClosure;

    ScopedResult();
    explicit ScopedResult(DecodedDrawImage image);
    ScopedResult(DecodedDrawImage image, DestructionCallback callback);
    ScopedResult(const ScopedResult&) = delete;
    ScopedResult(ScopedResult&& other);
    ~ScopedResult();

    ScopedResult& operator=(const ScopedResult&) = delete;
    ScopedResult& operator=(ScopedResult&& other);

    explicit operator bool() const { return !!image_; }
    const DecodedDrawImage& decoded_image() const { return image_; }
    bool needs_unlock() const { return !destruction_callback_.is_null(); }

   private:
    void DestroyDecode();

    DecodedDrawImage image_;
    DestructionCallback destruction_callback_;
  };

  virtual ~ImageProvider() = default;

  // Returns the decoded image to use during rasterization. If no image is
  // provided, the draw is skipped.
  virtual ScopedResult GetRasterContent(const DrawImage& draw_image) = 0;
};

}  // namespace cc

#endif  // CC_PAINT_IMAGE_PROVIDER_H_
