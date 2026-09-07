// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_IMAGE_ELEMENT_BASE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_IMAGE_ELEMENT_BASE_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/graphics/image.h"

namespace blink {

class ImageLoader;
class ImageResourceContent;

class CORE_EXPORT ImageElementBase {
 public:
  virtual ImageLoader& GetImageLoader() const = 0;

  // Parses the given async parameter value into an ImageDecodingMode. This is
  // used by SVGImageElement and HTMLImageElement since this class is a common
  // base for both elements.
  static Image::ImageDecodingMode ParseImageDecodingMode(const AtomicString&);

  ImageResourceContent* CachedImage() const;

  // Returns the decoding mode that should be used when painting this element,
  // given the PaintImage::Id that will be used to paint it.
  // Used with HTMLImageElement and SVGImageElement types.
  Image::ImageDecodingMode GetDecodingModeForPainting(PaintImage::Id);

 protected:
  virtual ~ImageElementBase() = default;

  Image::ImageDecodingMode decoding_mode_ =
      Image::ImageDecodingMode::kUnspecifiedDecode;

 private:
  // The id for the PaintImage used the last time this element was painted.
  PaintImage::Id last_painted_image_id_ = PaintImage::kInvalidId;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_IMAGE_ELEMENT_BASE_H_
