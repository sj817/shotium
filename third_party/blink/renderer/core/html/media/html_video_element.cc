/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/media/html_video_element.h"

#include "cc/layers/layer.h"
#include "third_party/blink/renderer/core/css/css_property_names.h"
#include "third_party/blink/renderer/core/dom/attribute.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_idioms.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/layout/geometry/physical_rect.h"
#include "third_party/blink/renderer/core/layout/layout_image.h"
#include "third_party/blink/renderer/core/layout/layout_video.h"
#include "third_party/blink/renderer/platform/graphics/static_bitmap_image.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "ui/gfx/geometry/size.h"

namespace blink {

HTMLVideoElement::HTMLVideoElement(Document& document)
    : HTMLMediaElement(html_names::kVideoTag, document) {
  if (document.GetSettings()) {
    default_poster_url_ =
        AtomicString(document.GetSettings()->GetDefaultVideoPosterURL());
  }

  EnsureUserAgentShadowRoot();
  UpdateStateIfNeeded();
}

void HTMLVideoElement::Trace(Visitor* visitor) const {
  visitor->Trace(image_loader_);
  HTMLMediaElement::Trace(visitor);
}

bool HTMLVideoElement::HasPendingActivity() const {
  return HTMLMediaElement::HasPendingActivity() ||
         (image_loader_ && image_loader_->HasPendingActivity());
}

Node::InsertionNotificationRequest HTMLVideoElement::InsertedInto(
    ContainerNode& insertion_point) {
  return HTMLMediaElement::InsertedInto(insertion_point);
}

void HTMLVideoElement::RemovedFrom(ContainerNode& insertion_point) {
  HTMLMediaElement::RemovedFrom(insertion_point);
}

bool HTMLVideoElement::LayoutObjectIsNeeded(const DisplayStyle& style) const {
  return HTMLElement::LayoutObjectIsNeeded(style);
}

LayoutObject* HTMLVideoElement::CreateLayoutObject(const ComputedStyle& style) {
  if (auto* content_image =
          DynamicTo<ImageContentData>(style.GetContentData())) {
    if (!content_image->GetImage()->ErrorOccurred()) {
      return LayoutObject::CreateObject(this, style);
    }
  }
  return MakeGarbageCollected<LayoutVideo>(this);
}

void HTMLVideoElement::AttachLayoutTree(AttachContext& context) {
  HTMLMediaElement::AttachLayoutTree(context);
  // Initiate loading of the poster image if a default poster image is
  // specified and no poster has been loaded (=> no ImageLoader created).
  if (!default_poster_url_.empty() && !image_loader_) {
    UpdatePosterImage();
  }
  if (image_loader_ && GetLayoutObject()) {
    image_loader_->OnAttachLayoutTree();
  }
}

void HTMLVideoElement::UpdatePosterImage() {
  // The loading=lazy path that used to gate this on IntersectionObserver
  // visibility was removed: ShouldDeferMediaLoad() (core/loader/
  // lazy_media_helper.cc) always required CanExecuteScripts(), which is
  // never true in this build (there is no V8), so lazy deferral could never
  // actually trigger here. The poster always loads eagerly, which is what
  // this already amounted to in practice.
  if (!image_loader_) {
    image_loader_ = MakeGarbageCollected<HTMLImageLoader>(this);
  }
  image_loader_->UpdateFromElement();
}

void HTMLVideoElement::CollectStyleForPresentationAttribute(
    const QualifiedName& name,
    const AtomicString& value,
    HeapVector<CSSPropertyValue, 8>& style) {
  if (name == html_names::kWidthAttr) {
    AddHTMLLengthToStyle(style, CSSPropertyID::kWidth, value);
    const AtomicString& height = FastGetAttribute(html_names::kHeightAttr);
    if (height) {
      ApplyAspectRatioToStyle(value, height, style);
    }
  } else if (name == html_names::kHeightAttr) {
    AddHTMLLengthToStyle(style, CSSPropertyID::kHeight, value);
    const AtomicString& width = FastGetAttribute(html_names::kWidthAttr);
    if (width) {
      ApplyAspectRatioToStyle(width, value, style);
    }
  } else {
    HTMLMediaElement::CollectStyleForPresentationAttribute(name, value, style);
  }
}

bool HTMLVideoElement::IsPresentationAttribute(
    const QualifiedName& name) const {
  if (name == html_names::kWidthAttr || name == html_names::kHeightAttr) {
    return true;
  }
  return HTMLMediaElement::IsPresentationAttribute(name);
}

void HTMLVideoElement::ParseAttribute(
    const AttributeModificationParams& params) {
  if (params.name == html_names::kPosterAttr) {
    const KURL poster_image_url = PosterImageURL();
    // Load the poster if set, |VideoPainter| will decide whether to draw
    // it. Only create an ImageLoader if a non-empty URL is seen.
    if (image_loader_ || !poster_image_url.IsEmpty()) {
      UpdatePosterImage();
    }
  } else {
    HTMLMediaElement::ParseAttribute(params);
  }
}

bool HTMLVideoElement::IsURLAttribute(const Attribute& attribute) const {
  return attribute.GetName() == html_names::kPosterAttr ||
         HTMLMediaElement::IsURLAttribute(attribute);
}

const AtomicString HTMLVideoElement::ImageSourceURL() const {
  const AtomicString& url = FastGetAttribute(html_names::kPosterAttr);
  if (!StripLeadingAndTrailingHtmlSpaces(url).empty()) {
    return url;
  }
  return default_poster_url_;
}

KURL HTMLVideoElement::PosterImageURL() const {
  StringView url = StripLeadingAndTrailingHtmlSpaces(ImageSourceURL());
  if (url.empty()) {
    return KURL();
  }
  return GetDocument().CompleteURL(url);
}

bool HTMLVideoElement::IsDefaultPosterImageURL() const {
  return ImageSourceURL() == default_poster_url_;
}

void HTMLVideoElement::RequestSaveVideoFrame() {
  // This is the browser-initiated "Save Video Frame As" context-menu action.
  // CreateStaticBitmapImage() always returns nullptr (see the header): there
  // is no decoded frame to save, so this silently does nothing -- the same
  // thing a real browser does when a video has no current frame.
  if (!CreateStaticBitmapImage()) {
    return;
  }
}

scoped_refptr<Image> HTMLVideoElement::GetSourceImageForCanvas(
    SourceImageStatus* status,
    const gfx::SizeF&) {
  scoped_refptr<Image> snapshot = CreateStaticBitmapImage();
  if (!snapshot) {
    *status = kInvalidSourceImageStatus;
    return nullptr;
  }

  *status = kNormalSourceImageStatus;
  return snapshot;
}

gfx::SizeF HTMLVideoElement::ElementSize(
    const gfx::SizeF&,
    const RespectImageOrientationEnum) const {
  return gfx::SizeF(videoWidth(), videoHeight());
}

gfx::Size HTMLVideoElement::videoVisibleSize() const {
  return gfx::Size();
}

void HTMLVideoElement::DidMoveToNewDocument(Document& old_document) {
  if (image_loader_) {
    image_loader_->ElementDidMoveToNewDocument();
  }

  HTMLMediaElement::DidMoveToNewDocument(old_document);
  if (image_loader_ || FastHasAttribute(html_names::kPosterAttr)) {
    UpdatePosterImage();
  }
}

void HTMLVideoElement::DidChangeIsInCanvasSubtree() {
  HTMLMediaElement::DidChangeIsInCanvasSubtree();
  if (IsInCanvasSubtree()) {
    UpdateLayoutObject();
  }
}

void HTMLVideoElement::AddedEventListener(
    const AtomicString& event_type,
    RegisteredEventListener& registered_listener) {
  if (event_type == event_type_names::kEnterpictureinpicture) {
    UseCounter::Count(GetExecutionContext(),
                      WebFeature::kEnterPictureInPictureEventListener);
  } else if (event_type == event_type_names::kLeavepictureinpicture) {
    UseCounter::Count(GetExecutionContext(),
                      WebFeature::kLeavePictureInPictureEventListener);
  }

  HTMLMediaElement::AddedEventListener(event_type, registered_listener);
}

void HTMLVideoElement::StyleDidChange(const ComputedStyle* old_style,
                                      const ComputedStyle& new_style) {
  const auto new_filter_quality =
      (new_style.ImageRendering() == EImageRendering::kPixelated ||
       new_style.ImageRendering() == EImageRendering::kCrispEdges)
          ? cc::PaintFlags::FilterQuality::kNone
          : cc::PaintFlags::FilterQuality::kLow;
  const auto new_dynamic_range_limit = new_style.GetDynamicRangeLimit();
  filter_quality_ = new_filter_quality;
  dynamic_range_limit_ = new_dynamic_range_limit;
  // There is no cc::Layer to push these onto any more (no player, so
  // CcLayer() is always null); the CSS-derived values are still computed and
  // cached above so this function's contract with LayoutVideo::StyleDidChange
  // is unchanged.
}

}  // namespace blink
