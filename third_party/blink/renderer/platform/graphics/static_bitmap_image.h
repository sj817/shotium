// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_STATIC_BITMAP_IMAGE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_STATIC_BITMAP_IMAGE_H_

#include "base/byte_size.h"
#include "base/notreached.h"
#include "components/viz/common/resources/shared_image_format_utils.h"
#include "third_party/blink/renderer/platform/graphics/image.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "ui/gfx/hdr_metadata.h"

namespace blink {

class PLATFORM_EXPORT StaticBitmapImage : public Image {
 public:
  // The ImageOrientation should be derived from the source of the image data.
  static scoped_refptr<StaticBitmapImage> Create(
      PaintImage,
      ImageOrientation = ImageOrientationEnum::kDefault);
  static scoped_refptr<StaticBitmapImage> Create(
      sk_sp<SkData> data,
      const SkImageInfo&,
      const gfx::HDRMetadata&,
      ImageOrientation = ImageOrientationEnum::kDefault);

  StaticBitmapImage(ImageOrientation orientation) : orientation_(orientation) {}

  bool IsStaticBitmapImage() const override { return true; }

  // Methods overridden by all sub-classes
  ~StaticBitmapImage() override = default;

  gfx::Size SizeWithConfig(SizeConfig) const final;

  // Methods have common implementation for all sub-classes
  bool FirstFrameIsComplete() override { return true; }
  void DestroyDecodedData() override {}

  // Methods that have a default implementation, and overridden by only one
  // sub-class
  virtual bool IsValid() const { return true; }
  virtual void Transfer() {}

  // Creates a non-gpu copy of the image, or returns this if image is already
  // non-gpu.
  virtual scoped_refptr<StaticBitmapImage> MakeUnaccelerated() { return this; }

  bool IsPremultiplied() const {
    return GetAlphaType() == SkAlphaType::kPremul_SkAlphaType;
  }

  // Methods have exactly the same implementation for all sub-classes
  bool OriginClean() const { return is_origin_clean_; }
  void SetOriginClean(bool flag) { is_origin_clean_ = flag; }

  // StaticBitmapImage needs to store the orientation of the image itself,
  // because the underlying representations do not. If the bitmap represents
  // a non-default orientation it must be explicitly given in the constructor.
  ImageOrientation Orientation() const override { return orientation_; }

  void SetOrientation(ImageOrientation orientation) {
    orientation_ = orientation;
  }

  // This function results in a readback due to using SkImage::readPixels().
  // Returns transparent black pixels if the input SkImageInfo.bounds() does
  // not intersect with the input image boundaries. When `apply_orientation`
  // is true this method will orient the data according to the source's EXIF
  // information.
  Vector<uint8_t> CopyImageData(const SkImageInfo& info,
                                bool apply_orientation);

  virtual gfx::Size GetSize() const = 0;
  virtual SkAlphaType GetAlphaType() const = 0;
  virtual gfx::ColorSpace GetColorSpace() const = 0;
  virtual viz::SharedImageFormat GetSharedImageFormat() const = 0;
  virtual const gfx::HDRMetadata& GetHdrMetadata() const = 0;

  base::ByteSize EstimatedSizeInBytes() const {
    return base::ByteSize(
        GetSharedImageFormat().EstimatedSizeInBytes(GetSize()));
  }

 protected:
  // Helper for sub-classes
  void DrawHelper(cc::PaintCanvas*,
                  const cc::PaintFlags&,
                  const gfx::RectF&,
                  const gfx::RectF&,
                  const ImageDrawOptions&,
                  const PaintImage&);

  // The image orientation is stored here because it is only available when the
  // static image is created and the underlying representations do not store
  // the information. The property is set at construction based on the source of
  // the image data.
  ImageOrientation orientation_ = ImageOrientationEnum::kDefault;

  // SkImage does not carry the web resource security origin.
  bool is_origin_clean_ = true;
};

template <>
struct DowncastTraits<StaticBitmapImage> {
  static bool AllowFrom(const Image& image) {
    return image.IsStaticBitmapImage();
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_STATIC_BITMAP_IMAGE_H_
