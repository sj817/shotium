/**
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
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
 */

#include "third_party/blink/renderer/core/html/html_plugin_element.h"

#include <algorithm>

#include "base/feature_list.h"
#include "services/network/public/cpp/permissions_policy/permissions_policy_declaration.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/mojom/fetch/fetch_api_request.mojom-blink.h"
#include "third_party/blink/public/mojom/loader/request_context_frame_type.mojom-blink.h"
#include "third_party/blink/public/mojom/permissions_policy/policy_value.mojom-blink-forward.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/core/css/css_property_names.h"
#include "third_party/blink/renderer/core/css/style_change_reason.h"
#include "third_party/blink/renderer/core/display_lock/display_lock_utilities.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/execution_context/agent.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/html/html_image_loader.h"
#include "third_party/blink/renderer/core/html/html_slot_element.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_object.h"
#include "third_party/blink/renderer/core/layout/layout_image.h"
#include "third_party/blink/renderer/core/loader/mixed_content_checker.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_from_url.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/public/scheduling_policy.h"
#include "third_party/blink/renderer/platform/wtf/text/strcat.h"

namespace blink {

namespace {

String GetMIMETypeFromURL(const KURL& url) {
  StringView filename = url.LastPathComponent();
  wtf_size_t extension_pos = filename.rfind('.');
  if (extension_pos != kNotFound) {
    StringView extension = filename.substr(extension_pos + 1);
    return MIMETypeRegistry::GetWellKnownMIMETypeForExtension(extension);
  }
  return String();
}

String ResolveMIMEType(const String& specified_type, const KURL& url) {
  if (!specified_type.empty()) {
    return specified_type;
  }
  // Try to guess the MIME type based off the extension.
  return GetMIMETypeFromURL(url);
}

}  // anonymous namespace

HTMLPlugInElement::HTMLPlugInElement(const QualifiedName& tag_name,
                                     Document& doc,
                                     const CreateElementFlags flags)
    : HTMLFrameOwnerElement(tag_name, doc),
      ActiveScriptWrappable<HTMLPlugInElement>({}),
      is_delaying_load_event_(false),
      // needs_plugin_update_(!IsCreatedByParser) allows HTMLObjectElement to
      // delay EmbeddedContentView updates until after all children are
      // parsed. For HTMLEmbedElement this delay is unnecessary, but it is
      // simpler to make both classes share the same codepath in this class.
      needs_plugin_update_(!flags.IsCreatedByParser()) {
  SetHasCustomStyleCallbacks();
}

HTMLPlugInElement::~HTMLPlugInElement() {
  DCHECK(!is_delaying_load_event_);
}

void HTMLPlugInElement::Trace(Visitor* visitor) const {
  visitor->Trace(image_loader_);
  HTMLFrameOwnerElement::Trace(visitor);
}

bool HTMLPlugInElement::HasPendingActivity() const {
  return image_loader_ && image_loader_->HasPendingActivity();
}

bool HTMLPlugInElement::CanStartSelection() const {
  return UseFallbackContent() && Node::CanStartSelection();
}

bool HTMLPlugInElement::WillRespondToMouseClickEvents() {
  if (IsDisabledFormControl())
    return false;
  LayoutObject* r = GetLayoutObject();
  return r && (r->IsEmbeddedObject() || r->IsLayoutEmbeddedContent());
}

void HTMLPlugInElement::DidMoveToNewDocument(Document& old_document) {
  if (image_loader_)
    image_loader_->ElementDidMoveToNewDocument();
  HTMLFrameOwnerElement::DidMoveToNewDocument(old_document);
}

void HTMLPlugInElement::AttachLayoutTree(AttachContext& context) {
  HTMLFrameOwnerElement::AttachLayoutTree(context);

  LayoutObject* layout_object = GetLayoutObject();
  if (!layout_object || UseFallbackContent()) {
    return;
  }

  // This element may have been attached previously, and if we created a frame
  // back then, re-use it now. We do not want to reload the frame if we don't
  // have to, as that would cause us to lose any state changed after loading.
  // Re-using the frame also matters if we have to re-attach for printing; we
  // don't support reloading anything during printing (the frame would just show
  // up blank then).
  const Frame* content_frame = ContentFrame();
  if (content_frame && !dispose_view_) {
    // We should only re-use the frame if we're actually re-attaching as
    // LayoutEmbeddedContent. We may for instance have become an image, without
    // having triggered a plugin reload, and that this layout object type change
    // happens now "for free" for completely different reasons (e.g. CSS display
    // type change).
    if (layout_object->IsLayoutEmbeddedContent())
      SetEmbeddedContentView(content_frame->View());
  } else if (!IsImageType() && NeedsPluginUpdate() &&
             GetLayoutEmbeddedObject() &&
             !GetLayoutEmbeddedObject()->ShowsUnavailablePluginIndicator() &&
             !is_delaying_load_event_) {
    // If we're in a content-visibility subtree that can prevent layout, then
    // add our layout object to the frame view's update list. This is typically
    // done during layout, but if we're blocking layout, we will never update
    // the plugin and thus delay the load event indefinitely.
    if (DisplayLockUtilities::LockedAncestorPreventingLayout(*this)) {
      auto* embedded_object = GetLayoutEmbeddedObject();
      if (auto* frame_view = embedded_object->GetFrameView())
        frame_view->AddPartToUpdate(*embedded_object);
    }
    is_delaying_load_event_ = true;
    GetDocument().IncrementLoadEventDelayCount();
    GetDocument().LoadPluginsSoon();
  }
  if (image_loader_ && IsA<LayoutImage>(*layout_object)) {
    image_loader_->OnAttachLayoutTree();
  }
  if (layout_object->AffectsWhitespaceSiblings())
    context.previous_in_flow = layout_object;

  dispose_view_ = false;
}

void HTMLPlugInElement::NaturalSizingInfoChanged() {
  HTMLFrameOwnerElement::NaturalSizingInfoChanged();
  if (auto* embedded_object = GetLayoutEmbeddedObject())
    embedded_object->NaturalSizeChanged();
}

void HTMLPlugInElement::UpdatePlugin() {
  UpdatePluginInternal();
  if (is_delaying_load_event_) {
    is_delaying_load_event_ = false;
    GetDocument().DecrementLoadEventDelayCount();
  }
}

Node::InsertionNotificationRequest HTMLPlugInElement::InsertedInto(
    ContainerNode& insertion_point) {
  if (insertion_point.isConnected())
    GetDocument().DelayLoadEventUntilLayoutTreeUpdate();
  return HTMLFrameOwnerElement::InsertedInto(insertion_point);
}

network::ParsedPermissionsPolicy HTMLPlugInElement::ConstructContainerPolicy()
    const {
  return GetLegacyFramePolicies();
}

void HTMLPlugInElement::DetachLayoutTree(bool performing_reattach) {
  // Update the EmbeddedContentView the next time we attach (detaching destroys
  // the plugin).
  // FIXME: None of this "needsPluginUpdate" related code looks right.
  if (GetLayoutObject() && !UseFallbackContent())
    SetNeedsPluginUpdate(true);

  if (is_delaying_load_event_) {
    is_delaying_load_event_ = false;
    GetDocument().DecrementLoadEventDelayCount();
  }

  SetEmbeddedContentView(nullptr);

  // We should attempt to use the same view afterwards, so that we don't lose
  // state. But only if we're reattaching. Otherwise we need to throw it away,
  // since there's no telling what's going to happen next, and it wouldn't be
  // safe to keep it.
  if (!performing_reattach)
    SetDisposeView();

  HTMLFrameOwnerElement::DetachLayoutTree(performing_reattach);
}

LayoutObject* HTMLPlugInElement::CreateLayoutObject(
    const ComputedStyle& style) {
  // Fallback content breaks the DOM->layoutObject class relationship of this
  // class and all superclasses because createObject won't necessarily return
  // a LayoutEmbeddedObject or LayoutEmbeddedContent.
  if (UseFallbackContent())
    return LayoutObject::CreateObject(this, style);

  if (IsImageType()) {
    LayoutImage* image = MakeGarbageCollected<LayoutImage>(this);
    image->SetImageResource(MakeGarbageCollected<LayoutImageResource>());
    return image;
  }

  embedded_content_is_available_ = true;
  return MakeGarbageCollected<LayoutEmbeddedObject>(this);
}

void HTMLPlugInElement::FinishParsingChildren() {
  HTMLFrameOwnerElement::FinishParsingChildren();
  if (!UseFallbackContent())
    SetNeedsPluginUpdate(true);
}

bool HTMLPlugInElement::IsPresentationAttribute(
    const QualifiedName& name) const {
  if (name == html_names::kWidthAttr || name == html_names::kHeightAttr ||
      name == html_names::kVspaceAttr || name == html_names::kHspaceAttr ||
      name == html_names::kAlignAttr)
    return true;
  return HTMLFrameOwnerElement::IsPresentationAttribute(name);
}

void HTMLPlugInElement::CollectStyleForPresentationAttribute(
    const QualifiedName& name,
    const AtomicString& value,
    HeapVector<CSSPropertyValue, 8>& style) {
  if (name == html_names::kWidthAttr) {
    AddHTMLLengthToStyle(style, CSSPropertyID::kWidth, value);
  } else if (name == html_names::kHeightAttr) {
    AddHTMLLengthToStyle(style, CSSPropertyID::kHeight, value);
  } else if (name == html_names::kVspaceAttr) {
    AddHTMLLengthToStyle(style, CSSPropertyID::kMarginTop, value);
    AddHTMLLengthToStyle(style, CSSPropertyID::kMarginBottom, value);
  } else if (name == html_names::kHspaceAttr) {
    AddHTMLLengthToStyle(style, CSSPropertyID::kMarginLeft, value);
    AddHTMLLengthToStyle(style, CSSPropertyID::kMarginRight, value);
  } else if (name == html_names::kAlignAttr) {
    ApplyAlignmentAttributeToStyle(value, style);
  } else {
    HTMLFrameOwnerElement::CollectStyleForPresentationAttribute(name, value,
                                                                style);
  }
}

void HTMLPlugInElement::DefaultEventHandler(Event& event) {
  // Firefox seems to use a fake event listener to dispatch events to plugin
  // (tested with mouse events only). This is observable via different order
  // of events - in Firefox, event listeners specified in HTML attributes
  // fires first, then an event gets dispatched to plugin, and only then
  // other event listeners fire. Hopefully, this difference does not matter in
  // practice.

  // FIXME: Mouse down and scroll events are passed down to plugin via custom
  // code in EventHandler; these code paths should be united.

  LayoutObject* r = GetLayoutObject();
  if (!r || !r->IsLayoutEmbeddedContent())
    return;
  if (auto* embedded_object = DynamicTo<LayoutEmbeddedObject>(r)) {
    if (embedded_object->ShowsUnavailablePluginIndicator())
      return;
  }
  HTMLFrameOwnerElement::DefaultEventHandler(event);
}

bool HTMLPlugInElement::HasCustomFocusLogic() const {
  return !UseFallbackContent();
}

bool HTMLPlugInElement::IsPluginElement() const {
  return true;
}

bool HTMLPlugInElement::IsFocusableStyle(UpdateBehavior update_behavior) const {
  if (HTMLFrameOwnerElement::SupportsFocus(update_behavior) !=
          FocusableState::kNotFocusable &&
      HTMLFrameOwnerElement::IsFocusableStyle(update_behavior)) {
    return true;
  }

  if (UseFallbackContent() ||
      !HTMLFrameOwnerElement::IsFocusableStyle(update_behavior)) {
    return false;
  }
  return embedded_content_is_available_;
}

HTMLPlugInElement::ObjectContentType HTMLPlugInElement::GetObjectContentType()
    const {
  KURL url = GetDocument().CompleteURL(url_);
  String mime_type = ResolveMIMEType(service_type_, url);
  if (mime_type.empty()) {
    return ObjectContentType::kFrame;
  }

  if (MIMETypeRegistry::IsSupportedImageMIMEType(mime_type))
    return ObjectContentType::kImage;
  if (MIMETypeRegistry::IsSupportedNonImageMIMEType(mime_type))
    return ObjectContentType::kFrame;
  return ObjectContentType::kNone;
}

bool HTMLPlugInElement::IsImageType() const {
  if (GetDocument().GetFrame())
    return GetObjectContentType() == ObjectContentType::kImage;
  return MIMETypeRegistry::IsSupportedImageResourceMIMEType(service_type_);
}

LayoutEmbeddedObject* HTMLPlugInElement::GetLayoutEmbeddedObject() const {
  // HTMLObjectElement and HTMLEmbedElement may return arbitrary LayoutObjects
  // when using fallback content.
  return DynamicTo<LayoutEmbeddedObject>(GetLayoutObject());
}

// We don't use url_, as it may not be the final URL that the object loads,
// depending on <param> values.
bool HTMLPlugInElement::AllowedToLoadFrameURL(const String& url) {
  if (ContentFrame() && ProtocolIsJavaScript(url)) {
    return GetExecutionContext()->GetSecurityOrigin()->CanAccess(
        ContentFrame()->GetSecurityContext()->GetSecurityOrigin());
  }
  return true;
}

bool HTMLPlugInElement::RequestObject() {
  if (url_.empty() && service_type_.empty())
    return false;

  if (ProtocolIsJavaScript(url_))
    return false;

  KURL completed_url = url_.empty() ? KURL() : GetDocument().CompleteURL(url_);
  if (!AllowedToLoadObject(completed_url, service_type_))
    return false;

  ObjectContentType object_type = GetObjectContentType();
  if (object_type == ObjectContentType::kFrame ||
      object_type == ObjectContentType::kImage) {
    if (object_type == ObjectContentType::kFrame) {
      UseCounter::Count(GetDocument(),
                        WebFeature::kPluginElementLoadedDocument);
    } else if (object_type == ObjectContentType::kImage) {
      UseCounter::Count(GetDocument(), WebFeature::kPluginElementLoadedImage);
    }

    // If the plugin element already contains a subframe,
    // loadOrRedirectSubframe will re-use it. Otherwise, it will create a
    // new frame and set it as the LayoutEmbeddedContent's EmbeddedContentView,
    // causing what was previously in the EmbeddedContentView to be torn down.
    return LoadOrRedirectSubframe(completed_url, GetNameAttribute(), true);
  }

  return false;
}

void HTMLPlugInElement::DispatchErrorEvent() {
  ReportFallbackResourceTimingIfNeeded();
  DispatchEvent(*Event::Create(event_type_names::kError));
}

bool HTMLPlugInElement::AllowedToLoadObject(const KURL& url,
                                            const String& mime_type) {
  if (url.IsEmpty() && mime_type.empty())
    return false;

  // If present, `url` must contain a valid non-empty URL potentially surrounded
  // by spaces.
  if (!url.IsEmpty() && !url.IsValid()) {
    return false;
  }

  LocalFrame* frame = GetDocument().GetFrame();
  Settings* settings = frame->GetSettings();
  if (!settings)
    return false;

  if (MIMETypeRegistry::IsJavaAppletMIMEType(mime_type))
    return false;

  auto* csp = GetExecutionContext()->GetContentSecurityPolicy();
  if (!csp->AllowObjectFromSource(url)) {
    if (auto* layout_object = GetLayoutEmbeddedObject()) {
      embedded_content_is_available_ = false;
      layout_object->SetPluginAvailability(
          LayoutEmbeddedObject::kPluginBlockedByContentSecurityPolicy);
    }
    return false;
  }
  // If the URL is empty, a plugin could still be instantiated if a MIME-type
  // is specified.
  return (!mime_type.empty() && url.IsEmpty()) ||
         !MixedContentChecker::ShouldBlockFetch(
             frame, mojom::blink::RequestContextType::OBJECT,
             network::mojom::blink::IPAddressSpace::kUnknown, url,
             ResourceRequest::RedirectStatus::kNoRedirect, url,
             ReportingDisposition::kReport,
             GetDocument().Loader()->GetContentSecurityNotifier());
}

void HTMLPlugInElement::DidAddUserAgentShadowRoot(ShadowRoot&) {
  ShadowRoot* shadow_root = UserAgentShadowRoot();
  DCHECK(shadow_root);
  shadow_root->AppendChild(
      MakeGarbageCollected<HTMLSlotElement>(GetDocument()));
}

bool HTMLPlugInElement::HasFallbackContent() const {
  return false;
}

bool HTMLPlugInElement::UseFallbackContent() const {
  return false;
}

void HTMLPlugInElement::ReattachOnPluginChangeIfNeeded(bool require_layout) {
  if (UseFallbackContent() || !NeedsPluginUpdate() ||
      (require_layout && !GetLayoutObject())) {
    return;
  }

  SetNeedsStyleRecalc(
      kSubtreeStyleChange,
      StyleChangeReasonForTracing::Create(style_change_reason::kPluginChanged));
  SetForceReattachLayoutTree();

  // Make sure that we don't attempt to re-use the view through re-attachment.
  SetDisposeView();
}

void HTMLPlugInElement::UpdateServiceTypeIfEmpty() {
  if (service_type_.empty() && ProtocolIs(url_, "data")) {
    service_type_ = MimeTypeFromDataURL(url_);
  }
}

const ComputedStyle* HTMLPlugInElement::CustomStyleForLayoutObject(
    const StyleRecalcContext& style_recalc_context) {
  const ComputedStyle* style =
      OriginalStyleForLayoutObject(style_recalc_context);
  if (IsImageType() && !GetLayoutObject() && style &&
      LayoutObjectIsNeeded(*style)) {
    if (!image_loader_) {
      image_loader_ = MakeGarbageCollected<HTMLImageLoader>(this);
    }
    image_loader_->UpdateFromElement(ImageLoader::kUpdateNormal,
                                     /* force_blocking */ true);
  }
  return style;
}

}  // namespace blink
