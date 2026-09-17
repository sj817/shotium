/*
 * Copyright (C) 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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

#include "third_party/blink/renderer/core/frame/local_dom_window.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/command_line.h"
#include "base/metrics/histogram_functions.h"
#include "base/task/single_thread_task_runner.h"
#include "base/trace_event/trace_id_helper.h"
#include "base/trace_event/typed_macros.h"
#include "build/build_config.h"
#include "cc/input/snap_selection_strategy.h"
#include "net/storage_access_api/status.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/switches.h"
#include "third_party/blink/public/mojom/frame/frame.mojom-blink.h"
#include "third_party/blink/public/mojom/permissions_policy/policy_disposition.mojom-blink.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_scroll_to_options.h"
#include "third_party/blink/renderer/core/css/css_computed_style_declaration.h"
#include "third_party/blink/renderer/core/css/css_rule_list.h"
#include "third_party/blink/renderer/core/css/dom_window_css.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver.h"
#include "third_party/blink/renderer/core/dom/document_init.h"
#include "third_party/blink/renderer/core/dom/events/event_dispatch_forbidden_scope.h"
#include "third_party/blink/renderer/core/dom/events/scoped_event_queue.h"
#include "third_party/blink/renderer/core/dom/frame_request_callback_collection.h"
#include "third_party/blink/renderer/core/dom/scriptable_document_parser.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/events/error_event.h"
#include "third_party/blink/renderer/core/events/hash_change_event.h"
#include "third_party/blink/renderer/core/events/page_transition_event.h"
#include "third_party/blink/renderer/core/events/pop_state_event.h"
#include "third_party/blink/renderer/core/execution_context/execution_context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/execution_context/window_agent.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/dom_visual_viewport.h"
#include "third_party/blink/renderer/core/frame/event_handler_registry.h"
#include "third_party/blink/renderer/core/frame/frame_console.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/viewport_data.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_registry.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_registry_assignment.h"
#include "third_party/blink/renderer/core/html/forms/form_controller.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/layout/adjust_for_absolute_zoom.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/frame_load_request.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/paint/paint_layer_scrollable_area.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/route_matching/navigation_state.h"
#include "third_party/blink/renderer/core/scheduler/scripted_idle_task_controller.h"
#include "third_party/blink/renderer/core/scheduler/task_attribution_util.h"
#include "third_party/blink/renderer/core/scroll/scroll_promise_resolver.h"
#include "third_party/blink/renderer/core/scroll/scroll_types.h"
#include "third_party/blink/renderer/core/scroll/scrollbar_theme.h"
#include "third_party/blink/renderer/core/timing/dom_window_performance.h"
#include "third_party/blink/renderer/core/timing/event_timing.h"
#include "third_party/blink/renderer/core/timing/soft_navigation_heuristics.h"
#include "third_party/blink/renderer/core/timing/window_performance.h"
#include "third_party/blink/renderer/core/trustedtypes/trusted_type_policy_factory.h"
#include "third_party/blink/renderer/platform/back_forward_cache_buffer_limit_tracker.h"
#include "third_party/blink/renderer/platform/bindings/source_location.h"
#include "third_party/blink/renderer/platform/blob/blob_url.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/network/network_state_notifier.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/scheduler/public/dummy_schedulers.h"
#include "third_party/blink/renderer/platform/scheduler/public/event_loop.h"
#include "third_party/blink/renderer/platform/scheduler/public/post_cross_thread_task.h"
#include "third_party/blink/renderer/platform/storage/blink_storage_key.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_copier_std.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/wtf/text/strcat.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/uuid.h"

namespace blink {

// The file-local RequestAnimationFrame() helper was here. It wrapped a script
// FrameRequestCallback in a V8FrameCallback and handed it to the document. Both
// halves are gone with the bindings.

class LocalDOMWindow::NetworkStateObserver final
    : public GarbageCollected<LocalDOMWindow::NetworkStateObserver>,
      public NetworkStateNotifier::NetworkStateObserver,
      public ExecutionContextLifecycleObserver {
 public:
  explicit NetworkStateObserver(ExecutionContext* context)
      : ExecutionContextLifecycleObserver(context) {}

  void Initialize() {
    online_observer_handle_ = GetNetworkStateNotifier().AddOnLineObserver(
        this, GetExecutionContext()->GetTaskRunner(TaskType::kNetworking));
  }

  void OnLineStateChange(bool on_line) override {
    AtomicString event_name =
        on_line ? event_type_names::kOnline : event_type_names::kOffline;
    auto* window = To<LocalDOMWindow>(GetExecutionContext());
    window->DispatchEvent(*Event::Create(event_name));
  }

  void ContextDestroyed() override { online_observer_handle_ = nullptr; }

  void Trace(Visitor* visitor) const override {
    ExecutionContextLifecycleObserver::Trace(visitor);
  }

 private:
  std::unique_ptr<NetworkStateNotifier::NetworkStateObserverHandle>
      online_observer_handle_;
};

LocalDOMWindow::LocalDOMWindow(LocalFrame& frame, WindowAgent* agent)
    : DOMWindow(frame),
      // The v8::Isolate* that used to lead this argument list is gone with V8;
      // ExecutionContext no longer holds one.
      ExecutionContext(agent),
      visualViewport_(MakeGarbageCollected<DOMVisualViewport>(this)),
      token_(frame.GetLocalFrameToken()),
      network_state_observer_(MakeGarbageCollected<NetworkStateObserver>(this)),
      closewatcher_stack_(
          MakeGarbageCollected<CloseWatcher::WatcherStack>(this)) {}

void LocalDOMWindow::BindContentSecurityPolicy() {
  DCHECK(!GetContentSecurityPolicy()->IsBound());
  GetContentSecurityPolicy()->BindToDelegate(
      GetContentSecurityPolicyDelegate());
}

void LocalDOMWindow::Initialize() {
  network_state_observer_->Initialize();
}

void LocalDOMWindow::ClearForReuse() {
  is_dom_window_reused_ = true;
  // update event listener counts before clearing document_
  if (document_ && HasEventListeners()) {
    GetEventTargetData()->event_listener_map.ForAllEventListenerTypes(
        [this](const AtomicString& event_type, uint32_t count) {
          document_->DidRemoveEventListeners(count);
        });
  }
  // Reset per-document metrics bookkeeping before clearing `document_`.
  if (soft_navigation_heuristics_) {
    soft_navigation_heuristics_->Shutdown();
    soft_navigation_heuristics_ = nullptr;
  }
  document_ = nullptr;

  WindowPerformance::ClearForWindowReuse(*this);
}

void LocalDOMWindow::ResetWindowAgent(WindowAgent* agent) {
  ResetAgent(agent);
  if (document_) {
    document_->ResetAgent(*agent);
  }

  CHECK(GetFrame());
  GetFrame()->GetFrameScheduler()->SetAgentClusterId(GetAgentClusterID());

}

void LocalDOMWindow::AcceptLanguagesChanged() {
  if (navigator_) {
    navigator_->SetLanguagesDirty();
  }

  DispatchEvent(*Event::Create(event_type_names::kLanguagechange));
}

TrustedTypePolicyFactory* LocalDOMWindow::GetTrustedTypes() const {
  DCHECK(IsMainThread());
  if (!trusted_types_) {
    trusted_types_ =
        MakeGarbageCollected<TrustedTypePolicyFactory>(GetExecutionContext());
  }
  return trusted_types_.Get();
}

mojom::blink::V8CacheOptions LocalDOMWindow::GetV8CacheOptions() const {
  if (LocalFrame* frame = GetFrame()) {
    if (const Settings* settings = frame->GetSettings()) {
      return settings->GetV8CacheOptions();
    }
  }

  return mojom::blink::V8CacheOptions::kDefault;
}

bool LocalDOMWindow::IsContextThread() const {
  return IsMainThread();
}

const KURL& LocalDOMWindow::Url() const {
  return document()->Url();
}

const KURL& LocalDOMWindow::BaseURL() const {
  return document()->BaseURL();
}

KURL LocalDOMWindow::CompleteURL(const String& url) const {
  return document()->CompleteURL(url);
}

void LocalDOMWindow::DisableEval(const String& error_message) {
  // This forwarded the CSP-derived message to the script engine so that a
  // later eval() would throw it. There is no script engine and no eval(), so
  // there is nothing to arm.
}

void LocalDOMWindow::SetWasmEvalErrorMessage(const String& error_message) {
  // Same as DisableEval(): nothing can compile WebAssembly here.
}

String LocalDOMWindow::UserAgent() const {
  if (!GetFrame()) {
    return String();
  }

  return GetFrame()->Loader().UserAgent();
}

UserAgentMetadata LocalDOMWindow::GetUserAgentMetadata() const {
  return GetFrame()->Loader().UserAgentMetadata().value_or(
      blink::UserAgentMetadata());
}

HttpsState LocalDOMWindow::GetHttpsState() const {
  // TODO(https://crbug.com/880986): Implement Document's HTTPS state in more
  // spec-conformant way.
  return CalculateHttpsState(GetSecurityOrigin());
}

ResourceFetcher* LocalDOMWindow::Fetcher() {
  return document()->Fetcher();
}

bool LocalDOMWindow::CanExecuteScripts(
    ReasonForCallingCanExecuteScripts reason) {
  if (!GetFrame()) {
    return false;
  }

  // Detached frames should not be attempting to execute script.
  DCHECK(!GetFrame()->IsDetached());

  // Normally, scripts are not allowed in sandboxed contexts that disallow them.
  // However, there is an exception for cases when the script should bypass the
  // main world's CSP (such as for privileged isolated worlds). See
  // https://crbug.com/811528.
  if (IsSandboxed(network::mojom::blink::WebSandboxFlags::kScripts) &&
      !ContentSecurityPolicy::ShouldBypassMainWorldDeprecated(this)) {
    // FIXME: This message should be moved off the console once a solution to
    // https://bugs.webkit.org/show_bug.cgi?id=103274 exists.
    if (reason == kAboutToExecuteScript) {
      AddConsoleMessage(MakeGarbageCollected<ConsoleMessage>(
          mojom::blink::ConsoleMessageSource::kSecurity,
          mojom::blink::ConsoleMessageLevel::kError,
          StrCat({"Blocked script execution in '", Url().ElidedString(),
                  "' because the document's frame is sandboxed and the "
                  "'allow-scripts' permission is not set."})));
    }
    return false;
  }
  bool script_enabled = GetFrame()->ScriptEnabled();
  if (!script_enabled && reason == kAboutToExecuteScript) {
    WebContentSettingsClient* settings_client =
        GetFrame()->GetContentSettingsClient();
    if (settings_client) {
      settings_client->DidNotAllowScript();
    }
  }
  return script_enabled;
}

bool LocalDOMWindow::AllowInlineJavascriptUrl(const KURL& url,
                                              Element* element) {
  // Only the CSP verdict matters here; the javascript: URL's script text is
  // never extracted because nothing can run it.

  // AllowInline below will check the source's hash against CSP, which is why
  // it needs an exact script_source.
  String decoded_url = DecodeUrlEscapeSequences(
      url.GetString(), DecodeUrlMode::kUtf8OrIsomorphic);

  // Check the CSP of the caller (the "source browsing context") if required,
  // as per https://html.spec.whatwg.org/C/#javascript-protocol.
  return GetContentSecurityPolicy()->AllowInline(
      ContentSecurityPolicy::InlineType::kNavigation, element, decoded_url,
      String() /* nonce */, Url(),
      TextPosition(OrdinalNumber::First(), OrdinalNumber::BeforeFirst()));
}

