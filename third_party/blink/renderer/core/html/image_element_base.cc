// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/image_element_base.h"

#include "third_party/blink/renderer/core/keywords.h"
#include "third_party/blink/renderer/core/loader/image_loader.h"

namespace blink {

// static
Image::ImageDecodingMode ImageElementBase::ParseImageDecodingMode(
    const AtomicString& async_attr_value) {
  if (async_attr_value.IsNull())
    return Image::kUnspecifiedDecode;

  const auto& value = async_attr_value.ToAsciiLower();
  if (value == keywords::kAsync)
    return Image::kAsyncDecode;
  if (value == keywords::kSync)
    return Image::kSyncDecode;
  return Image::kUnspecifiedDecode;
}

ImageResourceContent* ImageElementBase::CachedImage() const {
  return GetImageLoader().GetContent();
}

Image::ImageDecodingMode ImageElementBase::GetDecodingModeForPainting(
    PaintImage::Id new_id) {
  const bool content_transitioned =
      last_painted_image_id_ != PaintImage::kInvalidId &&
      new_id != PaintImage::kInvalidId && last_painted_image_id_ != new_id;
  last_painted_image_id_ = new_id;

  // If the image for the element was transitioned, and no preference has been
  // specified by the author, prefer sync decoding to avoid flickering the
  // element. Async decoding of this image would cause us to display
  // intermediate frames with no image while the decode is in progress which
  // creates a visual flicker in the transition.
  if (content_transitioned &&
      decoding_mode_ == Image::ImageDecodingMode::kUnspecifiedDecode)
    return Image::ImageDecodingMode::kSyncDecode;
  return decoding_mode_;
}

}  // namespace blink
