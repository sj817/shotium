/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2013 Apple Inc. All rights
 * reserved.
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_PAGE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_PAGE_H_

#include <memory>
#include <optional>

#include "base/check_op.h"
#include "base/dcheck_is_on.h"
#include "base/types/pass_key.h"
#include "net/cookies/site_for_cookies.h"
#include "third_party/blink/public/common/fenced_frame/redacted_fenced_frame_config.h"
#include "third_party/blink/public/common/fingerprinting_protection/noise_token.h"
#include "third_party/blink/public/common/page/color_provider_color_maps.h"
#include "third_party/blink/public/mojom/devtools/inspector_issue.mojom-blink-forward.h"
#include "third_party/blink/public/mojom/frame/color_scheme.mojom-blink-forward.h"
#include "third_party/blink/public/mojom/page/page.mojom-blink-forward.h"
#include "third_party/blink/public/mojom/page/page_visibility_state.mojom-blink.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/platform/scheduler/web_scoped_virtual_time_pauser.h"
#include "third_party/blink/public/web/web_window_features.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/frame/deprecation/deprecation.h"
#include "third_party/blink/renderer/core/frame/settings_delegate.h"
#include "third_party/blink/renderer/core/page/page_visibility_observer.h"
#include "third_party/blink/renderer/core/page/viewport_description.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_set.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_linked_hash_set.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_vector.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"
#include "third_party/blink/renderer/platform/scheduler/public/agent_group_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/public/page_scheduler.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace cc {
class AnimationHost;
}  // namespace cc

namespace ui {
class ColorProvider;
}  // namespace ui