void LocalDOMWindow::ExceptionThrown(ErrorEvent* event) {
  // This used to hand the exception to the script engine's debugger, which
  // logged it and forwarded it to any attached inspector. Neither exists any
  // more, so report the error on the console directly, keeping the source
  // location the ErrorEvent carries.
  String text = event->message();
  if (SourceLocation* location = event->Location();
      location && !location->Url().empty()) {
    text = StrCat({text, " (", location->Url(), ":",
                   String::Number(location->LineNumber()), ":",
                   String::Number(location->ColumnNumber()), ")"});
  }
  AddConsoleMessage(
      MakeGarbageCollected<ConsoleMessage>(
          mojom::blink::ConsoleMessageSource::kJavaScript,
          mojom::blink::ConsoleMessageLevel::kError, text),
      /*discard_duplicates=*/false);
}

// https://w3c.github.io/webappsec-referrer-policy/#determine-requests-referrer
KURL LocalDOMWindow::OutgoingReferrerUrl() const {
  // Step 3.1: "If environment's global object is a Window object, then"
  // Step 3.1.1: "Let document be the associated Document of environment's
  // global object."

  // Step 3.1.2: "If document's origin is an opaque origin, return no referrer."
  if (GetSecurityOrigin()->IsOpaque()) {
    return NullUrl();
  }

  // Step 3.1.3: "While document is an iframe srcdoc document, let document be
  // document's browsing context's browsing context container's node document."
  Document* referrer_document = document();
  if (LocalFrame* frame = GetFrame()) {
    while (frame->GetDocument()->IsSrcdocDocument()) {
      // Srcdoc documents must be local within the containing frame.
      frame = To<LocalFrame>(frame->Tree().Parent());
      // Srcdoc documents cannot be top-level documents, by definition,
      // because they need to be contained in iframes with the srcdoc.
      DCHECK(frame);
    }
    referrer_document = frame->GetDocument();
  }

  // Step: 3.1.4: "Let referrerSource be document's URL."
  return referrer_document->OutgoingReferrerUrl();
}

