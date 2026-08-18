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
#include "base/metrics/histogram_macros.h"
#include "base/task/single_thread_task_runner.h"
#include "base/trace_event/trace_id_helper.h"
#include "base/trace_event/typed_macros.h"
#include "build/build_config.h"
#include "cc/input/snap_selection_strategy.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/storage_access_api/status.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/switches.h"
#include "third_party/blink/public/mojom/devtools/inspector_issue.mojom-blink.h"
#include "third_party/blink/public/mojom/frame/frame.mojom-blink.h"
#include "third_party/blink/public/mojom/permissions_policy/policy_disposition.mojom-blink.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_picture_in_picture_window_options.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_scroll_to_options.h"
#include "third_party/blink/renderer/core/css/css_computed_style_declaration.h"
#include "third_party/blink/renderer/core/css/css_rule_list.h"
#include "third_party/blink/renderer/core/css/dom_window_css.h"
#include "third_party/blink/renderer/core/css/media_query_list.h"
#include "third_party/blink/renderer/core/css/media_query_matcher.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver.h"
#include "third_party/blink/renderer/core/css/style_media.h"
#include "third_party/blink/renderer/core/display_lock/display_lock_document_state.h"
#include "third_party/blink/renderer/core/dom/document_init.h"
#include "third_party/blink/renderer/core/dom/events/add_event_listener_options_resolved.h"
#include "third_party/blink/renderer/core/dom/events/event_dispatch_forbidden_scope.h"
#include "third_party/blink/renderer/core/dom/events/scoped_event_queue.h"
#include "third_party/blink/renderer/core/dom/frame_request_callback_collection.h"
#include "third_party/blink/renderer/core/dom/scriptable_document_parser.h"
#include "third_party/blink/renderer/core/editing/editor.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/ime/input_method_controller.h"
#include "third_party/blink/renderer/core/editing/spellcheck/spell_checker.h"
#include "third_party/blink/renderer/core/editing/suggestion/text_suggestion_controller.h"
#include "third_party/blink/renderer/core/events/error_event.h"
#include "third_party/blink/renderer/core/events/hash_change_event.h"
#include "third_party/blink/renderer/core/events/message_event.h"
#include "third_party/blink/renderer/core/events/page_transition_event.h"
#include "third_party/blink/renderer/core/events/pop_state_event.h"
#include "third_party/blink/renderer/core/execution_context/execution_context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/execution_context/window_agent.h"
#include "third_party/blink/renderer/core/frame/bar_prop.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/document_policy_violation_report_body.h"
#include "third_party/blink/renderer/core/frame/dom_viewport.h"
#include "third_party/blink/renderer/core/frame/dom_visual_viewport.h"
#include "third_party/blink/renderer/core/frame/event_handler_registry.h"
#include "third_party/blink/renderer/core/frame/external.h"
#include "third_party/blink/renderer/core/frame/frame_console.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/core/frame/permissions_policy_violation_report_body.h"
#include "third_party/blink/renderer/core/frame/report.h"
#include "third_party/blink/renderer/core/frame/reporting_context.h"
#include "third_party/blink/renderer/core/frame/screen.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/viewport_data.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_registry.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_registry_assignment.h"
#include "third_party/blink/renderer/core/html/forms/form_controller.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"
#include "third_party/blink/renderer/core/html/plugin_document.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/layout/adjust_for_absolute_zoom.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/frame_load_request.h"
#include "third_party/blink/renderer/core/origin_trials/origin_trial_context.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/create_window.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/page/scrolling/scrolling_coordinator.h"
#include "third_party/blink/renderer/core/page/scrolling/sync_scroll_attempt_heuristic.h"
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
#include "third_party/blink/renderer/core/trustedtypes/trusted_types_util.h"
#include "third_party/blink/renderer/core/view_transition/view_transition_supplement.h"
#include "third_party/blink/renderer/platform/back_forward_cache_buffer_limit_tracker.h"
#include "third_party/blink/renderer/platform/bindings/exception_messages.h"
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
#include "third_party/blink/renderer/platform/scheduler/public/task_attribution_info.h"
#include "third_party/blink/renderer/platform/scheduler/public/task_attribution_tracker.h"
#include "third_party/blink/renderer/platform/storage/blink_storage_key.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/widget/frame_widget.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_copier_std.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/wtf/text/strcat.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/uuid.h"
#include "ui/display/screen_info.h"

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
      ExecutionContext(agent,
                       /*Same value as IsWindow(). is_window=*/true),
      viewport_(MakeGarbageCollected<DOMViewport>(this)),
      visualViewport_(MakeGarbageCollected<DOMVisualViewport>(this)),
      should_print_when_finished_loading_(false),
      input_method_controller_(
          MakeGarbageCollected<InputMethodController>(*this, frame)),
      spell_checker_(MakeGarbageCollected<SpellChecker>(*this)),
      text_suggestion_controller_(
          MakeGarbageCollected<TextSuggestionController>(*this)),
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
  GetAgent()->AttachContext(this);
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
  document_ = nullptr;

  // Reset per-document metrics bookkeeping.
  if (soft_navigation_heuristics_) {
    soft_navigation_heuristics_->Shutdown();
    soft_navigation_heuristics_ = nullptr;
  }
  WindowPerformance::ClearForWindowReuse(*this);
}

void LocalDOMWindow::ResetWindowAgent(WindowAgent* agent) {
  GetAgent()->DetachContext(this);
  ResetAgent(agent);
  if (document_) {
    document_->ResetAgent(*agent);
  }

  CHECK(GetFrame());
  GetFrame()->GetFrameScheduler()->SetAgentClusterId(GetAgentClusterID());

  GetAgent()->AttachContext(this);
}

void LocalDOMWindow::AcceptLanguagesChanged() {
  if (navigator_) {
    navigator_->SetLanguagesDirty();
  }

  DispatchEvent(*Event::Create(event_type_names::kLanguagechange));
}

Event* LocalDOMWindow::CurrentEvent() const {
  return current_event_.Get();
}

