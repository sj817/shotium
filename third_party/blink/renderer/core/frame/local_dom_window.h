/*
 * Copyright (C) 2006, 2007, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_DOM_WINDOW_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_DOM_WINDOW_H_

#include <memory>

#include "base/task/single_thread_task_runner.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "services/network/public/mojom/content_security_policy.mojom-blink.h"
#include "services/network/public/mojom/storage_access_api.mojom-blink.h"
#include "third_party/blink/public/common/frame/history_user_activation_state.h"
#include "third_party/blink/public/common/tokens/tokens.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_scroll_result.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/universal_global_scope.h"
#include "third_party/blink/renderer/core/frame/use_counter_impl.h"
#include "third_party/blink/renderer/core/frame/window_event_handlers.h"
#include "third_party/blink/renderer/core/frame/window_or_worker_global_scope.h"
#include "third_party/blink/renderer/core/html/closewatcher/close_watcher.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_map.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_set.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/prefinalizer.h"
#include "third_party/blink/renderer/platform/storage/blink_storage_key.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class CSSStyleDeclaration;
class CustomElementRegistry;
class Document;
class DocumentInit;
class DOMVisualViewport;
class Element;
class FrameConsole;
class LocalFrame;
class Navigator;
class ScrollToOptions;
class SoftNavigationHeuristics;
class TrustedTypePolicyFactory;
class WindowAgent;

// Note: if you're thinking of returning something DOM-related by reference,
// please ping dcheng@chromium.org first. You probably don't want to do that.
class CORE_EXPORT LocalDOMWindow final : public DOMWindow,
                                         public ExecutionContext,
                                         public WindowOrWorkerGlobalScope,
                                         public UniversalGlobalScope,
                                         public WindowEventHandlers,
                                         public Supplementable<LocalDOMWindow> {
  USING_PRE_FINALIZER(LocalDOMWindow, Dispose);

 public:
  // Size thresholds for network efficiency guardrails policy enforcement.
  // These are const public for testing purpose.
  static constexpr size_t kGuardrailsLargeDataThresholdBytes =
      100 * 1024;  // 100kB
  static constexpr size_t kGuardrailsLargeImageThresholdBytes =
      200 * 1024;  // 200kB

  LocalDOMWindow(LocalFrame&, WindowAgent*);
  ~LocalDOMWindow() override;

  // Returns the token identifying the frame that this ExecutionContext was
  // associated with at the moment of its creation. This remains valid even
  // after the frame has been destroyed and the ExecutionContext is detached.
  // This is used as a stable and persistent identifier for attributing detached
  // context memory usage.
  const LocalFrameToken& GetLocalFrameToken() const { return token_; }
  ExecutionContextToken GetExecutionContextToken() const final {
    return token_;
  }

  LocalFrame* GetFrame() const {
    // UnsafeTo<> is safe here because DOMWindow's frame can only change to
    // nullptr, and it was constructed with a LocalFrame in the constructor.
    return UnsafeTo<LocalFrame>(DOMWindow::GetFrame());
  }

  void Initialize();
  void ClearForReuse();

  void ResetWindowAgent(WindowAgent*);

  mojom::blink::V8CacheOptions GetV8CacheOptions() const override;

  // Bind Content Security Policy to this window. This will cause the
  // CSP to resolve the 'self' attribute and all policies will then be
  // applied to this document.
  void BindContentSecurityPolicy();

  void Trace(Visitor*) const override;

  // ExecutionContext overrides:
  bool IsWindow() const final { return true; }
  bool IsContextThread() const final;
  const KURL& Url() const final;
  const KURL& BaseURL() const final;
  KURL CompleteURL(const String&) const final;
  void DisableEval(const String& error_message) final;
  void SetWasmEvalErrorMessage(const String& error_message) final;
  String UserAgent() const final;
  UserAgentMetadata GetUserAgentMetadata() const final;
  HttpsState GetHttpsState() const final;
  ResourceFetcher* Fetcher() final;
  bool CanExecuteScripts(ReasonForCallingCanExecuteScripts) final;
  void ExceptionThrown(ErrorEvent*) final;
  EventTarget* ErrorEventTarget() final { return this; }
  KURL OutgoingReferrerUrl() const final;
  void SetInitiatorStateToken(
      const InitiatorStateToken& initiator_state_token) final;
  CoreProbeSink* GetProbeSink() final;
  const BrowserInterfaceBrokerProxy& GetBrowserInterfaceBroker() const final;
  FrameOrWorkerScheduler* GetScheduler() final;
  scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner(TaskType) final;
  // TODO(crbug.com/451479061): Consider moving the following function
  // under trustedTypes/
  TrustedTypePolicyFactory* GetTrustedTypes() const final;
  ScriptWrappable* ToScriptWrappable() final { return this; }
  void ReportPermissionsPolicyViolation(
      network::mojom::PermissionsPolicyFeature,
      mojom::blink::PolicyDisposition,
      const String& reporting_endpoint,
      const String& message = g_empty_string) const final;
  void ReportPotentialPermissionsPolicyViolation(
      network::mojom::PermissionsPolicyFeature,
      mojom::blink::PolicyDisposition,
      const String& reporting_endpoint,
      const String& message = g_empty_string,
      const String& allow_attribute = g_empty_string,
      const String& src_attribute = g_empty_string) const final;
  void ReportDocumentPolicyViolation(
      mojom::blink::DocumentPolicyFeature,
      mojom::blink::PolicyDisposition,
      const String& message = g_empty_string,
      // If source_file is set to empty string,
      // current JS file would be used as source_file instead.
      const String& source_file = g_empty_string) const final;
  void SetIsInBackForwardCache(bool) final;
  net::StorageAccessApiStatus GetStorageAccessApiStatus() const final;
  std::optional<mojom::blink::PolicyDisposition> GetGuardrailsPolicyState()
      const final;
  bool CheckGuardrailsPolicyForAssetSize(GuardrailPolicyAssetType asset_type,
                                         size_t bytes,
                                         const KURL& url) const final;

  void AddConsoleMessageImpl(ConsoleMessage*, bool discard_duplicates) final;

  scoped_refptr<base::SingleThreadTaskRunner>
  GetAgentGroupSchedulerCompositorTaskRunner() final;

  // UseCounter orverrides:
  void CountUse(mojom::WebFeature feature) final;
  void CountWebDXFeature(mojom::blink::WebDXFeature feature) final;

  // Count |feature| only when this window is associated with a cross-site
  // iframe. A "site" is a scheme and registrable domain.
  void CountUseOnlyInCrossSiteIframe(mojom::blink::WebFeature feature) override;

  // Count permissions policy feature usage through use counter.
  void CountPermissionsPolicyUsage(
      network::mojom::PermissionsPolicyFeature feature,
      UseCounterImpl::PermissionsPolicyUsageType type);

  // Checks if navigation to Javascript URL is allowed. This check should run
  // before any action is taken (e.g. creating new window) for all
  // same-origin navigations.
  bool AllowInlineJavascriptUrl(const KURL& url, Element* element);

  Document* InstallNewDocument(const DocumentInit&);

  // EventTarget overrides:
  ExecutionContext* GetExecutionContext() const override;
  const LocalDOMWindow* ToLocalDOMWindow() const override;
  LocalDOMWindow* ToLocalDOMWindow() override;

  // Same-origin DOM Level 0
  Navigator* navigator();

  double scrollX() const;
  double scrollY() const;

  DOMVisualViewport* visualViewport();

  // DOM Level 2 AbstractView Interface
  Document* document() const;

  // WebKit extensions
  double devicePixelRatio() const;

  // Element::scrollTo() forwards to this for the document element.
  void scrollTo(const ScrollToOptions*) const;

  // DOM Level 2 Style Interface
  CSSStyleDeclaration* getComputedStyle(
      Element*,
      const String& pseudo_elt = String()) const;

  // Custom elements
  //
  // `window.customElements` -- the global registry, owned by the main world.
  CustomElementRegistry* customElements() const;
  CustomElementRegistry* MaybeCustomElements() const;

  DEFINE_ATTRIBUTE_EVENT_LISTENER(search, kSearch)

  DEFINE_ATTRIBUTE_EVENT_LISTENER(orientationchange, kOrientationchange)

  void FrameDestroyed();
  void Reset();

  FrameConsole* GetFrameConsole() const;

  void PrintErrorMessage(const String&) const;

  // Events
  // EventTarget API
  void RemoveAllEventListeners() override;

  using EventTarget::DispatchEvent;
  DispatchEventResult DispatchEvent(Event&, EventTarget*);

  void FinishedLoading(FrameLoader::NavigationFinishState);

  void EnqueueWindowEvent(Event&, TaskType);
  void EnqueueDocumentEvent(Event&, TaskType);
  void EnqueueNonPersistedPageshowEvent();
  void EnqueueHashchangeEvent(const String& old_url,
                              const String& new_url,
                              UserNavigationInvolvement involvement);
  void DispatchPopstateEvent(bool has_ua_visual_transition,
                             UserNavigationInvolvement involvement);
  void DispatchWindowLoadEvent();
  // Dispatches the window load event and the non-persisted pageshow event
  // after the document finishes loading.
  void DispatchLoadAndPageshowEvents();

  void AcceptLanguagesChanged();


  bool CrossOriginIsolatedCapability() const override;
  bool IsIsolatedContext() const override;

  // These delegate to the document_.
  ukm::UkmRecorder* UkmRecorder() override;
  ukm::SourceId UkmSourceID() const override;

  const BlinkStorageKey& GetStorageKey() const { return storage_key_; }
  void SetStorageKey(const BlinkStorageKey& storage_key);

  // Returns a token used to retrieve state associated with this LocalWindow in
  // the browser process. This should be passed to navigations started from this
  // LocalWindow. The InitiatorStateToken remains valid as long as:
  //   - The frame exists and its policies have not changed.
  //   - Or there is an ongoing navigation started from the frame in the state
  //   associated with the token.
  const InitiatorStateToken& GetInitiatorStateToken() const {
    return initiator_state_token_;
  }

  // Called when a network request buffered an additional `num_bytes` while this
  // frame is in back-forward cache.
  void DidBufferLoadWhileInBackForwardCache(bool update_process_wide_count,
                                            size_t num_bytes);

  // Whether the window is credentialless or not.
  bool credentialless() const;

  bool IsInFencedFrame() const override;

  CloseWatcher::WatcherStack* closewatcher_stack() {
    return closewatcher_stack_.Get();
  }

  // Sets the StorageAccessApiStatus. Calls to this method must not downgrade
  // the status.
  void SetStorageAccessApiStatus(net::StorageAccessApiStatus status);

  SoftNavigationHeuristics* GetSoftNavigationHeuristics() {
    return soft_navigation_heuristics_.Get();
  }

 protected:
  // EventTarget overrides.
  void AddedEventListener(const AtomicString& event_type,
                          RegisteredEventListener&) override;
  void RemovedEventListener(const AtomicString& event_type,
                            const RegisteredEventListener&) override;

  // Protected DOMWindow overrides.

 private:
  class NetworkStateObserver;

  // Intentionally private to prevent redundant checks.
  bool IsLocalDOMWindow() const override { return true; }

  bool HasInsecureContextInAncestors() const override;

  Document& GetDocumentForWindowEventHandler() const override {
    return *document();
  }

  void Dispose();

  void DispatchLoadEvent();

  void UpdateEventListenerCountsToDocumentForReuseIfNeeded();

  Member<Document> document_;
  Member<DOMVisualViewport> visualViewport_;

  mutable Member<Navigator> navigator_;
  mutable Member<CustomElementRegistry> custom_elements_;

  // The single TrustedTypePolicyFactory for this window.
  mutable Member<TrustedTypePolicyFactory> trusted_types_;

  // A dummy scheduler to return when the window is detached.
  // All operations on it result in no-op, but due to this it's safe to
  // use the returned value of GetScheduler() without additional checks.
  // A task posted to a task runner obtained from one of its task runners
  // will be forwarded to the default task runner.
  // TODO(altimin): We should be able to remove it after we complete
  // frame:document lifetime refactoring.
  std::unique_ptr<FrameOrWorkerScheduler> detached_scheduler_;


  // Tracks which features have already been potentially violated in this
  // document. This helps to count them only once per page load.
  // We don't use std::bitset to avoid to include
  // permissions_policy.mojom-blink.h.
  mutable Vector<bool> potentially_violated_features_;

  // Token identifying the LocalFrame that this window was associated with at
  // creation. Remains valid even after the frame is destroyed and the context
  // is detached.
  const LocalFrameToken token_;

  // Token tracking the state of policies in the LocalFrame. Used to find an
  // InitiatorNavigationState in the browser process for navigations started
  // from this LocalFrame. This will be updated if the policies of the
  // LocalFrame change (e.g. Referrer policy, CSP).
  InitiatorStateToken initiator_state_token_;

  // The storage key for this LocalDomWindow.
  BlinkStorageKey storage_key_;

  // Fire "online" and "offline" events.
  Member<NetworkStateObserver> network_state_observer_;

  // The total bytes buffered by all network requests in this frame while frozen
  // due to back-forward cache. This number gets reset when the frame gets out
  // of the back-forward cache.
  size_t total_bytes_buffered_while_in_back_forward_cache_ = 0;

  Member<CloseWatcher::WatcherStack> closewatcher_stack_;

  Member<SoftNavigationHeuristics> soft_navigation_heuristics_;

  // Records this window's Storage Access API status. It cannot be downgraded.
  net::StorageAccessApiStatus storage_access_api_status_ =
      net::StorageAccessApiStatus::kNone;

  // Used to indicate if the DOM window is reused or not.
  bool is_dom_window_reused_ = false;
};

template <>
struct DowncastTraits<LocalDOMWindow> {
  static bool AllowFrom(const ExecutionContext& context) {
    return context.IsWindow();
  }
  static bool AllowFrom(const DOMWindow& window) {
    return window.IsLocalDOMWindow();
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_DOM_WINDOW_H_