void LocalDOMWindow::SetInitiatorStateToken(
    const InitiatorStateToken& initiator_state_token) {
  initiator_state_token_ = initiator_state_token;
}

CoreProbeSink* LocalDOMWindow::GetProbeSink() {
  return probe::ToCoreProbeSink(GetFrame());
}

const BrowserInterfaceBrokerProxy& LocalDOMWindow::GetBrowserInterfaceBroker()
    const {
  if (!GetFrame()) {
    return GetEmptyBrowserInterfaceBroker();
  }

  return GetFrame()->GetBrowserInterfaceBroker();
}

FrameOrWorkerScheduler* LocalDOMWindow::GetScheduler() {
  if (GetFrame()) {
    return GetFrame()->GetFrameScheduler();
  }
  if (!detached_scheduler_) {
    // CreateDummyFrameScheduler() used to take the v8::Isolate* to tag its
    // idle tasks with; the isolate parameter is gone along with V8.
    detached_scheduler_ = scheduler::CreateDummyFrameScheduler();
  }
  return detached_scheduler_.get();
}

scoped_refptr<base::SingleThreadTaskRunner> LocalDOMWindow::GetTaskRunner(
    TaskType type) {
  if (GetFrame()) {
    return GetFrame()->GetTaskRunner(type);
  }
  TRACE_EVENT_INSTANT("blink",
                      "LocalDOMWindow::GetTaskRunner_ThreadTaskRunner");
  // In most cases, the ExecutionContext will get us to a relevant Frame. In
  // some cases, though, there isn't a good candidate (most commonly when either
  // the passed-in document or the ExecutionContext used to be attached to a
  // Frame but has since been detached) so we will use the default task runner
  // of the AgentGroupScheduler that created this window.
  return To<WindowAgent>(GetAgent())
      ->GetAgentGroupScheduler()
      .DefaultTaskRunner();
}

void LocalDOMWindow::ReportPermissionsPolicyViolation(
    network::mojom::PermissionsPolicyFeature feature,
    mojom::blink::PolicyDisposition disposition,
    const String& reporting_endpoint,
    const String& message) const {
  // shotium has no ReportingObserver endpoint, no browser-side UMA/UKM sink
  // and no DevTools; policy violations and feature use are not recorded.
}

void LocalDOMWindow::ReportPotentialPermissionsPolicyViolation(
    network::mojom::PermissionsPolicyFeature feature,
    mojom::blink::PolicyDisposition disposition,
    const String& reporting_endpoint,
    const String& message,
    const String& allow_attribute,
    const String& src_attribute) const {}

void LocalDOMWindow::ReportDocumentPolicyViolation(
    mojom::blink::DocumentPolicyFeature feature,
    mojom::blink::PolicyDisposition disposition,
    const String& message,
    const String& source_file) const {}

void LocalDOMWindow::AddConsoleMessageImpl(ConsoleMessage* console_message,
                                           bool discard_duplicates) {
  CHECK(IsContextThread());

  if (!GetFrame()) {
    return;
  }

  if (document() && console_message->Location()->IsUnknown()) {
    // TODO(dgozman): capture correct location at call places instead.
    unsigned line_number = 0;
    if (!document()->IsInDocumentWrite() &&
        document()->GetScriptableDocumentParser()) {
      ScriptableDocumentParser* parser =
          document()->GetScriptableDocumentParser();
      if (parser->IsParsingAtLineNumber()) {
        line_number = parser->LineNumber().OneBasedInt();
      }
    }
    Vector<DOMNodeId> nodes(console_message->Nodes());
    std::optional<mojom::blink::ConsoleMessageCategory> category =
        console_message->Category();
    console_message = MakeGarbageCollected<ConsoleMessage>(
        console_message->GetSource(), console_message->GetLevel(),
        console_message->Message(),
        // The trailing nullptr used to be a captured V8 stack trace; the
        // (url, function, line, column) constructor no longer takes one.
        MakeGarbageCollected<SourceLocation>(Url().GetString(), String(),
                                             line_number, 0));
    console_message->SetNodes(GetFrame(), std::move(nodes));
    if (category) {
      console_message->SetCategory(*category);
    }
  }

  GetFrame()->Console().AddMessage(console_message, discard_duplicates);
}

