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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_VIDEO_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_VIDEO_ELEMENT_H_

#include "cc/paint/paint_flags.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_image_source.h"
#include "third_party/blink/renderer/core/html/html_image_loader.h"
#include "third_party/blink/renderer/core/html/media/html_media_element.h"

namespace cc {
class PaintCanvas;
}

namespace gfx {
class Rect;
class Size;
}

namespace blink {

class StaticBitmapImage;

class CORE_EXPORT HTMLVideoElement final : public HTMLMediaElement,
                                           public CanvasImageSource {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit HTMLVideoElement(Document&);
  void Trace(Visitor*) const override;

  ElementType GetElementType() const final {
    return ElementType::kHTMLVideoElement;
  }

  bool HasPendingActivity() const final;

  // Node override.
  Node::InsertionNotificationRequest InsertedInto(ContainerNode&) override;
  void RemovedFrom(ContainerNode&) override;

  // Always 0: there is no player to report a decoded frame's natural size.
  unsigned videoWidth() const { return 0; }
  unsigned videoHeight() const { return 0; }

  gfx::Size videoVisibleSize() const;

  // Statistics -- always 0, since nothing is ever decoded.
  unsigned webkitDecodedFrameCount() const { return 0; }
  unsigned webkitDroppedFrameCount() const { return 0; }

  // Used by canvas to gain raw pixel access. There is no current frame, so
  // this is a no-op; kept because LocalFrame's context-menu "copy video
  // frame" action and VideoPainter's software-paint fallback both still call
  // it unconditionally.
  void PaintCurrentFrame(cc::PaintCanvas*,
                         const gfx::Rect&,
                         const cc::PaintFlags&,
                         bool acquire_texture_backing) const {}

  bool HasAvailableVideoFrame() const { return false; }
  bool HasReadableVideoFrame() const { return false; }

  KURL PosterImageURL() const override;

  // Returns whether the current poster image URL is the default for the
  // document.
  // TODO(1190335): Remove this once default poster image URL is removed.
  bool IsDefaultPosterImageURL() const;

  // Always returns nullptr: converting a decoded media::VideoFrame into a
  // StaticBitmapImage needed media::PaintCanvasVideoRenderer, which lived in
  // //media/renderers and was cut with the rest of the decode pipeline.
  // There is never a current frame to convert.
  scoped_refptr<StaticBitmapImage> CreateStaticBitmapImage(
      std::optional<gfx::Size> size = std::nullopt,
      bool reinterpret_as_srgb = false,
      RespectImageOrientationEnum respect_orientation =
          kRespectImageOrientation) {
    return nullptr;
  }

  // CanvasImageSource implementation
  scoped_refptr<Image> GetSourceImageForCanvas(SourceImageStatus*,
                                               const gfx::SizeF&) override;
  bool IsVideoElement() const override { return true; }
  bool WouldTaintOrigin() const override { return !IsMediaDataCorsSameOrigin(); }
  gfx::SizeF ElementSize(const gfx::SizeF&,
                         const RespectImageOrientationEnum) const override;
  const KURL& SourceURL() const override { return currentSrc(); }
  bool IsHTMLVideoElement() const override { return true; }
  // Video elements currently always go through RAM when used as a canvas image
  // source.
  bool IsAccelerated() const override { return false; }

  void StyleDidChange(const ComputedStyle* old_style,
                      const ComputedStyle& new_style);

  // The :video-persistent pseudo-class was for videos with custom controls in
  // Android auto-picture-in-picture. Picture-in-Picture no longer exists
  // (HTMLMediaElement::SupportsPictureInPicture() is unconditionally false),
  // so this is permanently false rather than removed outright --
  // selector_checker.cc still evaluates the pseudo and expects an answer.
  bool IsPersistent() const { return false; }

  bool IsRichlyEditableForAccessibility() const override { return false; }

  void RequestSaveVideoFrame();

 protected:
  // EventTarget overrides.
  void AddedEventListener(const AtomicString& event_type,
                          RegisteredEventListener&) override;

 private:
  bool LayoutObjectIsNeeded(const DisplayStyle&) const override;
  LayoutObject* CreateLayoutObject(const ComputedStyle&) override;
  void AttachLayoutTree(AttachContext&) override;
  void UpdatePosterImage();
  void ParseAttribute(const AttributeModificationParams&) override;
  bool IsPresentationAttribute(const QualifiedName&) const override;
  void CollectStyleForPresentationAttribute(
      const QualifiedName&,
      const AtomicString&,
      HeapVector<CSSPropertyValue, 8>&) override;
  bool IsURLAttribute(const Attribute&) const override;
  const AtomicString ImageSourceURL() const override;

  void DidMoveToNewDocument(Document& old_document) override;
  void DidChangeIsInCanvasSubtree() override;

  Member<HTMLImageLoader> image_loader_;

  AtomicString default_poster_url_;

  // Paint flags set based on CSS properties. There is no cc::Layer to
  // propagate them to any more (no player, no video frames), but StyleDidChange
  // is still called by LayoutVideo, so these are kept for that codepath's sake.
  cc::PaintFlags::FilterQuality filter_quality_ =
      cc::PaintFlags::FilterQuality::kLow;
  cc::PaintFlags::DynamicRangeLimitMixture dynamic_range_limit_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_VIDEO_ELEMENT_H_
