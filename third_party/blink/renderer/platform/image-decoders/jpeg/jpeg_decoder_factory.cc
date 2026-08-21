// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/image-decoders/jpeg/jpeg_decoder_factory.h"

#include "third_party/blink/renderer/platform/image-decoders/jpeg/jpeg_image_decoder.h"

namespace blink {

std::unique_ptr<ImageDecoder> CreateJpegImageDecoder(
    ImageDecoder::AlphaOption alpha_option,
    ColorBehavior color_behavior,
    cc::AuxImage aux_image,
    wtf_size_t max_decoded_bytes) {
  // Upstream returns a JpegRustImageDecoder here when
  // skia::IsRustyJpegEnabled() and the request has no decode budget. That
  // feature ships disabled, so JPEGImageDecoder was always what ran.
  return std::make_unique<JPEGImageDecoder>(alpha_option, color_behavior,
                                            aux_image, max_decoded_bytes);
}

}  // namespace blink