scoped_refptr<base::SingleThreadTaskRunner>
LocalDOMWindow::GetAgentGroupSchedulerCompositorTaskRunner() {
  if (!GetFrame()) {
    return nullptr;
  }
  auto* frame_scheduler = GetFrame()->GetFrameScheduler();
  return frame_scheduler->GetAgentGroupScheduler()->CompositorTaskRunner();
}

void LocalDOMWindow::CountUse(mojom::WebFeature feature) {}

void LocalDOMWindow::CountWebDXFeature(mojom::blink::WebDXFeature feature) {}

void LocalDOMWindow::CountPermissionsPolicyUsage(
    network::mojom::PermissionsPolicyFeature feature,
    UseCounterImpl::PermissionsPolicyUsageType type) {}

void LocalDOMWindow::CountUseOnlyInCrossSiteIframe(
    mojom::blink::WebFeature feature) {}

bool LocalDOMWindow::HasInsecureContextInAncestors() const {
  for (Frame* parent = GetFrame()->Tree().Parent(); parent;
       parent = parent->Tree().Parent()) {
    // Stop the walk at a secure-context root; see
    // `ContentBrowserClient::IsSecureContextRoot()` for details.
    if (parent->GetSecurityContext()->IsSecureContextRoot()) {
      return false;
    }
    auto* origin = parent->GetSecurityContext()->GetSecurityOrigin();
    if (!origin->IsPotentiallyTrustworthy()) {
      return true;
    }
  }
  return false;
}

Document* LocalDOMWindow::InstallNewDocument(const DocumentInit& init) {
  // Blink should never attempt to install a new Document to a LocalDOMWindow
  // that's not attached to a LocalFrame.
  DCHECK(GetFrame());
  // Either:
  // - `this` should be a new LocalDOMWindow, that has never had a Document
  //   associated with it or
  // - `this` is being reused, and the previous Document has been disassociated
  //   via `ClearForReuse()`.
  DCHECK(!document_);
  DCHECK_EQ(init.GetWindow(), this);

  document_ = init.CreateDocument();
  document_->Initialize();

  document_->GetViewportData().UpdateViewportDescription();

  auto* frame_scheduler = GetFrame()->GetFrameScheduler();
  frame_scheduler->OnDidInstallNewDocument();
  frame_scheduler->TraceUrlChange(document_->Url().GetString());
  frame_scheduler->SetCrossOriginToNearestMainFrame(
      GetFrame()->IsCrossOriginToNearestMainFrame());

  GetFrame()->GetPage()->GetChromeClient().InstallSupplements(*GetFrame());

  UpdateEventListenerCountsToDocumentForReuseIfNeeded();

  CHECK(!soft_navigation_heuristics_);
  soft_navigation_heuristics_ = SoftNavigationHeuristics::CreateIfNeeded(this);

  return document_.Get();
}

void LocalDOMWindow::EnqueueWindowEvent(Event& event, TaskType task_type) {
  EnqueueEvent(event, task_type);
}

void LocalDOMWindow::EnqueueDocumentEvent(Event& event, TaskType task_type) {
  if (document_) {
    document_->EnqueueEvent(event, task_type);
  }
}

void LocalDOMWindow::DispatchWindowLoadEvent() {
#if DCHECK_IS_ON()
  DCHECK(!EventDispatchForbiddenScope::IsEventDispatchForbidden());
#endif
  // Delay 'load' event if we are in EventQueueScope.  This is a short-term
  // workaround to avoid Editing code crashes.  We should always dispatch
  // 'load' event asynchronously.  crbug.com/569511.
  if (ScopedEventQueue::Instance()->ShouldQueueEvents() && document_) {
    document_->GetTaskRunner(TaskType::kNetworking)
        ->PostTask(FROM_HERE, BindOnce(&LocalDOMWindow::DispatchLoadEvent,
                                       WrapPersistent(this)));
    return;
  }
  DispatchLoadEvent();
}

void LocalDOMWindow::DispatchLoadAndPageshowEvents() {
  DispatchWindowLoadEvent();

  // An extension to step 4.5. or a part of step 4.6.3. of
  // https://html.spec.whatwg.org/C/#traverse-the-history .
  //
  // 4.5. ..., invoke the reset algorithm of each of those elements.
  // 4.6.3. Run any session history document visibility change steps ...
  if (document_) {
    document_->EnsureFormController().RestoreImmediately();
  }

  // 4.6.4. Fire an event named pageshow at the Document object's relevant
  // global object, ...
  EnqueueNonPersistedPageshowEvent();
}

void LocalDOMWindow::EnqueueNonPersistedPageshowEvent() {
  // FIXME: https://bugs.webkit.org/show_bug.cgi?id=36334 Pageshow event needs
  // to fire asynchronously.  As per spec pageshow must be triggered
  // asynchronously.  However to be compatible with other browsers blink fires
  // pageshow synchronously unless we are in EventQueueScope.
  if (ScopedEventQueue::Instance()->ShouldQueueEvents() && document_) {
    // The task source should be kDOMManipulation, but the spec doesn't say
    // anything about this.
    EnqueueWindowEvent(*PageTransitionEvent::Create(event_type_names::kPageshow,
                                                    false /* persisted */),
                       TaskType::kMiscPlatformAPI);
  } else {
    DispatchEvent(*PageTransitionEvent::Create(event_type_names::kPageshow,
                                               false /* persisted */),
                  document_.Get());
  }
}

