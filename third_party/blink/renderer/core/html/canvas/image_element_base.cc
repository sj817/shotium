// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/canvas/image_element_base.h"

#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/keywords.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/loader/image_loader.h"
#include "third_party/blink/renderer/core/svg/graphics/svg_image_for_container.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"

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

const Element& ImageElementBase::GetElement() const {
  return *GetImageLoader().GetElement();
}

mojom::blink::PreferredColorScheme ImageElementBase::PreferredColorScheme()
    const {
  const Element& element = GetElement();
  const ComputedStyle* style = element.GetComputedStyle();
  return element.GetDocument().GetStyleEngine().ResolveColorSchemeForEmbedding(
      style);
}

bool ImageElementBase::IsImageElement() const {
  return CachedImage() && !IsA<SVGImage>(CachedImage()->GetImage());
}

scoped_refptr<Image> ImageElementBase::GetSourceImageForCanvas(
    SourceImageStatus* status,
    const gfx::SizeF& default_object_size) {
  ImageResourceContent* image_content = CachedImage();
  if (!GetImageLoader().ImageComplete() || !image_content) {
    *status = kIncompleteSourceImageStatus;
    return nullptr;
  }

  if (image_content->ErrorOccurred()) {
    *status = kUndecodableSourceImageStatus;
    return nullptr;
  }

  scoped_refptr<Image> source_image = image_content->GetImage();

  if (auto* svg_image = DynamicTo<SVGImage>(source_image.get())) {
    UseCounter::Count(GetElement().GetDocument(), WebFeature::kSVGInCanvas2D);
    if (svg_image->HasSVGForeignObject()) {
      UseCounter::Count(GetElement().GetDocument(),
                        WebFeature::kSVGForeignObjectDrawnIntoCanvas);
    }
    const SVGImageViewInfo* view_info =
        SVGImageForContainer::CreateViewInfo(*svg_image, GetElement());
    const gfx::SizeF image_size = SVGImageForContainer::ConcreteObjectSize(
        *svg_image, view_info, default_object_size);
    if (image_size.IsEmpty()) {
      *status = kZeroSizeImageSourceStatus;
      return nullptr;
    }
    source_image = SVGImageForContainer::Create(
        *svg_image, image_size, 1, view_info, PreferredColorScheme());
  }

  if (source_image->Size().IsEmpty()) {
    *status = kZeroSizeImageSourceStatus;
    return nullptr;
  }

  *status = kNormalSourceImageStatus;
  return source_image->ImageForDefaultFrame();
}

bool ImageElementBase::WouldTaintOrigin() const {
  return CachedImage() && !CachedImage()->IsCorsSameOrigin();
}

gfx::SizeF ImageElementBase::ElementSize(
    const gfx::SizeF& default_object_size,
    const RespectImageOrientationEnum respect_orientation) const {
  ImageResourceContent* image_content = CachedImage();
  if (!image_content || !image_content->HasImage())
    return gfx::SizeF();
  Image* image = image_content->GetImage();
  if (auto* svg_image = DynamicTo<SVGImage>(image)) {
    const SVGImageViewInfo* view_info =
        SVGImageForContainer::CreateViewInfo(*svg_image, GetElement());
    return SVGImageForContainer::ConcreteObjectSize(*svg_image, view_info,
                                                    default_object_size);
  }
  return gfx::SizeF(image->Size(respect_orientation));
}

gfx::SizeF ImageElementBase::DefaultDestinationSize(
    const gfx::SizeF& default_object_size,
    const RespectImageOrientationEnum respect_orientation) const {
  return ElementSize(default_object_size, respect_orientation);
}

bool ImageElementBase::IsAccelerated() const {
  return false;
}

const KURL& ImageElementBase::SourceURL() const {
  return CachedImage()->GetResponse().CurrentRequestUrl();
}

bool ImageElementBase::IsOpaque() const {
  ImageResourceContent* image_content = CachedImage();
  if (!GetImageLoader().ImageComplete() || !image_content)
    return false;
  Image* image = image_content->GetImage();
  return image->IsOpaque();
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
