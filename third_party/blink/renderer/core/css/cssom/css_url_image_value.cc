// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/cssom/css_url_image_value.h"

#include "third_party/blink/renderer/core/css/css_image_value.h"
#include "third_party/blink/renderer/core/loader/resource/image_resource_content.h"
#include "third_party/blink/renderer/core/style/style_image.h"

namespace blink {

const String& CSSURLImageValue::url() const {
  return value_->RelativeUrl();
}

std::optional<gfx::Size> CSSURLImageValue::IntrinsicSize() const {
  if (Status() != ResourceStatus::kCached) {
    return std::nullopt;
  }

  DCHECK(!value_->IsCachePending());
  ImageResourceContent* resource_content = value_->CachedImage()->CachedImage();

  return resource_content
             ? resource_content->IntrinsicSize(kRespectImageOrientation)
             : gfx::Size(0, 0);
}

ResourceStatus CSSURLImageValue::Status() const {
  if (value_->IsCachePending()) {
    return ResourceStatus::kNotStarted;
  }
  return value_->CachedImage()->CachedImage()->GetContentStatus();
}

const CSSValue* CSSURLImageValue::ToCSSValue() const {
  return value_.Get();
}

void CSSURLImageValue::Trace(Visitor* visitor) const {
  visitor->Trace(value_);
  CSSStyleImageValue::Trace(visitor);
}

}  // namespace blink