void LocalDOMWindow::EnqueueHashchangeEvent(
    const String& old_url,
    const String& new_url,
    UserNavigationInvolvement involvement) {
  // https://html.spec.whatwg.org/C/#history-traversal
  EnqueueWindowEvent(*HashChangeEvent::Create(old_url, new_url, involvement),
                     TaskType::kDOMManipulation);
}

void LocalDOMWindow::DispatchPopstateEvent(
    bool has_ua_visual_transition,
    UserNavigationInvolvement involvement) {
  DCHECK(GetFrame());
  auto* event =
      PopStateEvent::Create(has_ua_visual_transition, involvement);
  NavigationEventTiming event_timing_scope(GetFrame(), *event);
  DispatchEvent(*event);
}
LocalDOMWindow::~LocalDOMWindow() = default;

void LocalDOMWindow::Dispose() {
  BackForwardCacheBufferLimitTracker::Get()
      .DidRemoveFrameOrWorkerFromBackForwardCache(
          total_bytes_buffered_while_in_back_forward_cache_);
  total_bytes_buffered_while_in_back_forward_cache_ = 0;

  // Oilpan: should the LocalDOMWindow be GCed along with its LocalFrame without
  // the frame having first notified its observers of imminent destruction, the
  // LocalDOMWindow will not have had an opportunity to remove event listeners.
  //
  // Arrange for that removal to happen using a prefinalizer action. Making
  // LocalDOMWindow eager finalizable is problematic as other eagerly finalized
  // objects may well want to access their associated LocalDOMWindow from their
  // destructors.
  if (!GetFrame()) {
    return;
  }

  RemoveAllEventListeners();
}

ExecutionContext* LocalDOMWindow::GetExecutionContext() const {
  return const_cast<LocalDOMWindow*>(this);
}

const LocalDOMWindow* LocalDOMWindow::ToLocalDOMWindow() const {
  return this;
}

LocalDOMWindow* LocalDOMWindow::ToLocalDOMWindow() {
  return this;
}

void LocalDOMWindow::FrameDestroyed() {
  TRACE_EVENT0("navigation", "LocalDOMWindow::FrameDestroyed");
  base::ScopedUmaHistogramTimer histogram_timer(
      "Navigation.LocalDOMWindow.FrameDestroyed");
  BackForwardCacheBufferLimitTracker::Get()
      .DidRemoveFrameOrWorkerFromBackForwardCache(
          total_bytes_buffered_while_in_back_forward_cache_);
  total_bytes_buffered_while_in_back_forward_cache_ = 0;

  // Some unit tests manually call FrameDestroyed(). Don't run it a second time.
  if (!GetFrame()) {
    return;
  }
  // Manually flush any remaining buffered performance entries before the window
  // is destroyed.
  DOMWindowPerformance::performance(*this)->FlushPerformanceEntries();
  // In the Reset() case, this Document::Shutdown() early-exits because it was
  // already called earlier in the commit process.
  // TODO(japhet): Can we merge this function and Reset()? At least, this
  // function should be renamed to Detach(), since in the Reset() case the frame
  // is not being destroyed.
  document()->Shutdown();
  document()->RemoveAllEventListenersRecursively();
  if (soft_navigation_heuristics_) {
    soft_navigation_heuristics_->Shutdown();
    soft_navigation_heuristics_ = nullptr;
  }
  NotifyContextDestroyed();
  RemoveAllEventListeners();
  DisconnectFromFrame();
}

void LocalDOMWindow::Reset() {
  DCHECK(document());
  FrameDestroyed();

  navigator_ = nullptr;
  custom_elements_ = nullptr;
  trusted_types_ = nullptr;
}

FrameConsole* LocalDOMWindow::GetFrameConsole() const {
  if (!IsCurrentlyDisplayedInFrame()) {
    return nullptr;
  }
  return &GetFrame()->Console();
}

Navigator* LocalDOMWindow::navigator() {
  if (!navigator_) {
    navigator_ = MakeGarbageCollected<Navigator>(this);
  }
  return navigator_.Get();
}

// navigation() was here. It lazily built the NavigationApi -- window.navigation
// -- which exists to let script observe and intercept navigations. Every caller
// asked it "does script want to stop this?" and, with no script engine, always
// got "no". The callers took that branch inline instead; see
// core/loader/frame_loader.cc.

// SchedulePostMessage(), DispatchPostMessage() and
// DispatchMessageEventWithOriginCheck() were here. PostedMessage carried a
// SerializedScriptValue -- the structured clone of a script value -- and
// postMessage() has no caller without a script engine, so the delivery side
// had nothing left to deliver.

double LocalDOMWindow::scrollX() const {
  if (!GetFrame() || !GetFrame()->GetPage()) {
    return 0;
  }

  LocalFrameView* view = GetFrame()->View();
  if (!view) {
    return 0;
  }

  // TODO(crbug.com/1499981): This should be removed once synchronized scrolling
  // impact is understood.

  document()->UpdateStyleAndLayout(DocumentUpdateReason::kJavaScript);

  // TODO(bokan): This is wrong when the document.rootScroller is non-default.
  // crbug.com/505516.
  double viewport_x = view->LayoutViewport()->GetWebExposedScrollOffset().x();
  return AdjustForAbsoluteZoom::AdjustScroll(viewport_x,
                                             GetFrame()->LayoutZoomFactor());
}

double LocalDOMWindow::scrollY() const {
  if (!GetFrame() || !GetFrame()->GetPage()) {
    return 0;
  }

  LocalFrameView* view = GetFrame()->View();
  if (!view) {
    return 0;
  }

  // TODO(crbug.com/1499981): This should be removed once synchronized scrolling
  // impact is understood.

  document()->UpdateStyleAndLayout(DocumentUpdateReason::kJavaScript);

  // TODO(bokan): This is wrong when the document.rootScroller is non-default.
  // crbug.com/505516.
  double viewport_y = view->LayoutViewport()->GetWebExposedScrollOffset().y();
  return AdjustForAbsoluteZoom::AdjustScroll(viewport_y,
                                             GetFrame()->LayoutZoomFactor());
}