namespace blink {
class AutoscrollController;
class BrowserControls;
class ChromeClient;
class ConsoleMessageStorage;
class DragCaret;
class FocusController;
class Frame;
class LocalFrame;
class PageAnimator;
class PageScaleConstraintsSet;
class ScopedPagePauser;
class ScrollbarTheme;
class Settings;
class SpatialNavigationController;
class SVGDocumentResourceTracker;
class TopDocumentRootScrollerController;
class VisualViewport;

// A Page roughly corresponds to a tab or popup window in a browser. It owns a
// tree of frames (a blink::FrameTree). The root frame is called the main frame.
//
// Note that frames can be local or remote to this process.
class CORE_EXPORT Page final : public GarbageCollected<Page>,
                               public Supplementable<Page>,
                               public SettingsDelegate,
                               public PageScheduler::Delegate {
  friend class Settings;

 public:
  // Any pages not owned by a web view should be created using this method.
  static Page* CreateNonOrdinary(
      ChromeClient& chrome_client,
      AgentGroupScheduler& agent_group_scheduler,
      const ColorProviderColorMaps* color_provider_colors);

  // An "ordinary" page is a fully-featured page owned by a web view.
  static Page* CreateOrdinary(
      ChromeClient& chrome_client,
      Page* opener,
      AgentGroupScheduler& agent_group_scheduler,
      const base::UnguessableToken& browsing_context_group_token,
      const ColorProviderColorMaps* color_provider_colors);

  Page(base::PassKey<Page>,
       ChromeClient& chrome_client,
       AgentGroupScheduler& agent_group_scheduler,
       const base::UnguessableToken& browsing_context_group_token,
       const ColorProviderColorMaps* color_provider_colors,
       bool is_ordinary);
  Page(const Page&) = delete;
  Page& operator=(const Page&) = delete;
  ~Page() override;

  void CloseSoon();
  bool IsClosing() const { return is_closing_; }

  using PageSet = HeapHashSet<WeakMember<Page>>;

  // Return the current set of full-fledged, ordinary pages.
  // Each created and owned by a WebView.
  //
  // This set does not include Pages created for other, internal purposes
  // (SVGImages, inspector overlays, page popups etc.)
  static PageSet& OrdinaryPages();

  // Returns pages related to the current browsing context (excluding the
  // current page).  See also
  // https://html.spec.whatwg.org/C/#nested-browsing-contexts
  HeapVector<Member<Page>> RelatedPages();

  // Should be called when |GetScrollbarTheme().UsesOverlayScrollbars()|
  // changes.
  static void UsesOverlayScrollbarsChanged();
  static void ForcedColorsChanged();
  static void PlatformColorsChanged();
  static void ColorSchemeChanged();

  bool UpdateColorProviders(
      const ColorProviderColorMaps& color_provider_colors);
  const ui::ColorProvider* GetColorProviderForPainting(
      mojom::blink::ColorScheme color_scheme,
      bool in_forced_colors) const;

  // Returns the color provider colors for this page. Used to support the
  // creation of Non-ordiany pages from a main page.
  const ColorProviderColorMaps& GetColorProviderColorMaps() {
    return color_provider_colors_;
  }

  void InitialStyleChanged();
  void UAStyleChanged();
  void UpdateAcceleratedCompositingSettings();

  ViewportDescription GetViewportDescription() const;

  void SetMainFrame(Frame*);
  Frame* MainFrame() const { return main_frame_.Get(); }

  void SetPreviousMainFrameForLocalSwap(
      LocalFrame* previous_main_frame_for_local_swap) {
    previous_main_frame_for_local_swap_ = previous_main_frame_for_local_swap;
  }

  LocalFrame* GetPreviousMainFrameForLocalSwap() {
    return previous_main_frame_for_local_swap_.Get();
  }

  // Escape hatch for existing code that assumes that the root frame is
  // always a LocalFrame. With OOPI, this is not always the case. Code that
  // depends on this will generally have to be rewritten to propagate any
  // necessary state through all renderer processes for that page and/or
  // coordinate/rely on the browser process to help dispatch/coordinate work.
  LocalFrame* DeprecatedLocalMainFrame() const;

  void Animate(base::TimeTicks monotonic_frame_begin_time);

  PageAnimator& Animator() { return *animator_; }
  ChromeClient& GetChromeClient() const {
    DCHECK(chrome_client_) << "No chrome client";
    return *chrome_client_;
  }
  AutoscrollController& GetAutoscrollController() const {
    return *autoscroll_controller_;
  }
  DragCaret& GetDragCaret() const { return *drag_caret_; }
  FocusController& GetFocusController() const { return *focus_controller_; }
  SpatialNavigationController& GetSpatialNavigationController();
  SVGDocumentResourceTracker& GetSVGDocumentResourceTracker();

  Settings& GetSettings() const { return *settings_; }

  Deprecation& GetDeprecation() { return deprecation_; }

  const WebWindowFeatures& GetWindowFeatures() const {
    return window_features_;
  }

  PageScaleConstraintsSet& GetPageScaleConstraintsSet();
  const PageScaleConstraintsSet& GetPageScaleConstraintsSet() const;

  BrowserControls& GetBrowserControls();
  const BrowserControls& GetBrowserControls() const;

  ConsoleMessageStorage& GetConsoleMessageStorage();
  const ConsoleMessageStorage& GetConsoleMessageStorage() const;

  TopDocumentRootScrollerController& GlobalRootScrollerController() const;

  VisualViewport& GetVisualViewport();
  const VisualViewport& GetVisualViewport() const;

  // Pausing is used to implement the "Optionally, pause while waiting for
  // the user to acknowledge the message" step of simple dialog processing:
  // https://html.spec.whatwg.org/C/#simple-dialogs
  //
  // Per https://html.spec.whatwg.org/C/#pause, no loads
  // are allowed to start/continue in this state, and all background processing
  // is also paused.
  bool Paused() const { return paused_; }
  void SetPaused(bool);

  // Frozen state corresponds to "lifecycle state for CPU suspension"
  // https://wicg.github.io/page-lifecycle/#sec-lifecycle-states
  bool Frozen() const { return frozen_; }

  void SetPageScaleFactor(float);
  float PageScaleFactor() const;

  float InspectorDeviceScaleFactorOverride() const {
    return inspector_device_scale_factor_override_;
  }
  void SetInspectorDeviceScaleFactorOverride(float override) {
    inspector_device_scale_factor_override_ = override;
  }

  void SetVisibilityState(mojom::blink::PageVisibilityState visibility_state,
                          bool is_initial_state);
  bool IsPageVisible() const;

  // Don't allow more than a certain number of frames in a page.
  static int MaxNumberOfFrames();

  void IncrementSubframeCount() { ++subframe_count_; }
  void DecrementSubframeCount() {
    DCHECK_GT(subframe_count_, 0);
    --subframe_count_;
  }
  int SubframeCount() const;

  void DidCommitLoad(LocalFrame*);

  void Trace(Visitor*) const override;

  void WillBeDestroyed();

  ScrollbarTheme& GetScrollbarTheme() const;

  AgentGroupScheduler& GetAgentGroupScheduler() const;
  PageScheduler* GetPageScheduler() const;

  // PageScheduler::Delegate implementation.
  bool IsOrdinary() const override;
  void OnSetPageFrozen(bool is_frozen) override;

  WebScopedVirtualTimePauser& HistoryNavigationVirtualTimePauser() {
    return history_navigation_virtual_time_pauser_;
  }

  HeapLinkedHashSet<WeakMember<PageVisibilityObserver>>&
  PageVisibilityObserverSet() {
    return page_visibility_observer_set_;
  }

  const mojom::blink::PageLifecycleStatePtr& GetPageLifecycleState() {
    return lifecycle_state_;
  }

  // Whether we've dispatched "pagehide" on this page previously, and haven't
  // dispatched the "pageshow" event after the last time we've dispatched
  // "pagehide". This means that we've navigated away from the page and it's
  // still hidden (possibly preserved in the back-forward cache, or unloaded).
  bool DispatchedPagehideAndStillHidden();

  // Similar to above, but will only return true if we've dispatched 'pagehide'
  // with the 'persisted' property set to 'true'.
  bool DispatchedPagehidePersistedAndStillHidden();

  // Fully invalidate paint of all local frames in this page.
  void InvalidatePaint();

  // Should be invoked when the main frame of this frame tree is a fenced frame.
  void SetIsMainFrameFencedFrameRoot();
  // Returns if the main frame of this frame tree is a fenced frame.
  bool IsMainFrameFencedFrameRoot() const;

  void SetDeprecatedFencedFrameMode(
      blink::FencedFrame::DeprecatedFencedFrameMode mode) {
    fenced_frame_mode_ = mode;
  }
  blink::FencedFrame::DeprecatedFencedFrameMode DeprecatedFencedFrameMode() {
    return fenced_frame_mode_;
  }

  // Returns the token uniquely identifying the browsing context group this page
  // lives in.
  const base::UnguessableToken& BrowsingContextGroupToken();

  // Update this Page's browsing context group after a navigation has taken
  // place.
  void UpdateBrowsingContextGroup(const base::UnguessableToken&);

  // Called on a new Page, passing an old Page as the parameter, when doing a
  // LocalFrame <-> LocalFrame swap when committing a navigation, to ensure that
  // e.g. the close task will still be processed after the swap, the list of
  // related pages will include the new page instead of the old page, etc.
  void TakePropertiesForLocalMainFrameSwap(Page* old_page);

  void NotifyRelatedPagesFinalized(bool has_other_related_pages) {
    related_pages_mutation_from_previous_page_finalized_ = true;
    has_other_related_pages_during_commit_ = has_other_related_pages;
  }

  bool RelatedPagesMutationFromPreviousPageFinalized() const {
    return related_pages_mutation_from_previous_page_finalized_;
  }
  bool HasOtherRelatedPagesDuringCommit() const {
    return has_other_related_pages_during_commit_;
  }

 private:
  // Test-only friends for the scroll-to-text-fragment feature
  // (TextFragmentAnchor et al.) used to be granted access here so they could
  // poke at private Page state. The whole feature is deleted -- there is no
  // browser UI to build a "copy link to highlight" affordance for -- and its
  // tests went with it.
  friend class ScopedPagePauser;
  class CloseTaskHandler;

  // SettingsDelegate overrides.
  void SettingsChanged(SettingsDelegate::ChangeType) override;

  void AcceptLanguagesChanged();

  void InvalidateColorScheme();

  // Connect the Page to the `opener_`'s related pages, if those exist.
  void LinkRelatedPagesIfNeeded();

  // Typically, the main frame and Page should both be owned by the embedder,
  // which must call Page::willBeDestroyed() prior to destroying Page. This
  // call detaches the main frame and clears this pointer, thus ensuring that
  // this field only references a live main frame.
  //
  // However, there are several locations (InspectorOverlay, SVGImage, and
  // WebPagePopupImpl) which don't hold a reference to the main frame at all
  // after creating it. These are still safe because they always create a
  // Frame with a LocalFrameView. LocalFrameView and Frame hold references to
  // each other, thus keeping each other alive. The call to willBeDestroyed()
  // breaks this cycle, so the frame is still properly destroyed once no
  // longer needed.
  // Note that the main frame can either be a LocalFrame or a RemoteFrame. When
  // the main frame is a RemoteFrame, it's possible for the RemoteFrame to not
  // be connected to any RenderFrameProxyHost on the browser side, if the Page
  // is a new page created for a provisional main frame. In that case, the main
  // frame is solely used as a placeholder to be swapped out by the provisional
  // main frame later on.
  // See comments in `AgentSchedulingGroup::CreateWebView()` for more details.
  Member<Frame> main_frame_;

  // When a Page is created for a provisional main frame, which is intended to
  // do a local main frame swap when its navigation commits, this will point to
  // the previous Page's main frame. This is so that the provisional main frame
  // can trigger the detach and "swap out" the previous Page's main frame. This
  // is a WeakMember because the lifetime of this page and the previous Page
  // should be independent. If the previous Page gets destroyed, the provisional
  // Page can still exist (but the browser might trigger its deletion later on).
  WeakMember<LocalFrame> previous_main_frame_for_local_swap_;

  Member<AgentGroupScheduler> agent_group_scheduler_;
  Member<PageAnimator> animator_;
  const Member<AutoscrollController> autoscroll_controller_;
  Member<ChromeClient> chrome_client_;
  const Member<DragCaret> drag_caret_;
  const Member<FocusController> focus_controller_;
  const Member<PageScaleConstraintsSet> page_scale_constraints_set_;
  HeapLinkedHashSet<WeakMember<PageVisibilityObserver>>
      page_visibility_observer_set_;
  const Member<BrowserControls> browser_controls_;
  const Member<ConsoleMessageStorage> console_message_storage_;
  const Member<TopDocumentRootScrollerController>
      global_root_scroller_controller_;
  const Member<VisualViewport> visual_viewport_;
  Member<SpatialNavigationController> spatial_navigation_controller_;
  Member<SVGDocumentResourceTracker> svg_document_resource_tracker_;

  Deprecation deprecation_;
  WebWindowFeatures window_features_;

  // Set to true when window.close() has been called and the Page will be
  // destroyed. The browsing contexts in this page should no longer be
  // discoverable via JS.
  // TODO(dcheng): Try to remove |DOMWindow::m_windowIsClosing| in favor of
  // this. However, this depends on resolving https://crbug.com/674641
  bool is_closing_;

  float inspector_device_scale_factor_override_;

  mojom::blink::PageLifecycleStatePtr lifecycle_state_;

  bool is_ordinary_;

  // See Page::Paused and Page::Frozen for the detailed description of paused
  // and frozen state. The main distinction is that "frozen" state is
  // web-exposed (onfreeze / onresume) and controlled from the browser process,
  // while "paused" state is an implementation detail of handling sync IPCs and
  // controlled from the renderer.
  bool paused_ = false;
  bool frozen_ = false;

  int subframe_count_;

  // The light, dark and forced_colors mode ColorProviders corresponding to the
  // top-level web container this Page is associated with.
  std::unique_ptr<ui::ColorProvider> light_color_provider_;
  std::unique_ptr<ui::ColorProvider> dark_color_provider_;
  std::unique_ptr<ui::ColorProvider> forced_colors_color_provider_;

  // Caching the color provider colors for easy creation of non ordinary pages
  // who may depend on the main Page for colors.
  ColorProviderColorMaps color_provider_colors_;

  // A circular, double-linked list of pages that are related to the current
  // browsing context.  See also RelatedPages method.
  Member<Page> next_related_page_;
  Member<Page> prev_related_page_;

  // Indicates whether the related pages set can change due to previous page's
  // mutations. Set when this (new page) is being committed. Once finalized, we
  // would not expect the previous page to change the related pages set
  // (although it can still change on this page).
  bool related_pages_mutation_from_previous_page_finalized_ = false;
  // Note that `has_other_related_pages_during_commit_` may not be in sync with
  // RelatedPages() list and that's a bug. This is only used for text fragment
  // checks to see if we're allowed to do a scroll or not. Ideally we don't need
  // this and should just check the RelatedPages() set if the bug is fixed. See
  // crbug.com/457771782 for details.
  bool has_other_related_pages_during_commit_ = false;

  // The Page that opened this Page.
  WeakMember<Page> opener_;

  std::unique_ptr<PageScheduler> page_scheduler_;


  // Whether the the Page's main document is a Fenced Frame document. This is
  // only set for the MPArch implementation and is true when the corresponding
  // browser side FrameTree has the FrameTree::Type of kFencedFrame.
  bool is_fenced_frame_tree_ = false;

  // This tracks the mode that the fenced frame is set to.
  blink::FencedFrame::DeprecatedFencedFrameMode fenced_frame_mode_ =
      blink::FencedFrame::DeprecatedFencedFrameMode::kDefault;

  WebScopedVirtualTimePauser history_navigation_virtual_time_pauser_;

  // The information determining the browsing context group this page lives in.
  base::UnguessableToken browsing_context_group_token_;

  Member<CloseTaskHandler> close_task_handler_;
};

extern template class CORE_EXTERN_TEMPLATE_EXPORT Supplement<Page>;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_PAGE_H_