void LocalDOMWindow::SetCurrentEvent(Event* new_event) {
  current_event_ = new_event;
}

TrustedTypePolicyFactory* LocalDOMWindow::GetTrustedTypes() const {
  DCHECK(IsMainThread());
  if (!trusted_types_) {
    trusted_types_ =
        MakeGarbageCollected<TrustedTypePolicyFactory>(GetExecutionContext());
  }
  return trusted_types_.Get();
}

bool LocalDOMWindow::IsCrossSiteSubframe() const {
  if (!GetFrame()) {
    return false;
  }
  if (GetFrame()->IsInFencedFrameTree()) {
    return true;
  }
  // It'd be nice to avoid the url::Origin temporaries, but that would require
  // exposing the net internal helper.
  // TODO: If the helper gets exposed, we could do this without any new
  // allocations using StringUtf8Adaptor.
  auto* top_origin =
      GetFrame()->Tree().Top().GetSecurityContext()->GetSecurityOrigin();
  return !net::registry_controlled_domains::SameDomainOrHost(
      top_origin->ToUrlOrigin(), GetSecurityOrigin()->ToUrlOrigin(),
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

bool LocalDOMWindow::IsCrossSiteSubframeIncludingScheme() const {
  if (!GetFrame()) {
    return false;
  }
  if (GetFrame()->IsInFencedFrameTree()) {
    return true;
  }
  return top()->GetFrame() &&
         !top()
              ->GetFrame()
              ->GetSecurityContext()
              ->GetSecurityOrigin()
              ->IsSameSiteWith(GetSecurityContext().GetSecurityOrigin());
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
  // This is basically a version of CheckAndGetJavascriptUrl, but where the
  // caller does not care about the actual string value, but only about the
  // error conditions. In this case, multiple checks just drop out.

  // AllowInline below will check the source's hash against CSP, which is why
  // it needs an exact script_source.
  String decoded_url = DecodeUrlEscapeSequences(
      url.GetString(), DecodeUrlMode::kUtf8OrIsomorphic);

  // Check the CSP of the caller (the "source browsing context") if required,
  // as per https://html.spec.whatwg.org/C/#javascript-protocol.
  return GetContentSecurityPolicy()->AllowInline(
      ContentSecurityPolicy::InlineType::kNavigation, element, decoded_url,
      String() /* nonce */, Url(), OrdinalNumber::First());
}

String LocalDOMWindow::CheckAndGetJavascriptUrl(
    const KURL& url,
    Element* element,
    network::mojom::CSPDisposition csp_disposition) {
  const int kJavascriptSchemeLength = sizeof("javascript:") - 1;
  String decoded_url = DecodeUrlEscapeSequences(
      url.GetString(), DecodeUrlMode::kUtf8OrIsomorphic);
  String script_source =
      decoded_url.DeprecatedSubstring(kJavascriptSchemeLength);

  if (csp_disposition == network::mojom::CSPDisposition::DO_NOT_CHECK) {
    return script_source;
  }

  // Check the CSP of the caller (the "source browsing context") if required,
  // as per https://html.spec.whatwg.org/C/#javascript-protocol.
  if (!GetContentSecurityPolicy()->AllowInline(
          ContentSecurityPolicy::InlineType::kNavigation, element, decoded_url,
          String() /* nonce */, Url(), OrdinalNumber::First())) {
    return String();
  }

  // Sanity check: If we're here, AllowInlineJavascriptUrl would have also
  // allowed this URL to proceed.
  DCHECK(AllowInlineJavascriptUrl(url, element));

  // https://w3c.github.io/trusted-types/dist/spec/#require-trusted-types-for-pre-navigation-check
  // 4.9.1.1. require-trusted-types-for Pre-Navigation check
  script_source =
      TrustedTypesCheckForJavascriptURLinNavigation(script_source, this);

  return script_source;
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
    const base::UnguessableToken& initiator_state_token) {
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
  if (disposition == mojom::blink::PolicyDisposition::kEnforce) {
    const_cast<LocalDOMWindow*>(this)->CountPermissionsPolicyUsage(
        feature, UseCounterImpl::PermissionsPolicyUsageType::kViolation);
  }

  if (!GetFrame()) {
    return;
  }

  // Construct the permissions policy violation report.
  bool is_isolated_context =
      GetExecutionContext() && GetExecutionContext()->IsIsolatedContext();
  const String& feature_name = GetNameForFeature(feature, is_isolated_context);
  const String& disp_str =
      (disposition == mojom::blink::PolicyDisposition::kReport ? "report"
                                                               : "enforce");

  PermissionsPolicyViolationReportBody* body =
      MakeGarbageCollected<PermissionsPolicyViolationReportBody>(
          feature_name, message, disp_str);

  Report* report = MakeGarbageCollected<Report>(
      ReportType::kPermissionsPolicyViolation, Url().GetString(), body);

  // Send the permissions policy violation report to the specified endpoint,
  // if one exists, as well as any ReportingObservers.
  if (!reporting_endpoint.empty()) {
    ReportingContext::From(this)->QueueReport(report, {reporting_endpoint});
  } else {
    ReportingContext::From(this)->QueueReport(report);
  }

  // TODO(iclelland): Report something different in report-only mode
  if (disposition == mojom::blink::PolicyDisposition::kEnforce) {
    GetFrame()->Console().AddMessage(MakeGarbageCollected<ConsoleMessage>(
        mojom::blink::ConsoleMessageSource::kViolation,
        mojom::blink::ConsoleMessageLevel::kError, body->message()));
  }
}

void LocalDOMWindow::ReportPotentialPermissionsPolicyViolation(
    network::mojom::PermissionsPolicyFeature feature,
    mojom::blink::PolicyDisposition disposition,
    const String& reporting_endpoint,
    const String& message,
    const String& allow_attribute,
    const String& src_attribute) const {
  CHECK(GetFrame());

  // Construct the potential permissions policy violation report.
  bool is_isolated_context =
      GetExecutionContext() && GetExecutionContext()->IsIsolatedContext();
  const String& feature_name = GetNameForFeature(feature, is_isolated_context);
  const String& disp_str =
      (disposition == mojom::blink::PolicyDisposition::kReport ? "report"
                                                               : "enforce");

  PermissionsPolicyViolationReportBody* body =
      MakeGarbageCollected<PermissionsPolicyViolationReportBody>(
          feature_name, message, disp_str, allow_attribute, src_attribute);

  Report* report = MakeGarbageCollected<Report>(
      ReportType::kPotentialPermissionsPolicyViolation, Url().GetString(),
      body);

  // Send the potential permissions policy violation report to the specified
  // endpoint if one exists, as well as any ReportingObservers.
  if (!reporting_endpoint.empty()) {
    ReportingContext::From(this)->QueueReport(report, {reporting_endpoint});
  } else {
    ReportingContext::From(this)->QueueReport(report);
  }

  if (disposition == mojom::blink::PolicyDisposition::kEnforce &&
      !reporting_endpoint.empty()) {
    GetFrame()->Console().AddMessage(MakeGarbageCollected<ConsoleMessage>(
        mojom::blink::ConsoleMessageSource::kViolation,
        mojom::blink::ConsoleMessageLevel::kError, body->message()));
  }
}

void LocalDOMWindow::ReportDocumentPolicyViolation(
    mojom::blink::DocumentPolicyFeature feature,
    mojom::blink::PolicyDisposition disposition,
    const String& message,
    const String& source_file) const {
  if (!GetFrame()) {
    return;
  }

  // Construct the document policy violation report.
  String feature_name(
      GetDocumentPolicyFeatureInfoMap().at(feature).feature_name);
  bool is_report_only = disposition == mojom::blink::PolicyDisposition::kReport;
  const String& disp_str = is_report_only ? "report" : "enforce";
  const DocumentPolicy* relevant_document_policy =
      is_report_only ? GetSecurityContext().GetReportOnlyDocumentPolicy()
                     : GetSecurityContext().GetDocumentPolicy();

  DocumentPolicyViolationReportBody* body =
      MakeGarbageCollected<DocumentPolicyViolationReportBody>(
          feature_name, message, disp_str, source_file);

  Report* report = MakeGarbageCollected<Report>(
      ReportType::kDocumentPolicyViolation, Url().GetString(), body);

  // Avoids sending duplicate reports, by comparing the generated MatchId.
  // The match ids are not guaranteed to be unique.
  // There are trade offs on storing full objects and storing match ids. Storing
  // full objects takes more memory. Storing match id has the potential of hash
  // collision. Since reporting is not a part critical system or have security
  // concern, dropping a valid report due to hash collision seems a reasonable
  // price to pay for the memory saving.
  unsigned report_id = report->MatchId();
  DCHECK(report_id);

  if (document_policy_violation_reports_sent_.Contains(report_id)) {
    return;
  }
  document_policy_violation_reports_sent_.insert(report_id);

  // Send the document policy violation report to any ReportingObservers.
  const std::optional<std::string> endpoint =
      relevant_document_policy->GetFeatureEndpoint(feature);

  if (is_report_only) {
    UMA_HISTOGRAM_ENUMERATION("Blink.UseCounter.DocumentPolicy.ReportOnly",
                              feature);
  } else {
    UMA_HISTOGRAM_ENUMERATION("Blink.UseCounter.DocumentPolicy.Enforced",
                              feature);
  }

  ReportingContext::From(this)->QueueReport(
      report, endpoint ? Vector<String>{endpoint->c_str()} : Vector<String>{});

  // TODO(iclelland): Report something different in report-only mode
  if (!is_report_only) {
    GetFrame()->Console().AddMessage(MakeGarbageCollected<ConsoleMessage>(
        mojom::blink::ConsoleMessageSource::kViolation,
        mojom::blink::ConsoleMessageLevel::kError, body->message()));
  }
}

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

void LocalDOMWindow::CountUse(mojom::WebFeature feature) {
  if (!GetFrame()) {
    return;
  }
  if (auto* loader = GetFrame()->Loader().GetDocumentLoader()) {
    loader->CountUse(feature);
  }
}

void LocalDOMWindow::CountWebDXFeature(mojom::blink::WebDXFeature feature) {
  if (!GetFrame()) {
    return;
  }
  if (auto* loader = GetFrame()->Loader().GetDocumentLoader()) {
    loader->CountWebDXFeature(feature);
  }
}

void LocalDOMWindow::CountPermissionsPolicyUsage(
    network::mojom::PermissionsPolicyFeature feature,
    UseCounterImpl::PermissionsPolicyUsageType type) {
  if (!GetFrame()) {
    return;
  }
  if (auto* loader = GetFrame()->Loader().GetDocumentLoader()) {
    loader->GetUseCounter().CountPermissionsPolicyUsage(feature, type,
                                                        *GetFrame());
  }
}

void LocalDOMWindow::CountUseOnlyInCrossOriginIframe(
    mojom::blink::WebFeature feature) {
  if (GetFrame() && GetFrame()->IsCrossOriginToOutermostMainFrame()) {
    CountUse(feature);
  }
}

void LocalDOMWindow::CountUseOnlyInSameOriginIframe(
    mojom::blink::WebFeature feature) {
  if (GetFrame() && !GetFrame()->IsOutermostMainFrame() &&
      !GetFrame()->IsCrossOriginToOutermostMainFrame()) {
    CountUse(feature);
  }
}

void LocalDOMWindow::CountUseOnlyInCrossSiteIframe(
    mojom::blink::WebFeature feature) {
  if (IsCrossSiteSubframeIncludingScheme()) {
    CountUse(feature);
  }
}

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
    document_->GetFormController().RestoreImmediately();
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

void LocalDOMWindow::DispatchPersistedPageshowEvent(
    base::TimeTicks navigation_start) {
  // Persisted pageshow events are dispatched for pages that are restored from
  // the back forward cache, and the event's timestamp should reflect the
  // |navigation_start| time of the back navigation.
  DispatchEvent(*PageTransitionEvent::CreatePersistedPageshow(navigation_start),
                document_.Get());
}

void LocalDOMWindow::DispatchPagehideEvent(
    PageTransitionEventPersistence persistence) {
  if (document_->UnloadStarted()) {
    // We've already dispatched pagehide (since it's the first thing we do when
    // starting unload) and shouldn't dispatch it again. We might get here on
    // a document that is already unloading/has unloaded but still part of the
    // FrameTree.
    // TODO(crbug.com/1119291): Investigate whether this is possible or not.
    return;
  }

  if (RuntimeEnabledFeatures::NavigationStateEnabled()) {
    // In case we come back to this document later via BFCache, there must not
    // be a dangling active navigation.
    NavigationState::AttemptFinishNavigationAndDestroy(document_);
  }

  // The navigation that triggered this pagehide is past the point of being
  // canceled (beforeunload has run without canceling). Promote any pending
  // navigationDestinationURL stashed during the navigate event so that JS
  // pagehide handlers can observe it via performance.getSpeculations().
  DOMWindowPerformance::performance(*this)->PromoteNavigationDestinationURL();

  DispatchEvent(
      *PageTransitionEvent::Create(event_type_names::kPagehide, persistence),
      document_.Get());
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

MediaQueryList* LocalDOMWindow::matchMedia(const String& media) {
  return document()->GetMediaQueryMatcher().MatchMedia(media);
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
  GetAgent()->DetachContext(this);
  NotifyContextDestroyed();
  RemoveAllEventListeners();
  DisconnectFromFrame();
}

void LocalDOMWindow::RegisterEventListenerObserver(
    EventListenerObserver* event_listener_observer) {
  event_listener_observers_.insert(event_listener_observer);
}

void LocalDOMWindow::Reset() {
  DCHECK(document());
  FrameDestroyed();

  screen_ = nullptr;
  locationbar_ = nullptr;
  menubar_ = nullptr;
  personalbar_ = nullptr;
  scrollbars_ = nullptr;
  statusbar_ = nullptr;
  toolbar_ = nullptr;
  navigator_ = nullptr;
  media_ = nullptr;
  custom_elements_ = nullptr;
  trusted_types_ = nullptr;
}

void LocalDOMWindow::SendOrientationChangeEvent() {
  DCHECK(RuntimeEnabledFeatures::OrientationEventEnabled());
  DCHECK(GetFrame()->IsLocalRoot());

  // Before dispatching the event, build a list of all frames in the page
  // to send the event to, to mitigate side effects from event handlers
  // potentially interfering with others.
  HeapVector<Member<LocalFrame>> frames;
  frames.push_back(GetFrame());
  for (wtf_size_t i = 0; i < frames.size(); i++) {
    for (Frame* child = frames[i]->Tree().FirstChild(); child;
         child = child->Tree().NextSibling()) {
      if (auto* child_local_frame = DynamicTo<LocalFrame>(child)) {
        frames.push_back(child_local_frame);
      }
    }
  }

  for (LocalFrame* frame : frames) {
    frame->DomWindow()->DispatchEvent(
        *Event::Create(event_type_names::kOrientationchange));
  }
}

int LocalDOMWindow::orientation() const {
  LocalFrame* frame = GetFrame();
  if (!frame) {
    return 0;
  }

  ChromeClient& chrome_client = frame->GetChromeClient();
  int orientation = chrome_client.GetScreenInfo(*frame).orientation_angle;
  // For backward compatibility, we want to return a value in the range of
  // [-90; 180] instead of [0; 360[ because window.orientation used to behave
  // like that in WebKit (this is a WebKit proprietary API).
  if (orientation == 270) {
    return -90;
  }
  return orientation;
}

Screen* LocalDOMWindow::screen() {
  if (!screen_) {
    LocalFrame* frame = GetFrame();
    int64_t display_id =
        frame ? frame->GetChromeClient().GetScreenInfo(*frame).display_id
              : Screen::kInvalidDisplayId;
    screen_ = MakeGarbageCollected<Screen>(this, display_id);
  }
  return screen_.Get();
}

BarProp* LocalDOMWindow::locationbar() {
  if (!locationbar_) {
    locationbar_ = MakeGarbageCollected<BarProp>(this);
  }
  return locationbar_.Get();
}

BarProp* LocalDOMWindow::menubar() {
  if (!menubar_) {
    menubar_ = MakeGarbageCollected<BarProp>(this);
  }
  return menubar_.Get();
}

BarProp* LocalDOMWindow::personalbar() {
  if (!personalbar_) {
    personalbar_ = MakeGarbageCollected<BarProp>(this);
  }
  return personalbar_.Get();
}

BarProp* LocalDOMWindow::scrollbars() {
  if (!scrollbars_) {
    scrollbars_ = MakeGarbageCollected<BarProp>(this);
  }
  return scrollbars_.Get();
}

BarProp* LocalDOMWindow::statusbar() {
  if (!statusbar_) {
    statusbar_ = MakeGarbageCollected<BarProp>(this);
  }
  return statusbar_.Get();
}

BarProp* LocalDOMWindow::toolbar() {
  if (!toolbar_) {
    toolbar_ = MakeGarbageCollected<BarProp>(this);
  }
  return toolbar_.Get();
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

// SchedulePostMessage() was here. PostedMessage carried a
// SerializedScriptValue -- the structured clone of a script value -- and went
// with DOMWindow's declaration of it. postMessage() has no caller without a
// script engine.

void LocalDOMWindow::DispatchPostMessage(
    MessageEvent* event,
    scoped_refptr<const SecurityOrigin> intended_target_origin,
    SourceLocation* location,
    const base::UnguessableToken& source_agent_cluster_id,
    scheduler::TaskAttributionInfo* task_state) {
  probe::AsyncTask async_task(this, event->async_task_context());
  if (!IsCurrentlyDisplayedInFrame()) {
    return;
  }

  // Used to entangle the event's disentangled MessageChannel ports into
  // live MessagePorts in this window's context; MessagePort support was
  // removed from MessageEvent (see message_event.h), so there is nothing
  // left to entangle.

  TRACE_EVENT("devtools.timeline", "HandlePostMessage",
              perfetto::Flow::Global(event->GetTraceId()));

  std::optional<scheduler::TaskAttributionTracker::TaskScope> task_scope(
      SetCurrentTaskStateIfTopLevel(task_state, this,
                                    TaskScopeType::kPostMessage));

  DispatchMessageEventWithOriginCheck(intended_target_origin.get(), event,
                                      location, source_agent_cluster_id);
}

void LocalDOMWindow::DispatchMessageEventWithOriginCheck(
    const SecurityOrigin* intended_target_origin,
    MessageEvent* event,
    SourceLocation* location,
    const base::UnguessableToken& source_agent_cluster_id) {
  TRACE_EVENT0("blink", "LocalDOMWindow::DispatchMessageEventWithOriginCheck");
  if (intended_target_origin) {
    bool valid_target =
        intended_target_origin->IsSameOriginWith(GetSecurityOrigin());

    if (!valid_target) {
      String message = ExceptionMessages::FailedToExecute(
          "postMessage", "DOMWindow",
          StrCat({"The target origin provided ('",
                  intended_target_origin->ToString(),
                  "') does not match the recipient window's origin ('",
                  GetSecurityOrigin()->ToString(), "')."}));
      auto* console_message = MakeGarbageCollected<ConsoleMessage>(
          mojom::ConsoleMessageSource::kSecurity,
          mojom::ConsoleMessageLevel::kWarning, message, location);
      GetFrameConsole()->AddMessage(console_message);
      return;
    }
  }

  scoped_refptr<const SecurityOrigin> sender_origin =
      event->GetSecurityOrigin();
  if (event->IsOriginCheckRequiredToAccessData()) {
    if (!sender_origin->IsSameOriginWith(GetSecurityOrigin())) {
      event = MessageEvent::CreateError(event);
    }
  }
  if (event->IsLockedToAgentCluster()) {
    if (!IsSameAgentCluster(source_agent_cluster_id)) {
      UseCounter::Count(
          this,
          WebFeature::kMessageEventSharedArrayBufferDifferentAgentCluster);
      event = MessageEvent::CreateError(event);
    } else {
      if (!sender_origin->IsSameOriginWith(GetSecurityOrigin())) {
        UseCounter::Count(
            this, WebFeature::kMessageEventSharedArrayBufferSameAgentCluster);
      } else {
        UseCounter::Count(this,
                          WebFeature::kMessageEventSharedArrayBufferSameOrigin);
      }
    }
  }

  if (!event->CanDeserializeIn(this)) {
    event = MessageEvent::CreateError(event);
  }

  // The delegatedCapability() checks that used to Activate() the
  // payment/fullscreen/display-capture/digital-credentials request tokens
  // here were removed along with MessageEvent::delegatedCapability() (see
  // message_event.h): capability delegation over postMessage no longer has
  // a signal to read, so there is nothing left to activate from this
  // event. The tokens themselves (and their consumers elsewhere) are
  // untouched -- they just never get activated via postMessage anymore.

  // MessageEvent::SetShouldMeasureDataAccessBeforeOrigin() was removed
  // along with the metric it fed (see message_event.h).

  if (GetFrame() &&
      GetFrame()->GetPage()->GetPageScheduler()->IsInBackForwardCache()) {
    // Enqueue the event when the page is in back/forward cache, so that it
    // would not cause JavaScript execution. The event will be dispatched upon
    // restore.
    EnqueueEvent(*event, TaskType::kInternalDefault);
  } else {
    DispatchEvent(*event);
  }
}

DomSelection* LocalDOMWindow::getSelection() {
  if (!IsCurrentlyDisplayedInFrame()) {
    return nullptr;
  }

  return document()->GetSelection();
}

Element* LocalDOMWindow::frameElement() const {
  if (!GetFrame()) {
    return nullptr;
  }

  return DynamicTo<HTMLFrameOwnerElement>(GetFrame()->Owner());
}

void LocalDOMWindow::print() {
  // Don't try to print if there's no frame attached anymore.
  if (!GetFrame()) {
    return;
  }

  if (GetFrame()->IsLoading()) {
    should_print_when_finished_loading_ = true;
    return;
  }

  CountUseOnlyInSameOriginIframe(WebFeature::kSameOriginIframeWindowPrint);
  CountUseOnlyInCrossOriginIframe(WebFeature::kCrossOriginWindowPrint);

  should_print_when_finished_loading_ = false;
  GetFrame()->GetPage()->GetChromeClient().Print(GetFrame());
}

void LocalDOMWindow::stop() {
  if (!GetFrame()) {
    return;
  }
  GetFrame()->Loader().StopAllLoaders(/*abort_client=*/true);
}

bool LocalDOMWindow::find(const String& string,
                          bool case_sensitive,
                          bool backwards,
                          bool wrap,
                          bool whole_word,
                          bool /*searchInFrames*/,
                          bool /*showDialog*/) const {
  auto forced_activatable_locks = document()
                                      ->GetDisplayLockDocumentState()
                                      .GetScopedForceActivatableLocks();

  if (!IsCurrentlyDisplayedInFrame()) {
    return false;
  }

  // Up-to-date, clean tree is required for finding text in page, since it
  // relies on TextIterator to look over the text.
  document()->UpdateStyleAndLayout(DocumentUpdateReason::kJavaScript);

  // FIXME (13016): Support searchInFrames and showDialog
  FindOptions options = FindOptions()
                            .SetBackwards(backwards)
                            .SetCaseInsensitive(!case_sensitive)
                            .SetWrappingAround(wrap)
                            .SetWholeWord(whole_word);
  return Editor::FindString(*GetFrame(), string, options);
}

bool LocalDOMWindow::offscreenBuffering() const {
  return true;
}

int LocalDOMWindow::outerHeight() const {
  if (!GetFrame()) {
    return 0;
  }

  LocalFrame* frame = GetFrame();

  // FencedFrames should return innerHeight to prevent passing
  // arbitrary data through the window height.
  if (frame->IsInFencedFrameTree()) {
    return innerHeight();
  }

  Page* page = frame->GetPage();
  if (!page) {
    return 0;
  }

  ChromeClient& chrome_client = page->GetChromeClient();
  if (page->GetSettings().GetReportScreenSizeInPhysicalPixelsQuirk()) {
    return static_cast<int>(
        lroundf(chrome_client.RootWindowRect(*frame).height() *
                chrome_client.GetScreenInfo(*frame).device_scale_factor));
  }
  int height = chrome_client.RootWindowRect(*frame).height();
  if (document() && document()->TextScaleMetaTagPresent()) {
    height = static_cast<int>(lroundf(
        height * chrome_client.GetScreenInfo(*frame).text_scale_multiplier));
  }
  return height;
}

int LocalDOMWindow::outerWidth() const {
  if (!GetFrame()) {
    return 0;
  }

  LocalFrame* frame = GetFrame();

  // FencedFrames should return innerWidth to prevent passing
  // arbitrary data through the window width.
  if (frame->IsInFencedFrameTree()) {
    return innerWidth();
  }

  Page* page = frame->GetPage();
  if (!page) {
    return 0;
  }

  ChromeClient& chrome_client = page->GetChromeClient();
  if (page->GetSettings().GetReportScreenSizeInPhysicalPixelsQuirk()) {
    return static_cast<int>(
        lroundf(chrome_client.RootWindowRect(*frame).width() *
                chrome_client.GetScreenInfo(*frame).device_scale_factor));
  }
  int width = chrome_client.RootWindowRect(*frame).width();
  if (document() && document()->TextScaleMetaTagPresent()) {
    width = static_cast<int>(lroundf(
        width * chrome_client.GetScreenInfo(*frame).text_scale_multiplier));
  }
  return width;
}

gfx::Size LocalDOMWindow::GetViewportSize() const {
  LocalFrameView* view = GetFrame()->View();
  if (!view) {
    return gfx::Size();
  }

  Page* page = GetFrame()->GetPage();
  if (!page) {
    return gfx::Size();
  }

  // The main frame's viewport size depends on the page scale. If viewport is
  // enabled, the initial page scale depends on the content width and is set
  // after a layout, perform one now so queries during page load will use the
  // up to date viewport. Also, a main frame needs at least one layout to set
  // its initial size.
  if (GetFrame()->IsMainFrame() &&
      (page->GetSettings().GetViewportEnabled() || !view->DidFirstLayout())) {
    document()->UpdateStyleAndLayout(DocumentUpdateReason::kJavaScript);
  }

  // FIXME: This is potentially too much work. We really only need to know the
  // dimensions of the parent frame's layoutObject.
  if (Frame* parent = GetFrame()->Tree().Parent()) {
    if (auto* parent_local_frame = DynamicTo<LocalFrame>(parent)) {
      parent_local_frame->GetDocument()->UpdateStyleAndLayout(
          DocumentUpdateReason::kJavaScript);
    }
  }

  return document()->View()->Size();
}

int LocalDOMWindow::innerHeight() const {
  if (!GetFrame()) {
    return 0;
  }

  return AdjustForAbsoluteZoom::AdjustInt(GetViewportSize().height(),
                                          GetFrame()->LayoutZoomFactor());
}

int LocalDOMWindow::innerWidth() const {
  if (!GetFrame()) {
    return 0;
  }

  return AdjustForAbsoluteZoom::AdjustInt(GetViewportSize().width(),
                                          GetFrame()->LayoutZoomFactor());
}

int LocalDOMWindow::screenX() const {
  LocalFrame* frame = GetFrame();
  if (!frame) {
    return 0;
  }

  Page* page = frame->GetPage();
  if (!page) {
    return 0;
  }

  ChromeClient& chrome_client = page->GetChromeClient();
  if (page->GetSettings().GetReportScreenSizeInPhysicalPixelsQuirk()) {
    return static_cast<int>(
        lroundf(chrome_client.RootWindowRect(*frame).x() *
                chrome_client.GetScreenInfo(*frame).device_scale_factor));
  }
  int screenX = chrome_client.RootWindowRect(*frame).x();
  if (document() && document()->TextScaleMetaTagPresent()) {
    screenX = static_cast<int>(lroundf(
        screenX * chrome_client.GetScreenInfo(*frame).text_scale_multiplier));
  }
  return screenX;
}

int LocalDOMWindow::screenY() const {
  LocalFrame* frame = GetFrame();
  if (!frame) {
    return 0;
  }

  Page* page = frame->GetPage();
  if (!page) {
    return 0;
  }

  ChromeClient& chrome_client = page->GetChromeClient();
  if (page->GetSettings().GetReportScreenSizeInPhysicalPixelsQuirk()) {
    return static_cast<int>(
        lroundf(chrome_client.RootWindowRect(*frame).y() *
                chrome_client.GetScreenInfo(*frame).device_scale_factor));
  }
  int screenY = chrome_client.RootWindowRect(*frame).y();
  if (document() && document()->TextScaleMetaTagPresent()) {
    screenY = static_cast<int>(lroundf(
        screenY * chrome_client.GetScreenInfo(*frame).text_scale_multiplier));
  }
  return screenY;
}

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
  SyncScrollAttemptHeuristic::DidAccessScrollOffset();

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
  SyncScrollAttemptHeuristic::DidAccessScrollOffset();

  document()->UpdateStyleAndLayout(DocumentUpdateReason::kJavaScript);

  // TODO(bokan): This is wrong when the document.rootScroller is non-default.
  // crbug.com/505516.
  double viewport_y = view->LayoutViewport()->GetWebExposedScrollOffset().y();
  return AdjustForAbsoluteZoom::AdjustScroll(viewport_y,
                                             GetFrame()->LayoutZoomFactor());
}

DOMViewport* LocalDOMWindow::viewport() {
  return viewport_.Get();
}

DOMVisualViewport* LocalDOMWindow::visualViewport() {
  return visualViewport_.Get();
}

const AtomicString& LocalDOMWindow::name() const {
  if (!IsCurrentlyDisplayedInFrame()) {
    return g_null_atom;
  }

  return GetFrame()->Tree().GetName();
}

void LocalDOMWindow::setName(const AtomicString& name) {
  if (!IsCurrentlyDisplayedInFrame()) {
    return;
  }

  GetFrame()->Tree().SetName(name, FrameTree::kReplicate);
}

void LocalDOMWindow::setStatus(const String& string) {
  status_ = string;
}

void LocalDOMWindow::setDefaultStatus(const String& string) {
  DCHECK(RuntimeEnabledFeatures::WindowDefaultStatusEnabled());
  default_status_ = string;
}

String LocalDOMWindow::origin() const {
  return GetSecurityOrigin()->ToString();
}

Document* LocalDOMWindow::document() const {
  return document_.Get();
}

StyleMedia* LocalDOMWindow::styleMedia() {
  if (!media_) {
    media_ = MakeGarbageCollected<StyleMedia>(this);
  }
  return media_.Get();
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

void LocalDOMWindow::scrollBy(double x, double y) const {
  ScrollToOptions* options = ScrollToOptions::Create();
  options->setLeft(x);
  options->setTop(y);
  scrollBy(options);
}

void LocalDOMWindow::scrollBy(const ScrollToOptions* scroll_to_options) const {
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
  SyncScrollAttemptHeuristic::DidSetScrollOffset();

  document()->UpdateStyleAndLayout(DocumentUpdateReason::kJavaScript);

  float x = 0.0f;
  float y = 0.0f;
  if (scroll_to_options->hasLeft()) {
    x = ScrollableArea::NormalizeNonFiniteScroll(
        base::saturated_cast<float>(scroll_to_options->left()));
  }
  if (scroll_to_options->hasTop()) {
    y = ScrollableArea::NormalizeNonFiniteScroll(
        base::saturated_cast<float>(scroll_to_options->top()));
  }

  PaintLayerScrollableArea* viewport = view->LayoutViewport();
  gfx::PointF current_position = viewport->ScrollPosition();
  gfx::Vector2dF scaled_delta(x * GetFrame()->LayoutZoomFactor(),
                              y * GetFrame()->LayoutZoomFactor());
  gfx::PointF new_scaled_position = current_position + scaled_delta;

  std::unique_ptr<cc::SnapSelectionStrategy> strategy =
      cc::SnapSelectionStrategy::CreateForDisplacement(
          current_position, scaled_delta,
          RuntimeEnabledFeatures::FractionalScrollOffsetsEnabled());
  new_scaled_position =
      viewport->GetSnapPositionAndSetTarget(*strategy).value_or(
          new_scaled_position);

  mojom::blink::ScrollBehavior scroll_behavior =
      ScrollableArea::V8EnumToScrollBehavior(
          scroll_to_options->behavior().AsEnum());
  viewport->SetProgrammaticScrollOffset(
      viewport->ScrollPositionToOffset(new_scaled_position),
      cc::ScrollSourceType::kRelativeScroll, scroll_behavior,
      resolver->CreateActiveScrollTracker());
}

void LocalDOMWindow::scrollTo(double x, double y) const {
  ScrollToOptions* options = ScrollToOptions::Create();
  options->setLeft(x);
  options->setTop(y);
  scrollTo(options);
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
  SyncScrollAttemptHeuristic::DidSetScrollOffset();

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

void LocalDOMWindow::scrollByForTesting(double x, double y) const {
  scrollBy(x, y);
}

void LocalDOMWindow::scrollToForTesting(double x, double y) const {
  scrollTo(x, y);
}

void LocalDOMWindow::moveBy(int x, int y) const {
  if (!GetFrame() || !GetFrame()->IsOutermostMainFrame() ||
      document()->IsPrerendering()) {
    return;
  }

  if (IsPictureInPictureWindow()) {
    return;
  }

  LocalFrame* frame = GetFrame();
  Page* page = frame->GetPage();
  if (!page) {
    return;
  }

  gfx::Rect window_rect = page->GetChromeClient().RootWindowRect(*frame);
  window_rect.Offset(x, y);
  if (base::FeatureList::IsEnabled(features::kMoveResizeWindowToIPCs)) {
    page->GetChromeClient().MoveWindowTo(window_rect.origin(), *frame);
  } else {
    page->GetChromeClient().SetWindowRect(window_rect, *frame);
  }
}

void LocalDOMWindow::moveTo(int x, int y) const {
  if (!GetFrame() || !GetFrame()->IsOutermostMainFrame() ||
      document()->IsPrerendering()) {
    return;
  }

  if (IsPictureInPictureWindow()) {
    return;
  }

  LocalFrame* frame = GetFrame();
  Page* page = frame->GetPage();
  if (!page) {
    return;
  }

  if (base::FeatureList::IsEnabled(features::kMoveResizeWindowToIPCs)) {
    page->GetChromeClient().MoveWindowTo(gfx::Point(x, y), *frame);
  } else {
    gfx::Rect window_rect = page->GetChromeClient().RootWindowRect(*frame);
    window_rect.set_origin(gfx::Point(x, y));
    page->GetChromeClient().SetWindowRect(window_rect, *frame);
  }
}

void LocalDOMWindow::resizeBy(int x,
                              int y,
                              ExceptionState& exception_state) const {
  if (!GetFrame() || !GetFrame()->IsOutermostMainFrame() ||
      document()->IsPrerendering()) {
    return;
  }

  if (IsPictureInPictureWindow()) {
    if (!LocalFrame::ConsumeTransientUserActivation(GetFrame())) {
      exception_state.ThrowDOMException(
          DOMExceptionCode::kNotAllowedError,
          "resizeBy() requires user activation in document picture-in-picture");
      return;
    }
  }

  LocalFrame* frame = GetFrame();
  Page* page = frame->GetPage();
  if (!page) {
    return;
  }

  gfx::Rect fr = page->GetChromeClient().RootWindowRect(*frame);
  gfx::Size dest(fr.width() + x, fr.height() + y);
  if (base::FeatureList::IsEnabled(features::kMoveResizeWindowToIPCs)) {
    page->GetChromeClient().ResizeWindowTo(dest, *frame);
  } else {
    page->GetChromeClient().SetWindowRect(gfx::Rect(fr.origin(), dest), *frame);
  }
}

void LocalDOMWindow::resizeTo(int width,
                              int height,
                              ExceptionState& exception_state) const {
  if (!GetFrame() || !GetFrame()->IsOutermostMainFrame() ||
      document()->IsPrerendering()) {
    return;
  }

  if (IsPictureInPictureWindow()) {
    if (!LocalFrame::ConsumeTransientUserActivation(GetFrame())) {
      exception_state.ThrowDOMException(
          DOMExceptionCode::kNotAllowedError,
          "resizeTo() requires user activation in document picture-in-picture");
      return;
    }
  }

  LocalFrame* frame = GetFrame();
  Page* page = frame->GetPage();
  if (!page) {
    return;
  }

  const gfx::Size dest(width, height);
  if (base::FeatureList::IsEnabled(features::kMoveResizeWindowToIPCs)) {
    page->GetChromeClient().ResizeWindowTo(dest, *frame);
  } else {
    gfx::Rect window_rect = page->GetChromeClient().RootWindowRect(*frame);
    window_rect.set_size(dest);
    page->GetChromeClient().SetWindowRect(window_rect, *frame);
  }
}

void LocalDOMWindow::cancelAnimationFrame(int id) {
  document()->CancelAnimationFrame(id, FrameCallbackType::kWebExposed);
}

bool LocalDOMWindow::originAgentCluster() const {
  return GetAgent()->GetAgentClusterKey().IsOriginKeyed();
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

External* LocalDOMWindow::external() {
  if (!external_) {
    external_ = MakeGarbageCollected<External>();
  }
  return external_.Get();
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
        *this, event_type, registered_listener.Options());
  }

  document()->AddListenerTypeIfNeeded(event_type, *this);
  document()->DidAddEventListeners(/*count*/ 1);
  if (registered_listener.Capture() &&
      RuntimeEnabledFeatures::SkipEventCaptureEnabled()) {
    document()->SetHasCaptureListener();
  }

  for (auto& it : event_listener_observers_) {
    it->DidAddEventListener(this, event_type);
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
        *this, event_type, registered_listener.Options());
  }

  for (auto& it : event_listener_observers_) {
    it->DidRemoveEventListener(this, event_type);
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

  for (auto& it : event_listener_observers_) {
    it->DidRemoveAllEventListeners(this);
  }

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
  bool was_should_print_when_finished_loading =
      should_print_when_finished_loading_;
  should_print_when_finished_loading_ = false;

  if (was_should_print_when_finished_loading &&
      state == FrameLoader::NavigationFinishState::kSuccess) {
    print();
  }

  if (RuntimeEnabledFeatures::NavigationStateEnabled()) {
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
  visitor->Trace(screen_);
  visitor->Trace(locationbar_);
  visitor->Trace(menubar_);
  visitor->Trace(personalbar_);
  visitor->Trace(scrollbars_);
  visitor->Trace(statusbar_);
  visitor->Trace(toolbar_);
  visitor->Trace(navigator_);
  visitor->Trace(media_);
  visitor->Trace(custom_elements_);
  visitor->Trace(external_);
  visitor->Trace(viewport_);
  visitor->Trace(visualViewport_);
  visitor->Trace(event_listener_observers_);
  visitor->Trace(current_event_);
  visitor->Trace(trusted_types_);
  visitor->Trace(input_method_controller_);
  visitor->Trace(spell_checker_);
  visitor->Trace(text_suggestion_controller_);
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

bool LocalDOMWindow::IsPaymentRequestTokenActive() const {
  return payment_request_token_.IsActive();
}

bool LocalDOMWindow::ConsumePaymentRequestToken() {
  return payment_request_token_.ConsumeIfActive();
}

bool LocalDOMWindow::IsFullscreenRequestTokenActive() const {
  return fullscreen_request_token_.IsActive();
}

bool LocalDOMWindow::ConsumeFullscreenRequestToken() {
  return fullscreen_request_token_.ConsumeIfActive();
}

bool LocalDOMWindow::IsDigitalCredentialsCreateTokenActive() const {
  return digital_credentials_create_token_.IsActive();
}

bool LocalDOMWindow::ConsumeDigitalCredentialsCreateToken() {
  return digital_credentials_create_token_.ConsumeIfActive();
}

bool LocalDOMWindow::IsDigitalCredentialsGetTokenActive() const {
  return digital_credentials_get_token_.IsActive();
}

bool LocalDOMWindow::ConsumeDigitalCredentialsGetToken() {
  return digital_credentials_get_token_.ConsumeIfActive();
}

bool LocalDOMWindow::IsDisplayCaptureRequestTokenActive() const {
  return display_capture_request_token_.IsActive();
}

bool LocalDOMWindow::ConsumeDisplayCaptureRequestToken() {
  return display_capture_request_token_.ConsumeIfActive();
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

bool LocalDOMWindow::IsPictureInPictureWindow() const {
  return is_picture_in_picture_window_;
}

void LocalDOMWindow::SetIsPictureInPictureWindow() {
  is_picture_in_picture_window_ = true;
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

void LocalDOMWindow::SetHasBeenRevealed(bool revealed) {
  if (has_been_revealed_ == revealed) {
    return;
  }
  has_been_revealed_ = revealed;
  CHECK(document_);
  document_->GetViewTransitions().DidChangeRevealState();
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

void LocalDOMWindow::requestResize(ExceptionState& state) {
  DCHECK(RuntimeEnabledFeatures::ResponsiveIframesEnabled());
  if (document_) {
    document_->RequestResizeResponsiveIframe(&state);
  }
}

}  // namespace blink