DOMVisualViewport* LocalDOMWindow::visualViewport() {
  return visualViewport_.Get();
}

Document* LocalDOMWindow::document() const {
  return document_.Get();
}

CSSStyleDeclaration* LocalDOMWindow::getComputedStyle(
    Element* elt,
    const String& pseudo_elt) const {
  DCHECK(elt);
  return MakeGarbageCollected<CSSComputedStyleDeclaration>(elt, false,
                                                           pseudo_elt);
}

double LocalDOMWindow::devicePixelRatio() const {
  if (!GetFrame()) {
    return 0.0;
  }

  return GetFrame()->DevicePixelRatio();
}

void LocalDOMWindow::scrollTo(const ScrollToOptions* scroll_to_options) const {
  ScrollPromiseResolver* resolver =
      MakeGarbageCollected<ScrollPromiseResolver>();

  if (!IsCurrentlyDisplayedInFrame()) {
    return;
  }

  LocalFrameView* view = GetFrame()->View();
  Page* page = GetFrame()->GetPage();
  if (!view || !page) {
    return;
  }

  // TODO(crbug.com/1499981): This should be removed once synchronized scrolling
  // impact is understood.

  // It is only necessary to have an up-to-date layout if the position may be
  // clamped, which is never the case for (0, 0).
  if (!scroll_to_options->hasLeft() || !scroll_to_options->hasTop() ||
      scroll_to_options->left() || scroll_to_options->top()) {
    document()->UpdateStyleAndLayout(DocumentUpdateReason::kJavaScript);
  }

  float scaled_x = 0.0f;
  float scaled_y = 0.0f;

  PaintLayerScrollableArea* viewport = view->LayoutViewport();
  ScrollOffset current_offset = viewport->GetScrollOffset();
  scaled_x = current_offset.x();
  scaled_y = current_offset.y();

  if (scroll_to_options->hasLeft()) {
    scaled_x = ScrollableArea::NormalizeNonFiniteScroll(
                   base::saturated_cast<float>(scroll_to_options->left())) *
               GetFrame()->LayoutZoomFactor();
  }

  if (scroll_to_options->hasTop()) {
    scaled_y = ScrollableArea::NormalizeNonFiniteScroll(
                   base::saturated_cast<float>(scroll_to_options->top())) *
               GetFrame()->LayoutZoomFactor();
  }

  gfx::PointF new_scaled_position = viewport->ScrollOffsetToPosition(
      SnapScrollOffsetToPhysicalPixels(ScrollOffset(scaled_x, scaled_y)));

  std::unique_ptr<cc::SnapSelectionStrategy> strategy =
      cc::SnapSelectionStrategy::CreateForEndPosition(
          new_scaled_position, scroll_to_options->hasLeft(),
          scroll_to_options->hasTop());
  new_scaled_position =
      viewport->GetSnapPositionAndSetTarget(*strategy).value_or(
          new_scaled_position);
  mojom::blink::ScrollBehavior scroll_behavior =
      ScrollableArea::V8EnumToScrollBehavior(
          scroll_to_options->behavior().AsEnum());
  viewport->SetProgrammaticScrollOffset(
      viewport->ScrollPositionToOffset(new_scaled_position),
      cc::ScrollSourceType::kAbsoluteScroll, scroll_behavior,
      resolver->CreateActiveScrollTracker());
}


CustomElementRegistry* LocalDOMWindow::customElements() const {
  if (!custom_elements_ && document_) {
    // There is only the main world left, whose id is 0.
    custom_elements_ =
        MakeGarbageCollected<CustomElementRegistry>(this, /*world_id=*/0);
    custom_elements_->MarkAsGlobalRegistry();
    custom_elements_->AssociatedWith(*document_);
    document_->SetCustomElementRegistry(
        CustomElementRegistryAssignment::Explicit(custom_elements_.Get()));
  }
  return custom_elements_.Get();
}

CustomElementRegistry* LocalDOMWindow::MaybeCustomElements() const {
  return custom_elements_.Get();
}

bool IsSuddenTerminationDisablerEvent(const AtomicString& event_type) {
  return event_type == event_type_names::kUnload ||
         event_type == event_type_names::kBeforeunload ||
         event_type == event_type_names::kPagehide ||
         event_type == event_type_names::kVisibilitychange;
}

void LocalDOMWindow::AddedEventListener(
    const AtomicString& event_type,
    RegisteredEventListener& registered_listener) {
  DOMWindow::AddedEventListener(event_type, registered_listener);
  if (auto* frame = GetFrame()) {
    frame->GetEventHandlerRegistry().DidAddEventHandler(
        *this, event_type, registered_listener.Passive());
  }

  document()->AddListenerTypeIfNeeded(event_type, *this);
  document()->DidAddEventListeners(/*count*/ 1);
  if (registered_listener.Capture() &&
      RuntimeEnabledFeatures::SkipEventCaptureEnabled()) {
    document()->SetHasCaptureListener();
  }

  if (event_type == event_type_names::kUnload) {
    CountDeprecation(WebFeature::kDocumentUnloadRegistered);
  } else if (event_type == event_type_names::kBeforeunload) {
    UseCounter::Count(this, WebFeature::kDocumentBeforeUnloadRegistered);
    if (GetFrame() && !GetFrame()->IsMainFrame()) {
      UseCounter::Count(this, WebFeature::kSubFrameBeforeUnloadRegistered);
    }
  } else if (event_type == event_type_names::kPagehide) {
    UseCounter::Count(this, WebFeature::kDocumentPageHideRegistered);
  } else if (event_type == event_type_names::kPageshow) {
    UseCounter::Count(this, WebFeature::kDocumentPageShowRegistered);
  }

  if (GetFrame() && IsSuddenTerminationDisablerEvent(event_type)) {
    GetFrame()->AddedSuddenTerminationDisablerListener(*this, event_type);
  }
}

