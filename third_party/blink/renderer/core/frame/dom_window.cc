// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/dom_window.h"

#include <algorithm>
#include <memory>

#include "base/containers/fixed_flat_map.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/trace_event/trace_event.h"
#include "services/network/public/mojom/web_sandbox_flags.mojom-blink.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/mojom/frame/frame.mojom-blink.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/event_target_names.h"
#include "third_party/blink/renderer/core/events/message_event.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/execution_context/security_context.h"
#include "third_party/blink/renderer/core/frame/coop_access_violation_report_body.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/frame.h"
#include "third_party/blink/renderer/core/frame/frame_client.h"
#include "third_party/blink/renderer/core/frame/frame_console.h"
#include "third_party/blink/renderer/core/frame/frame_owner.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/location.h"
#include "third_party/blink/renderer/core/frame/report.h"
#include "third_party/blink/renderer/core/frame/reporting_context.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/user_activation.h"
#include "third_party/blink/renderer/core/input/input_device_capabilities.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/focus_controller.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/url/dom_origin.h"
#include "third_party/blink/renderer/platform/bindings/source_location.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/text/strcat.h"

namespace blink {


DOMWindow::DOMWindow(Frame& frame)
    : frame_(frame), window_is_closing_(false) {}

DOMWindow::~DOMWindow() {
  // The frame must be disconnected before finalization.
  DCHECK(!frame_);
}

DOMOrigin* DOMWindow::GetDOMOrigin(LocalDOMWindow* accessing_window) const {
  const auto* local_window = DynamicTo<LocalDOMWindow>(this);
  if (!local_window || !accessing_window) {
    return nullptr;
  }
  // The origin is only exposed to a same-origin accessor. This is the
  // V8-free equivalent of the cross-origin check BindingSecurity used to
  // perform on the accessing script's realm.
  const SecurityOrigin* accessing_origin =
      accessing_window->GetSecurityOrigin();
  const SecurityOrigin* target_origin = local_window->GetSecurityOrigin();
  if (!accessing_origin || !target_origin ||
      !accessing_origin->CanAccess(target_origin)) {
    return nullptr;
  }
  return DOMOrigin::Create(target_origin);
}

const AtomicString& DOMWindow::InterfaceName() const {
  return event_target_names::kWindow;
}

const DOMWindow* DOMWindow::ToDOMWindow() const {
  return this;
}

bool DOMWindow::IsWindowOrWorkerGlobalScope() const {
  return true;
}

Location* DOMWindow::location() const {
  if (!location_)
    location_ = MakeGarbageCollected<Location>(const_cast<DOMWindow*>(this));
  return location_.Get();
}

bool DOMWindow::closed() const {
  return window_is_closing_ || !GetFrame() || !GetFrame()->GetPage();
}

unsigned DOMWindow::length() const {
  return GetFrame() ? GetFrame()->Tree().ScopedChildCount() : 0;
}

DOMWindow* DOMWindow::self() const {
  if (!GetFrame())
    return nullptr;

  return GetFrame()->DomWindow();
}

DOMWindow* DOMWindow::window() const {
  if (!GetFrame())
    return nullptr;

  return GetFrame()->DomWindow();
}

DOMWindow* DOMWindow::frames() const {
  if (!GetFrame())
    return nullptr;

  return GetFrame()->DomWindow();
}

DOMWindow* DOMWindow::opener() const {
  // FIXME: Use FrameTree to get opener as well, to simplify logic here.
  if (!GetFrame() || !GetFrame()->Client())
    return nullptr;

  Frame* opener = GetFrame()->Opener();
  return opener ? opener->DomWindow() : nullptr;
}

DOMWindow* DOMWindow::parent() const {
  if (!GetFrame())
    return nullptr;

  Frame* parent = GetFrame()->Tree().Parent();
  return parent ? parent->DomWindow() : GetFrame()->DomWindow();
}

DOMWindow* DOMWindow::top() const {
  if (!GetFrame())
    return nullptr;

  return GetFrame()->Tree().Top().DomWindow();
}

bool DOMWindow::IsCurrentlyDisplayedInFrame() const {
  if (GetFrame())
    SECURITY_CHECK(GetFrame()->DomWindow() == this);
  return GetFrame() && GetFrame()->GetPage();
}

// FIXME: Once we're throwing exceptions for cross-origin access violations, we
// will always sanitize the target frame details, so we can safely combine
// 'crossDomainAccessErrorMessage' with this method after considering exactly
// which details may be exposed to JavaScript.
//
// http://crbug.com/17325
String DOMWindow::SanitizedCrossDomainAccessErrorMessage(
    const LocalDOMWindow* accessing_window,
    CrossDocumentAccessPolicy cross_document_access) const {
  if (!accessing_window || !GetFrame())
    return String();

  const KURL& accessing_window_url = accessing_window->Url();
  if (accessing_window_url.IsNull())
    return String();

  const SecurityOrigin* active_origin = accessing_window->GetSecurityOrigin();
  String message;
  if (cross_document_access == CrossDocumentAccessPolicy::kDisallowed) {
    message =
        StrCat({"Blocked a restricted frame with origin \"",
                active_origin->ToString(), "\" from accessing another frame."});
  } else {
    message =
        StrCat({"Blocked a frame with origin \"", active_origin->ToString(),
                "\" from accessing a cross-origin frame."});
  }

  // FIXME: Evaluate which details from 'crossDomainAccessErrorMessage' may
  // safely be reported to JavaScript.

  return message;
}

String DOMWindow::CrossDomainAccessErrorMessage(
    const LocalDOMWindow* accessing_window,
    CrossDocumentAccessPolicy cross_document_access) const {
  if (!accessing_window || !GetFrame())
    return String();

  const KURL& accessing_window_url = accessing_window->Url();
  if (accessing_window_url.IsNull())
    return String();

  const SecurityOrigin* active_origin = accessing_window->GetSecurityOrigin();
  const SecurityOrigin* target_origin =
      GetFrame()->GetSecurityContext()->GetSecurityOrigin();
  auto* local_dom_window = DynamicTo<LocalDOMWindow>(this);
  // It's possible for a remote frame to be same origin with respect to a
  // local frame, but it must still be treated as a disallowed cross-domain
  // access. See https://crbug.com/601629.
  DCHECK(GetFrame()->IsRemoteFrame() ||
         !active_origin->CanAccess(target_origin) ||
         (local_dom_window &&
          accessing_window->GetAgent() != local_dom_window->GetAgent()));

  String message =
      StrCat({"Blocked a frame with origin \"", active_origin->ToString(),
              "\" from accessing a frame with origin \"",
              target_origin->ToString(), "\". "});

  // Sandbox errors: Use the origin of the frames' location, rather than their
  // actual origin (since we know that at least one will be "null").
  KURL active_url = accessing_window->Url();
  // TODO(alexmos): RemoteFrames do not have a document, and their URLs
  // aren't replicated.  For now, construct the URL using the replicated
  // origin for RemoteFrames. If the target frame is remote and sandboxed,
  // there isn't anything else to show other than "null" for its origin.
  KURL target_url = local_dom_window
                        ? local_dom_window->Url()
                        : KURL(NullUrl(), target_origin->ToString());
  using SandboxFlags = network::mojom::blink::WebSandboxFlags;
  if (GetFrame()->GetSecurityContext()->IsSandboxed(SandboxFlags::kOrigin) ||
      accessing_window->IsSandboxed(SandboxFlags::kOrigin)) {
    message = StrCat({"Blocked a frame at \"",
                      SecurityOrigin::Create(active_url)->ToString(),
                      "\" from accessing a frame at \"",
                      SecurityOrigin::Create(target_url)->ToString(), "\". "});

    if (GetFrame()->GetSecurityContext()->IsSandboxed(SandboxFlags::kOrigin) &&
        accessing_window->IsSandboxed(SandboxFlags::kOrigin)) {
      return StrCat({"Sandbox access violation: ", message,
                     " Both frames are sandboxed and lack the "
                     "\"allow-same-origin\" flag."});
    }

    if (GetFrame()->GetSecurityContext()->IsSandboxed(SandboxFlags::kOrigin)) {
      return StrCat({"Sandbox access violation: ", message,
                     " The frame being accessed is sandboxed and lacks "
                     "the \"allow-same-origin\" flag."});
    }

    return StrCat({"Sandbox access violation: ", message,
                   " The frame requesting access is sandboxed and lacks "
                   "the \"allow-same-origin\" flag."});
  }

  // Protocol errors: Use the URL's protocol rather than the origin's protocol
  // so that we get a useful message for non-heirarchal URLs like 'data:'.
  if (target_origin->Protocol() != active_origin->Protocol()) {
    return StrCat({message, " The frame requesting access has a protocol of \"",
                   active_url.Protocol(),
                   "\", the frame being accessed has a protocol of \"",
                   target_url.Protocol(), "\". Protocols must match."});
  }

  // 'document.domain' errors.
  if (target_origin->DomainWasSetInDOM() &&
      active_origin->DomainWasSetInDOM()) {
    return StrCat(
        {message, "The frame requesting access set \"document.domain\" to \"",
         active_origin->Domain(), "\", the frame being accessed set it to \"",
         target_origin->Domain(),
         "\". Both must set \"document.domain\" to the same value to allow "
         "access."});
  }
  if (active_origin->DomainWasSetInDOM()) {
    return StrCat({message,
                   "The frame requesting access set \"document.domain\" to \"",
                   active_origin->Domain(),
                   "\", but the frame being accessed did not. Both must set "
                   "\"document.domain\" to the same value to allow access."});
  }
  if (target_origin->DomainWasSetInDOM()) {
    return StrCat({message,
                   "The frame being accessed set \"document.domain\" to \"",
                   target_origin->Domain(),
                   "\", but the frame requesting access did not. Both must set "
                   "\"document.domain\" to the same value to allow access."});
  }
  if (cross_document_access == CrossDocumentAccessPolicy::kDisallowed) {
    return StrCat({message, "The document-access policy denied access."});
  }

  if (active_origin->CanAccess(target_origin)) {
    return StrCat({message,
                   "The frames are same-origin but belong to different agent "
                   "clusters, possibly due to conflicting Document Isolation "
                   "Policies."});
  }

  // Default.
  return StrCat({message, "Protocols, domains, and ports must match."});
}

void DOMWindow::Close(LocalDOMWindow* incumbent_window) {
  DCHECK(incumbent_window);

  if (!GetFrame() || !GetFrame()->IsOutermostMainFrame())
    return;

  Page* page = GetFrame()->GetPage();
  if (!page)
    return;

  Document* active_document = incumbent_window->document();
  if (!(active_document && active_document->GetFrame() &&
        active_document->GetFrame()->CanNavigate(*GetFrame()))) {
    return;
  }

  Settings* settings = GetFrame()->GetSettings();
  bool allow_scripts_to_close_windows =
      settings && settings->GetAllowScriptsToCloseWindows();

  if (!page->OpenedByDOM() && !allow_scripts_to_close_windows) {
    if (GetFrame()->Client()->BackForwardLength() > 1) {
      active_document->domWindow()->GetFrameConsole()->AddMessage(
          MakeGarbageCollected<ConsoleMessage>(
              mojom::blink::ConsoleMessageSource::kJavaScript,
              mojom::blink::ConsoleMessageLevel::kWarning,
              "Scripts may close only the windows that were opened by them."));
      return;
    } else {
      // https://html.spec.whatwg.org/multipage/nav-history-apis.html#script-closable
      // allows a window to be closed if its history length is 1, even if it was
      // not opened by script.
      UseCounter::Count(active_document,
                        WebFeature::kWindowCloseHistoryLengthOne);
    }
  }

  if (!GetFrame()->ShouldClose())
    return;

  ExecutionContext* execution_context = nullptr;
  if (auto* local_dom_window = DynamicTo<LocalDOMWindow>(this)) {
    execution_context = local_dom_window->GetExecutionContext();
  }
  probe::BreakableLocation(execution_context, "DOMWindow.close");

  page->CloseSoon();

  // So as to make window.closed return the expected result
  // after window.close(), separately record the to-be-closed
  // state of this window. Scripts may access window.closed
  // before the deferred close operation has gone ahead.
  window_is_closing_ = true;
}

InputDeviceCapabilitiesConstants* DOMWindow::GetInputDeviceCapabilities() {
  if (!input_capabilities_) {
    input_capabilities_ =
        MakeGarbageCollected<InputDeviceCapabilitiesConstants>();
  }
  return input_capabilities_.Get();
}

void DOMWindow::Trace(Visitor* visitor) const {
  visitor->Trace(frame_);
  visitor->Trace(input_capabilities_);
  visitor->Trace(location_);
  EventTarget::Trace(visitor);
}

}  // namespace blink
