/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2006, 2007, 2008, 2009, 2012 Apple Inc. All rights
 * reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_HTML_PLUGIN_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_HTML_PLUGIN_ELEMENT_H_

#include "services/network/public/cpp/permissions_policy/permissions_policy_declaration.h"
#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/create_element_flags.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"

namespace blink {

class HTMLImageLoader;
class LayoutEmbeddedContent;
class LayoutEmbeddedObject;

class CORE_EXPORT HTMLPlugInElement
    : public HTMLFrameOwnerElement,
      public ActiveScriptWrappable<HTMLPlugInElement> {
 public:
  ~HTMLPlugInElement() override;
  void Trace(Visitor*) const override;

  bool IsPlugin() const final { return true; }

  bool HasPendingActivity() const final;

  const String& Url() const { return url_; }

  // Public for FrameView::addPartToUpdate()
  bool NeedsPluginUpdate() const { return needs_plugin_update_; }
  void SetNeedsPluginUpdate(bool needs_plugin_update) {
    needs_plugin_update_ = needs_plugin_update;
  }
  void UpdatePlugin();


  network::ParsedPermissionsPolicy ConstructContainerPolicy() const override;

  bool IsImageType() const;
  HTMLImageLoader* ImageLoader() const { return image_loader_.Get(); }
  virtual bool UseFallbackContent() const;

 protected:
  HTMLPlugInElement(const QualifiedName& tag_name,
                    Document&,
                    const CreateElementFlags);

  // Node functions:
  InsertionNotificationRequest InsertedInto(
      ContainerNode& insertion_point) override;
  void DidMoveToNewDocument(Document& old_document) override;
  void AttachLayoutTree(AttachContext&) override;

  // Element functions:
  bool IsPresentationAttribute(const QualifiedName&) const override;
  void CollectStyleForPresentationAttribute(
      const QualifiedName&,
      const AtomicString&,
      HeapVector<CSSPropertyValue, 8>&) override;
  // HTMLFrameOwnerElement overrides:
  void NaturalSizingInfoChanged() final;

  virtual bool HasFallbackContent() const;

  LayoutEmbeddedObject* GetLayoutEmbeddedObject() const;
  bool AllowedToLoadFrameURL(const String& url);
  bool RequestObject();

  void DispatchErrorEvent();
  void ReattachOnPluginChangeIfNeeded(bool require_layout);

  void SetUrl(const StringView& url) {
    url_ = url.ToString();
    UpdateServiceTypeIfEmpty();
  }

  void SetServiceType(const String& service_type) {
    service_type_ = service_type;
    UpdateServiceTypeIfEmpty();
  }

  // Set when the current view cannot be re-used on reattach. This is the case
  // e.g. when attributes (e.g. src) change.
  void SetDisposeView() { dispose_view_ = true; }

  String service_type_;
  String url_;
  Member<HTMLImageLoader> image_loader_;
  bool is_delaying_load_event_;

 private:
  // Node overrides:
  bool CanContainRangeEndPoint() const override { return false; }
  bool CanStartSelection() const override;
  bool WillRespondToMouseClickEvents() final;
  void DefaultEventHandler(Event&) final;
  void DetachLayoutTree(bool performing_reattach) final;
  void FinishParsingChildren() final;

  // Element overrides:
  LayoutObject* CreateLayoutObject(const ComputedStyle&) override;
  FocusableState SupportsFocus(UpdateBehavior) const final {
    return FocusableState::kFocusable;
  }
  bool IsFocusableStyle(UpdateBehavior update_behavior =
                            UpdateBehavior::kStyleAndLayout) const final;
  void DidAddUserAgentShadowRoot(ShadowRoot&) final;
  const ComputedStyle* CustomStyleForLayoutObject(
      const StyleRecalcContext&) final;

  // HTMLElement overrides:
  bool HasCustomFocusLogic() const override;
  bool IsPluginElement() const final;

  virtual void UpdatePluginInternal() = 0;

  // Perform checks based on the URL and MIME-type of the object to load.
  bool AllowedToLoadObject(const KURL&, const String& mime_type);

  enum class ObjectContentType {
    kNone,
    kImage,
    kFrame,
  };
  ObjectContentType GetObjectContentType() const;


  void UpdateServiceTypeIfEmpty();

  bool needs_plugin_update_;
  // Represents |layoutObject() && layoutObject()->isEmbeddedObject() &&
  // !layoutEmbeddedItem().showsUnavailablePluginIndicator()|.  We want to
  // avoid accessing |layoutObject()| in layoutObjectIsFocusable().
  bool embedded_content_is_available_ = false;

  // True when the element has changed in such a way (new URL, for instance)
  // that we cannot re-use the old view when re-attaching.
  bool dispose_view_ = false;
};

template <>
struct DowncastTraits<HTMLPlugInElement> {
  static bool AllowFrom(const Node& node) {
    auto* html_element = DynamicTo<HTMLElement>(node);
    return html_element && AllowFrom(*html_element);
  }
  static bool AllowFrom(const HTMLElement& html_element) {
    return html_element.IsPluginElement();
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_HTML_PLUGIN_ELEMENT_H_