void LocalDOMWindow::RemovedEventListener(
    const AtomicString& event_type,
    const RegisteredEventListener& registered_listener) {
  DOMWindow::RemovedEventListener(event_type, registered_listener);
  document()->DidRemoveEventListeners(/*count*/ 1);
  if (auto* frame = GetFrame()) {
    frame->GetEventHandlerRegistry().DidRemoveEventHandler(
        *this, event_type, registered_listener.Passive());
  }

  // Update sudden termination disabler state if we removed a listener for
  // unload/beforeunload/pagehide/visibilitychange.
  if (GetFrame() && IsSuddenTerminationDisablerEvent(event_type)) {
    GetFrame()->RemovedSuddenTerminationDisablerListener(*this, event_type);
  }
}

void LocalDOMWindow::DispatchLoadEvent() {
  Event& load_event = *Event::Create(event_type_names::kLoad);
  DocumentLoader* document_loader =
      GetFrame() ? GetFrame()->Loader().GetDocumentLoader() : nullptr;
  if (document_loader &&
      document_loader->GetTiming().LoadEventStart().is_null()) {
    DocumentLoadTiming& timing = document_loader->GetTiming();
    timing.MarkLoadEventStart();
    DispatchEvent(load_event, document());
    timing.MarkLoadEventEnd();
  } else {
    DispatchEvent(load_event, document());
  }

  if (LocalFrame* frame = GetFrame()) {
    WindowPerformance* performance = DOMWindowPerformance::performance(*this);
    DCHECK(performance);
    performance->NotifyNavigationTimingToObservers();

    // For load events, send a separate load event to the enclosing frame only.
    // This is a DOM extension and is independent of bubbling/capturing rules of
    // the DOM.
    if (FrameOwner* owner = frame->Owner()) {
      owner->DispatchLoad();
    }

    if (frame->IsAttached()) {
      // DEVTOOLS_TIMELINE_TRACE_EVENT_INSTANT(...) was here.
      probe::LoadEventFired(frame);
    }
  }
}

DispatchEventResult LocalDOMWindow::DispatchEvent(Event& event,
                                                  EventTarget* target) {
#if DCHECK_IS_ON()
  DCHECK(!EventDispatchForbiddenScope::IsEventDispatchForbidden());
#endif

  event.SetTrusted(true);
  event.SetTarget(target ? target : this);
  event.SetCurrentTarget(this);
  event.SetEventPhase(Event::PhaseType::kAtTarget);

  // DEVTOOLS_TIMELINE_TRACE_EVENT(...) was here.
  return FireEventListeners(event);
}

void LocalDOMWindow::RemoveAllEventListeners() {
  int previous_unload_handlers_count =
      NumberOfEventListeners(event_type_names::kUnload);
  int previous_before_unload_handlers_count =
      NumberOfEventListeners(event_type_names::kBeforeunload);
  int previous_page_hide_handlers_count =
      NumberOfEventListeners(event_type_names::kPagehide);
  int previous_visibility_change_handlers_count =
      NumberOfEventListeners(event_type_names::kVisibilitychange);
  if (document_ && HasEventListeners()) {
    GetEventTargetData()->event_listener_map.ForAllEventListenerTypes(
        [this](const AtomicString& event_type, uint32_t count) {
          document_->DidRemoveEventListeners(count);
        });
  }
  EventTarget::RemoveAllEventListeners();

  if (GetFrame()) {
    GetFrame()->GetEventHandlerRegistry().DidRemoveAllEventHandlers(*this);
  }

  // Update sudden termination disabler state if we previously have listeners
  // for unload/beforeunload/pagehide/visibilitychange.
  if (GetFrame() && previous_unload_handlers_count) {
    GetFrame()->RemovedSuddenTerminationDisablerListener(
        *this, event_type_names::kUnload);
  }
  if (GetFrame() && previous_before_unload_handlers_count) {
    GetFrame()->RemovedSuddenTerminationDisablerListener(
        *this, event_type_names::kBeforeunload);
  }
  if (GetFrame() && previous_page_hide_handlers_count) {
    GetFrame()->RemovedSuddenTerminationDisablerListener(
        *this, event_type_names::kPagehide);
  }
  if (GetFrame() && previous_visibility_change_handlers_count) {
    GetFrame()->RemovedSuddenTerminationDisablerListener(
        *this, event_type_names::kVisibilitychange);
  }
}

void LocalDOMWindow::FinishedLoading(FrameLoader::NavigationFinishState state) {
  if (RuntimeEnabledFeatures::NavigationSourcePseudoClassEnabled()) {
    NavigationState::AttemptFinishNavigationAndDestroy(document_);
  }
}

void LocalDOMWindow::PrintErrorMessage(const String& message) const {
  if (!IsCurrentlyDisplayedInFrame()) {
    return;
  }

  if (message.empty()) {
    return;
  }

  GetFrameConsole()->AddMessage(MakeGarbageCollected<ConsoleMessage>(
      mojom::ConsoleMessageSource::kJavaScript,
      mojom::ConsoleMessageLevel::kError, message));
}

void LocalDOMWindow::Trace(Visitor* visitor) const {
  visitor->Trace(document_);
  visitor->Trace(navigator_);
  visitor->Trace(custom_elements_);
  visitor->Trace(visualViewport_);
  visitor->Trace(trusted_types_);
  visitor->Trace(network_state_observer_);
  visitor->Trace(closewatcher_stack_);
  visitor->Trace(soft_navigation_heuristics_);
  UniversalGlobalScope::Trace(visitor);
  DOMWindow::Trace(visitor);
  ExecutionContext::Trace(visitor);
  Supplementable<LocalDOMWindow>::Trace(visitor);
}

bool LocalDOMWindow::CrossOriginIsolatedCapability() const {
  // When crossOriginIsolation is enabled by DocumentIsolationPolicy, it ignores
  // the restriction placed on COI capability by the CrossOriginIsolated
  // permission policy. This is because the permission policy is necessary for
  // defending against cross-origin iframes when COI is enabled by COOP + COEP.
  // But with DocumentIsolationPolicy, the cross-origin iframe is guaranteed to
  // be out-of-process, so there is no risk to it having COI capability.
  // Therefore, it is safe to ignore the permission policy in this case.
  // TODO(crbug.com/393522283): Ensure the COI status of a context is properly
  // computed in the browser process and just pass it instead of passing several
  // booleans to the renderer process and having it do the computation.
  bool permission_policy_allows_coi =
      IsFeatureEnabled(
          network::mojom::PermissionsPolicyFeature::kCrossOriginIsolated) ||
      GetPolicyContainer()->GetPolicies().cross_origin_isolation_enabled_by_dip;
  return GetAgent()->IsCrossOriginIsolated() && permission_policy_allows_coi;
}

bool LocalDOMWindow::IsIsolatedContext() const {
  return Agent::IsIsolatedContext();
}

ukm::UkmRecorder* LocalDOMWindow::UkmRecorder() {
  DCHECK(document_);
  return document_->UkmRecorder();
}

ukm::SourceId LocalDOMWindow::UkmSourceID() const {
  DCHECK(document_);
  return document_->UkmSourceID();
}

void LocalDOMWindow::SetStorageKey(const BlinkStorageKey& storage_key) {
  storage_key_ = storage_key;
}

void LocalDOMWindow::SetIsInBackForwardCache(bool is_in_back_forward_cache) {
  ExecutionContext::SetIsInBackForwardCache(is_in_back_forward_cache);
  if (!is_in_back_forward_cache) {
    BackForwardCacheBufferLimitTracker::Get()
        .DidRemoveFrameOrWorkerFromBackForwardCache(
            total_bytes_buffered_while_in_back_forward_cache_);
    total_bytes_buffered_while_in_back_forward_cache_ = 0;
  }
}

void LocalDOMWindow::DidBufferLoadWhileInBackForwardCache(
    bool update_process_wide_count,
    size_t num_bytes) {
  total_bytes_buffered_while_in_back_forward_cache_ += num_bytes;
  if (update_process_wide_count) {
    BackForwardCacheBufferLimitTracker::Get().DidBufferBytes(num_bytes);
  }
}

bool LocalDOMWindow::credentialless() const {
  return GetExecutionContext()
      ->GetPolicyContainer()
      ->GetPolicies()
      .is_credentialless;
}

bool LocalDOMWindow::IsInFencedFrame() const {
  return GetFrame() && GetFrame()->IsInFencedFrameTree();
}

net::StorageAccessApiStatus LocalDOMWindow::GetStorageAccessApiStatus() const {
  return storage_access_api_status_;
}

std::optional<mojom::blink::PolicyDisposition>
LocalDOMWindow::GetGuardrailsPolicyState() const {
  // Probe the policy lists to set disposition accordingly. IsFeatureEnabled
  // assumes a value of |false| is stricter than |true|, but that's reversed for
  // this configuration point.
  const DocumentPolicy* enforced_policy =
      GetSecurityContext().GetDocumentPolicy();
  bool has_enforced_policy =
      enforced_policy &&
      enforced_policy
          ->GetFeatureValue(
              mojom::blink::DocumentPolicyFeature::kNetworkEfficiencyGuardrails)
          .BoolValue();

  const DocumentPolicy* report_only_policy =
      GetSecurityContext().GetReportOnlyDocumentPolicy();
  bool has_report_only_policy =
      report_only_policy &&
      report_only_policy
          ->GetFeatureValue(
              mojom::blink::DocumentPolicyFeature::kNetworkEfficiencyGuardrails)
          .BoolValue();

  if (!has_enforced_policy && !has_report_only_policy) {
    return std::nullopt;
  }

  return has_enforced_policy ? mojom::blink::PolicyDisposition::kEnforce
                             : mojom::blink::PolicyDisposition::kReport;
}

bool LocalDOMWindow::CheckGuardrailsPolicyForAssetSize(
    GuardrailPolicyAssetType asset_type,
    size_t bytes,
    const KURL& url) const {
  String message;
  switch (asset_type) {
    case GuardrailPolicyAssetType::kData:
      if (bytes <= kGuardrailsLargeDataThresholdBytes) {
        return false;
      }
      message = "large data URLs are disallowed by policy";
      break;
    case GuardrailPolicyAssetType::kImage:
      if (bytes <= kGuardrailsLargeImageThresholdBytes) {
        return false;
      }
      message = "large media is disallowed by policy";
      break;
  }

  std::optional<mojom::blink::PolicyDisposition> disposition =
      GetGuardrailsPolicyState();
  if (disposition.has_value()) {
    ReportDocumentPolicyViolation(
        mojom::blink::DocumentPolicyFeature::kNetworkEfficiencyGuardrails,
        disposition.value(), message, url);
    return true;
  }

  return false;
}

void LocalDOMWindow::SetStorageAccessApiStatus(
    net::StorageAccessApiStatus status) {
  CHECK_GE(status, storage_access_api_status_);
  storage_access_api_status_ = status;
}

void LocalDOMWindow::UpdateEventListenerCountsToDocumentForReuseIfNeeded() {
  if (!is_dom_window_reused_) {
    return;
  }
  if (document_ && HasEventListeners()) {
    GetEventTargetData()->event_listener_map.ForAllEventListenerTypes(
        [this](const AtomicString& event_type, uint32_t count) {
          document_->AddListenerTypeIfNeeded(event_type, *this);
          document_->DidAddEventListeners(count);
        });
  }
  is_dom_window_reused_ = false;
}

}  // namespace blink
