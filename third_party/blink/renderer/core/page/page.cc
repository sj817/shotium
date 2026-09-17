/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All
 * Rights Reserved.
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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

#include "third_party/blink/renderer/core/page/page.h"

#include "base/check.h"
#include "base/compiler_specific.h"
#include "base/feature_list.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/page/color_provider_color_maps.h"
#include "third_party/blink/public/mojom/frame/lifecycle.mojom-blink-forward.h"
#include "third_party/blink/public/mojom/page/page.mojom-blink.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/css/css_default_style_sheets.h"
#include "third_party/blink/renderer/core/css/style_change_reason.h"
#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/node_rare_data.h"
#include "third_party/blink/renderer/core/editing/drag_caret.h"
#include "third_party/blink/renderer/core/editing/markers/document_marker_controller.h"
#include "third_party/blink/renderer/core/frame/browser_controls.h"
#include "third_party/blink/renderer/core/frame/event_handler_registry.h"
#include "third_party/blink/renderer/core/frame/frame_console.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/core/frame/page_scale_constraints_set.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/viewport_data.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/fullscreen/fullscreen.h"
#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/inspector/console_message_storage.h"
#include "third_party/blink/renderer/core/layout/layout_object_inlines.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/loader/idleness_detector.h"
#include "third_party/blink/renderer/core/page/autoscroll_controller.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/focus_controller.h"
#include "third_party/blink/renderer/core/page/page_animator.h"
#include "third_party/blink/renderer/core/page/page_hidden_state.h"
#include "third_party/blink/renderer/core/page/scoped_page_pauser.h"
#include "third_party/blink/renderer/core/page/scrolling/top_document_root_scroller_controller.h"
#include "third_party/blink/renderer/core/paint/paint_layer_scrollable_area.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/scroll/scrollbar_theme.h"
#include "third_party/blink/renderer/core/scroll/scrollbar_theme_overlay_mobile.h"
#include "third_party/blink/renderer/core/svg/graphics/svg_image_chrome_client.h"
#include "third_party/blink/renderer/core/svg/svg_document_resource_tracker.h"
#include "third_party/blink/renderer/core/svg/svg_resource_scheduler_registry.h"
#include "third_party/blink/renderer/platform/bindings/source_location.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/heap/disallow_new_wrapper.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/scheduler/public/agent_group_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_scheduler.h"
#include "third_party/blink/renderer/platform/web_test_support.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_utils.h"

namespace blink {

namespace {
// This seems like a reasonable upper bound, and otherwise mutually
// recursive frameset pages can quickly bring the program to its knees
// with exponential growth in the number of frames.
const int kMaxNumberOfFrames = 1000;

}  // namespace

// Set of all live pages; includes internal Page objects that are
// not observable from scripts.
static Page::PageSet& AllPages() {
  using PageSetHolder = DisallowNewWrapper<Page::PageSet>;
  DEFINE_STATIC_LOCAL(Persistent<PageSetHolder>, pages,
                      (MakeGarbageCollected<PageSetHolder>()));
  return pages->Value();
}

Page::PageSet& Page::OrdinaryPages() {
  using PageSetHolder = DisallowNewWrapper<Page::PageSet>;
  DEFINE_STATIC_LOCAL(Persistent<PageSetHolder>, pages,
                      (MakeGarbageCollected<PageSetHolder>()));
  return pages->Value();
}

HeapVector<Member<Page>> Page::RelatedPages() {
  HeapVector<Member<Page>> result;
  Page* ptr = next_related_page_;
  while (ptr != this) {
    result.push_back(ptr);
    ptr = ptr->next_related_page_;
  }
  return result;
}

Page* Page::CreateNonOrdinary(
    ChromeClient& chrome_client,
    AgentGroupScheduler& agent_group_scheduler,
    const ColorProviderColorMaps* color_provider_colors) {
  return MakeGarbageCollected<Page>(
      base::PassKey<Page>(), chrome_client, agent_group_scheduler,
      /*browsing_context_group_token=*/base::UnguessableToken::Create(),
      color_provider_colors,
      /*is_ordinary=*/false);
}

Page* Page::CreateOrdinary(
    ChromeClient& chrome_client,
    Page* opener,
    AgentGroupScheduler& agent_group_scheduler,
    const base::UnguessableToken& browsing_context_group_token,
    const ColorProviderColorMaps* color_provider_colors) {
  Page* page = MakeGarbageCollected<Page>(
      base::PassKey<Page>(), chrome_client, agent_group_scheduler,
      browsing_context_group_token, color_provider_colors,
      /*is_ordinary=*/true);
  page->opener_ = opener;

  OrdinaryPages().insert(page);

  if (ScopedPagePauser::IsActive()) {
    page->SetPaused(true);
  }

  return page;
}

Page::Page(base::PassKey<Page>,
           ChromeClient& chrome_client,
           AgentGroupScheduler& agent_group_scheduler,
           const base::UnguessableToken& browsing_context_group_token,
           const ColorProviderColorMaps* color_provider_colors,
           bool is_ordinary)
    : SettingsDelegate(std::make_unique<Settings>()),
      main_frame_(nullptr),
      agent_group_scheduler_(agent_group_scheduler),
      animator_(MakeGarbageCollected<PageAnimator>(*this)),
      autoscroll_controller_(MakeGarbageCollected<AutoscrollController>(*this)),
      chrome_client_(&chrome_client),
      drag_caret_(MakeGarbageCollected<DragCaret>()),
      focus_controller_(MakeGarbageCollected<FocusController>(this)),
      page_scale_constraints_set_(
          MakeGarbageCollected<PageScaleConstraintsSet>(this)),
      browser_controls_(MakeGarbageCollected<BrowserControls>(*this)),
      console_message_storage_(MakeGarbageCollected<ConsoleMessageStorage>()),
      global_root_scroller_controller_(
          MakeGarbageCollected<TopDocumentRootScrollerController>(*this)),
      visual_viewport_(MakeGarbageCollected<VisualViewport>(*this)),
      inspector_device_scale_factor_override_(1),
      lifecycle_state_(mojom::blink::PageLifecycleState::New()),
      is_ordinary_(is_ordinary),
      subframe_count_(0),
      next_related_page_(this),
      prev_related_page_(this),
      browsing_context_group_token_(browsing_context_group_token) {
  DCHECK(!AllPages().Contains(this));
  AllPages().insert(this);

  page_scheduler_ = agent_group_scheduler_->CreatePageScheduler(this);
  // The scheduler should be set before the main frame.
  DCHECK(!main_frame_);
  if (auto* virtual_time_controller =
          page_scheduler_->GetVirtualTimeController()) {
    history_navigation_virtual_time_pauser_ =
        virtual_time_controller->CreateWebScopedVirtualTimePauser(
            "HistoryNavigation",
            WebScopedVirtualTimePauser::VirtualTaskDuration::kInstant);
  }
  UpdateColorProviders(color_provider_colors &&
                               !color_provider_colors->IsEmpty()
                           ? *color_provider_colors
                           : ColorProviderColorMaps::CreateDefault());
}

Page::~Page() {
  // WillBeDestroyed() must be called before Page destruction.
  DCHECK(!main_frame_);
}

void Page::CloseSoon() {
  // Make sure this Page can no longer be found by JS.
  is_closing_ = true;

  // Upstream also posted a deferred close task to the WebView here; shotium
  // has no WebView to close, so marking the page and stopping its loaders is
  // the whole operation.
  if (auto* main_local_frame = DynamicTo<LocalFrame>(main_frame_.Get()))
    main_local_frame->Loader().StopAllLoaders(/*abort_client=*/true);
}

ViewportDescription Page::GetViewportDescription() const {
  return MainFrame() && MainFrame()->IsLocalFrame() &&
                 DeprecatedLocalMainFrame()->GetDocument()
             ? DeprecatedLocalMainFrame()
                   ->GetDocument()
                   ->GetViewportData()
                   .GetViewportDescription()
             : ViewportDescription();
}

PageScaleConstraintsSet& Page::GetPageScaleConstraintsSet() {
  return *page_scale_constraints_set_;
}

const PageScaleConstraintsSet& Page::GetPageScaleConstraintsSet() const {
  return *page_scale_constraints_set_;
}

BrowserControls& Page::GetBrowserControls() {
  return *browser_controls_;
}

const BrowserControls& Page::GetBrowserControls() const {
  return *browser_controls_;
}

ConsoleMessageStorage& Page::GetConsoleMessageStorage() {
  return *console_message_storage_;
}

const ConsoleMessageStorage& Page::GetConsoleMessageStorage() const {
  return *console_message_storage_;
}

TopDocumentRootScrollerController& Page::GlobalRootScrollerController() const {
  return *global_root_scroller_controller_;
}

VisualViewport& Page::GetVisualViewport() {
  return *visual_viewport_;
}

const VisualViewport& Page::GetVisualViewport() const {
  return *visual_viewport_;
}

void Page::SetMainFrame(Frame* main_frame) {
  main_frame_ = main_frame;
  LinkRelatedPagesIfNeeded();
}

void Page::LinkRelatedPagesIfNeeded() {
  // Don't link if there's no opener, or if this page is already linked to other
  // pages, or if the opener is being detached (its related pages has been set
  // to null).
  if (!opener_ || prev_related_page_ != this || next_related_page_ != this ||
      !opener_->next_related_page_) {
    return;
  }
  // Before: ... -> opener -> next -> ...
  // After: ... -> opener -> page -> next -> ...
  Page* next = opener_->next_related_page_;
  opener_->next_related_page_ = this;
  prev_related_page_ = opener_;
  next_related_page_ = next;
  next->prev_related_page_ = this;
}

void Page::TakePropertiesForLocalMainFrameSwap(Page* old_page) {
  CHECK_EQ(prev_related_page_, this);
  CHECK_EQ(next_related_page_, this);

  // Make the related pages list include `this` in place of `old_page`.
  if (old_page->prev_related_page_ != old_page) {
    prev_related_page_ = old_page->prev_related_page_;
    prev_related_page_->next_related_page_ = this;
    old_page->prev_related_page_ = old_page;
  }
  if (old_page->next_related_page_ != old_page) {
    next_related_page_ = old_page->next_related_page_;
    next_related_page_->prev_related_page_ = this;
    old_page->next_related_page_ = old_page;
  }

  // If the previous page is an opener for other pages, make sure that the
  // openees point to the new page instead.
  for (auto& page : RelatedPages()) {
    if (page->opener_ == old_page) {
      page->opener_ = this;
    }
  }

  // Note that we don't update the `opener_` member here, since the
  // renderer-side opener is only set during construction and might be stale.
  // When we create the new page, we get the latest opener frame token, so the
  // new page's opener should be the most up-to-date opener.
}

LocalFrame* Page::DeprecatedLocalMainFrame() const {
  return To<LocalFrame>(main_frame_.Get());
}

SVGDocumentResourceTracker& Page::GetSVGDocumentResourceTracker() {
  if (!svg_document_resource_tracker_) {
    svg_document_resource_tracker_ =
        SVGResourceSchedulerRegistry::GetTracker(GetAgentGroupScheduler());
  }
  return *svg_document_resource_tracker_;
}

void Page::UsesOverlayScrollbarsChanged() {
  for (Page* page : AllPages()) {
    for (Frame* frame = page->MainFrame(); frame;
         frame = frame->Tree().TraverseNext()) {
      if (auto* local_frame = DynamicTo<LocalFrame>(frame)) {
        if (LocalFrameView* view = local_frame->View()) {
          view->UsesOverlayScrollbarsChanged();
        }
      }
    }
  }
}

void Page::ForcedColorsChanged() {
  PlatformColorsChanged();
  ColorSchemeChanged();
}

void Page::PlatformColorsChanged() {
  for (const Page* page : AllPages()) {
    for (Frame* frame = page->MainFrame(); frame;
         frame = frame->Tree().TraverseNext()) {
      if (auto* local_frame = DynamicTo<LocalFrame>(frame)) {
        if (Document* document = local_frame->GetDocument()) {
          document->PlatformColorsChanged();
        }
        if (LayoutView* view = local_frame->ContentLayoutObject())
          view->InvalidatePaintForViewAndDescendants();
      }
    }
  }
}

void Page::ColorSchemeChanged() {
  for (const Page* page : AllPages())
    for (Frame* frame = page->MainFrame(); frame;
         frame = frame->Tree().TraverseNext()) {
      if (auto* local_frame = DynamicTo<LocalFrame>(frame)) {
        if (Document* document = local_frame->GetDocument()) {
          document->ColorSchemeChanged();
        }
      }
    }
}

bool Page::UpdateColorProviders(
    const ColorProviderColorMaps& color_provider_colors) {
  // Color maps should not be empty as they are needed to create the color
  // providers.
  CHECK(!color_provider_colors.IsEmpty());

  bool did_color_provider_update = false;
  if (!ui::IsRendererColorMappingEquivalent(
          light_color_provider_.get(),
          color_provider_colors.light_colors_map)) {
    light_color_provider_ = ui::CreateColorProviderFromRendererColorMap(
        color_provider_colors.light_colors_map);
    did_color_provider_update = true;
  }
  if (!ui::IsRendererColorMappingEquivalent(
          dark_color_provider_.get(), color_provider_colors.dark_colors_map)) {
    dark_color_provider_ = ui::CreateColorProviderFromRendererColorMap(
        color_provider_colors.dark_colors_map);
    did_color_provider_update = true;
  }
  if (!ui::IsRendererColorMappingEquivalent(
          forced_colors_color_provider_.get(),
          color_provider_colors.forced_colors_map)) {
    forced_colors_color_provider_ =
        WebTestSupport::IsRunningWebTest()
            ? ui::CreateEmulatedForcedColorsColorProviderForTest()
            : ui::CreateColorProviderFromRendererColorMap(
                  color_provider_colors.forced_colors_map);
    did_color_provider_update = true;
  }

  if (did_color_provider_update) {
    color_provider_colors_ = color_provider_colors;
  }

  return did_color_provider_update;
}

const ui::ColorProvider* Page::GetColorProviderForPainting(
    mojom::blink::ColorScheme color_scheme,
    bool in_forced_colors) const {
  // All providers should be initialized and non-null before this function is
  // called.
  CHECK(light_color_provider_);
  CHECK(dark_color_provider_);
  CHECK(forced_colors_color_provider_);
  if (in_forced_colors) {
    return forced_colors_color_provider_.get();
  }

  return color_scheme == mojom::blink::ColorScheme::kDark
             ? dark_color_provider_.get()
             : light_color_provider_.get();
}

void Page::InitialStyleChanged() {
  for (Frame* frame = MainFrame(); frame;
       frame = frame->Tree().TraverseNext()) {
    auto* local_frame = DynamicTo<LocalFrame>(frame);
    if (!local_frame)
      continue;
    local_frame->GetDocument()->GetStyleEngine().InitialStyleChanged();
  }
}

void Page::UAStyleChanged() {
  for (Frame* frame = MainFrame(); frame;
       frame = frame->Tree().TraverseNext()) {
    if (auto* local_frame = DynamicTo<LocalFrame>(frame)) {
      local_frame->GetDocument()->GetStyleEngine().UAStyleChanged();
    }
  }
}

static void RestoreSVGImageAnimations() {
  for (const Page* page : AllPages()) {
    if (auto* svg_image_chrome_client =
            DynamicTo<IsolatedSVGChromeClient>(page->GetChromeClient())) {
      svg_image_chrome_client->RestoreAnimationIfNeeded();
    }
  }
}

void Page::SetPaused(bool paused) {
  if (paused == paused_)
    return;

  paused_ = paused;
  for (Frame* frame = MainFrame(); frame;
       frame = frame->Tree().TraverseNext()) {
    if (auto* local_frame = DynamicTo<LocalFrame>(frame)) {
      local_frame->OnPageLifecycleStateUpdated();
    }
  }
}

void Page::SetPageScaleFactor(float scale) {
  GetVisualViewport().SetScale(scale);
}

float Page::PageScaleFactor() const {
  return GetVisualViewport().Scale();
}

void Page::SetVisibilityState(
    mojom::blink::PageVisibilityState visibility_state,
    bool is_initial_state) {
  if (lifecycle_state_->visibility == visibility_state)
    return;

  // Are we entering / leaving a state that would map to the "visible" state, in
  // the `document.visibilityState` sense?
  const bool was_visible = lifecycle_state_->visibility ==
                           mojom::blink::PageVisibilityState::kVisible;
  const bool is_visible =
      visibility_state == mojom::blink::PageVisibilityState::kVisible;

  lifecycle_state_->visibility = visibility_state;

  if (is_initial_state)
    return;

  for (auto& observer : page_visibility_observer_set_) {
    observer->PageVisibilityChanged();
  }

  if (main_frame_) {
    if (lifecycle_state_->visibility ==
        mojom::blink::PageVisibilityState::kVisible) {
      RestoreSVGImageAnimations();
    }
    // If we're eliding visibility transitions between the two `kHidden*`
    // states, then we never get here unless one state was `kVisible` and the
    // other was not.  However, if we aren't eliding those transitions, then we
    // need to do so now; from the Frame's point of view, nothing is changing if
    // this is a change between the two `kHidden*` states.  Both map to "hidden"
    // in the sense of `document.visibilityState`, and dispatching an event when
    // the web-exposed state hasn't changed is confusing.
    //
    // This check could be enabled for both cases, and the result in the
    // "eliding" case shouldn't change.  It's not, just to be safe, since this
    // is intended as a fall-back to previous behavior.
    if (!RuntimeEnabledFeatures::DispatchHiddenVisibilityTransitionsEnabled() ||
        was_visible || is_visible) {
      main_frame_->DidChangeVisibilityState();
    }

    // Ensure that autoscrolling ends whenever the page transitions to
    // non-visible.
    if (!is_visible) {
      GetAutoscrollController().StopMiddleClickAutoscroll(
          DynamicTo<LocalFrame>(GetFocusController().FocusedOrMainFrame()));
    }
  }
}

bool Page::IsPageVisible() const {
  return lifecycle_state_->visibility ==
         mojom::blink::PageVisibilityState::kVisible;
}

bool Page::DispatchedPagehideAndStillHidden() {
  return lifecycle_state_->pagehide_dispatch !=
         mojom::blink::PagehideDispatch::kNotDispatched;
}

bool Page::DispatchedPagehidePersistedAndStillHidden() {
  return lifecycle_state_->pagehide_dispatch ==
         mojom::blink::PagehideDispatch::kDispatchedPersisted;
}

void Page::OnSetPageFrozen(bool frozen) {
  if (frozen_ == frozen)
    return;
  frozen_ = frozen;

  for (Frame* frame = main_frame_.Get(); frame;
       frame = frame->Tree().TraverseNext()) {
    if (auto* local_frame = DynamicTo<LocalFrame>(frame)) {
      local_frame->OnPageLifecycleStateUpdated();
    }
  }
}

// static
int Page::MaxNumberOfFrames() {
  return kMaxNumberOfFrames;
}

// static
#if DCHECK_IS_ON()
void CheckFrameCountConsistency(int expected_frame_count, Frame* frame) {
  DCHECK_GE(expected_frame_count, 0);

  int actual_frame_count = 0;

  // This used to additionally count each local frame's ``DocumentFencedFrames``
  // (fenced frames are not part of the regular frame tree walked below).
  // Fenced frames were removed along with core/html/fenced_frame/, so a plain
  // tree walk now accounts for every frame.
  for (; frame; frame = frame->Tree().TraverseNext()) {
    ++actual_frame_count;
  }

  DCHECK_EQ(expected_frame_count, actual_frame_count);
}
#endif

int Page::SubframeCount() const {
#if DCHECK_IS_ON()
  CheckFrameCountConsistency(subframe_count_ + 1, MainFrame());
#endif
  return subframe_count_;
}

void Page::SettingsChanged(ChangeType change_type) {
  switch (change_type) {
    case ChangeType::kStyle:
      InitialStyleChanged();
      break;
    case ChangeType::kViewportDescription:
      if (MainFrame() && MainFrame()->IsLocalFrame()) {
        DeprecatedLocalMainFrame()
            ->GetDocument()
            ->GetViewportData()
            .UpdateViewportDescription();
      }
      break;
    case ChangeType::kViewportPaintProperties:
      if (GetVisualViewport().IsActiveViewport()) {
        GetVisualViewport().SetNeedsPaintPropertyUpdate();
      }
      if (auto* local_frame = DynamicTo<LocalFrame>(MainFrame())) {
        if (LocalFrameView* view = local_frame->View())
          view->SetNeedsPaintPropertyUpdate();
      }
      break;
    case ChangeType::kDNSPrefetching:
      for (Frame* frame = MainFrame(); frame;
           frame = frame->Tree().TraverseNext()) {
        if (auto* local_frame = DynamicTo<LocalFrame>(frame))
          local_frame->GetDocument()->InitDNSPrefetch();
      }
      break;
    case ChangeType::kImageLoading:
      for (Frame* frame = MainFrame(); frame;
           frame = frame->Tree().TraverseNext()) {
        if (auto* local_frame = DynamicTo<LocalFrame>(frame)) {
          // Notify the fetcher that the image loading setting has changed,
          // which may cause previously deferred requests to load.
          local_frame->GetDocument()->Fetcher()->ReloadImagesIfNotDeferred();
          local_frame->GetDocument()->Fetcher()->SetAutoLoadImages(
              GetSettings().GetLoadsImagesAutomatically());
        }
      }
      break;
    case ChangeType::kFontScaleFactor:

      for (Frame* frame = MainFrame(); frame;
           frame = frame->Tree().TraverseNext()) {
        LocalFrame* local_frame = DynamicTo<LocalFrame>(frame);
        if (!local_frame) {
          continue;
        }
        Document* document = local_frame->GetDocument();
        if (!document || !document->IsActive()) {
          continue;
        }
        document->GetStyleEngine()
            .EnsureEnvironmentVariables()
            .UpdatePreferredTextScaleFromDocument();
        if (document->TextScaleMetaTagPresent()) {
          document->GetStyleEngine().InitialStyleChanged();
        }
      }
      break;
    case ChangeType::kFontFamily:
      for (Frame* frame = MainFrame(); frame;
           frame = frame->Tree().TraverseNext()) {
        if (auto* local_frame = DynamicTo<LocalFrame>(frame))
          local_frame->GetDocument()
              ->GetStyleEngine()
              .UpdateGenericFontFamilySettings();
      }
      break;
    case ChangeType::kAcceleratedCompositing:
      UpdateAcceleratedCompositingSettings();
      break;
    case ChangeType::kMediaQuery:
      for (Frame* frame = MainFrame(); frame;
           frame = frame->Tree().TraverseNext()) {
        if (auto* local_frame = DynamicTo<LocalFrame>(frame)) {
          local_frame->GetDocument()->MediaQueryAffectingValueChanged(
              MediaValueChange::kOther);
          // Used to notify the script-exposed navigator.preferences object
          // (NavigatorPreferences) so it could fire a change event. That
          // Navigator supplement was a JS API surface and is gone with V8.
        }
      }
      break;
    case ChangeType::kAccessibilityState:
      if (!MainFrame() || !MainFrame()->IsLocalFrame()) {
        break;
      }
      // Used to refresh the accessibility tree; no accessibility tree
      // exists to refresh anymore.
      break;
    case ChangeType::kViewportStyle: {
      auto* main_local_frame = DynamicTo<LocalFrame>(MainFrame());
      if (!main_local_frame)
        break;
      if (Document* doc = main_local_frame->GetDocument())
        doc->GetStyleEngine().ViewportStyleSettingChanged();
      break;
    }
    case ChangeType::kTextTrackKindUserPreference:
      for (Frame* frame = MainFrame(); frame;
           frame = frame->Tree().TraverseNext()) {
        if (auto* local_frame = DynamicTo<LocalFrame>(frame)) {
          Document* doc = local_frame->GetDocument();
          if (doc)
            HTMLMediaElement::SetTextTrackKindUserPreferenceForAllMediaElements(
                doc);
        }
      }
      break;
    case ChangeType::kDOMWorlds:
      // Used to forcibly instantiate the main-world WindowProxy when
      // Settings::ForceMainWorldInitialization was set. There is no script
      // context to instantiate any more.
      break;
    case ChangeType::kMediaControls:
      for (Frame* frame = MainFrame(); frame;
           frame = frame->Tree().TraverseNext()) {
        auto* local_frame = DynamicTo<LocalFrame>(frame);
        if (!local_frame)
          continue;
        Document* doc = local_frame->GetDocument();
        if (doc)
          HTMLMediaElement::OnMediaControlsEnabledChange(doc);
      }
      break;
    case ChangeType::kPaint: {
      InvalidatePaint();
      break;
    }
    case ChangeType::kScrollbarLayout: {
      for (Frame* frame = MainFrame(); frame;
           frame = frame->Tree().TraverseNext()) {
        auto* local_frame = DynamicTo<LocalFrame>(frame);
        if (!local_frame)
          continue;
        // Iterate through all of the scrollable areas and mark their layout
        // objects for layout.
        if (LocalFrameView* view = local_frame->View()) {
          for (const auto& scrollable_area : view->ScrollableAreas().Values()) {
            if (scrollable_area->ScrollsOverflow()) {
              if (auto* layout_box = scrollable_area->GetLayoutBox()) {
                layout_box->SetNeedsLayout(
                    layout_invalidation_reason::kScrollbarChanged);
              }
            }
          }
        }
      }
      break;
    }
    case ChangeType::kColorScheme:
      InvalidateColorScheme();
      break;
    case ChangeType::kUniversalAccess: {
      if (!GetSettings().GetAllowUniversalAccessFromFileURLs())
        break;
      for (Frame* frame = MainFrame(); frame;
           frame = frame->Tree().TraverseNext()) {
        // If we got granted universal access from file urls we need to grant
        // any outstanding security origin cross agent cluster access since
        // newly allocated agent clusters will be the universal agent.
        if (auto* local_frame = DynamicTo<LocalFrame>(frame)) {
          auto* window = local_frame->DomWindow();
          window->GetMutableSecurityOrigin()->GrantCrossAgentClusterAccess();
        }
      }
      break;
    }
    case ChangeType::kForcedColors: {
      ForcedColorsChanged();
      break;
    }
    case ChangeType::kAcceptLanguages:
      AcceptLanguagesChanged();
      break;
    case ChangeType::kTextTrackStyle:
      CSSDefaultStyleSheets::Instance().ResetTextTrackStyleSheet();
      UAStyleChanged();
      break;
  }
}

void Page::InvalidateColorScheme() {
  for (Frame* frame = MainFrame(); frame;
       frame = frame->Tree().TraverseNext()) {
    if (auto* local_frame = DynamicTo<LocalFrame>(frame)) {
      local_frame->GetDocument()->ColorSchemeChanged();
      // Used to notify the script-exposed navigator.preferences object
      // (NavigatorPreferences) so it could fire a change event. That
      // Navigator supplement was a JS API surface and is gone with V8.
    }
  }
}

void Page::InvalidatePaint() {
  for (Frame* frame = MainFrame(); frame;
       frame = frame->Tree().TraverseNext()) {
    auto* local_frame = DynamicTo<LocalFrame>(frame);
    if (!local_frame)
      continue;
    if (LayoutView* view = local_frame->ContentLayoutObject())
      view->InvalidatePaintForViewAndDescendants();
  }
}

void Page::UpdateAcceleratedCompositingSettings() {
  for (Frame* frame = MainFrame(); frame;
       frame = frame->Tree().TraverseNext()) {
    auto* local_frame = DynamicTo<LocalFrame>(frame);
    if (!local_frame)
      continue;
    // Mark all scrollable areas as needing a paint property update because the
    // compositing reasons may have changed.
    if (LocalFrameView* view = local_frame->View()) {
      for (const auto& scrollable_area : view->ScrollableAreas().Values()) {
        if (scrollable_area->ScrollsOverflow()) {
          if (auto* layout_box = scrollable_area->GetLayoutBox()) {
            layout_box->SetNeedsPaintPropertyUpdate();
          }
        }
      }
    }
  }
}

void Page::DidCommitLoad(LocalFrame* frame) {
  if (main_frame_ == frame) {
    GetConsoleMessageStorage().Clear();
    // TODO(loonybear): Most of this doesn't appear to take into account that
    // each SVGImage gets it's own Page instance.
    GetDeprecation().ClearSuppression();
    // Need to reset visual viewport position here since before commit load we
    // would update the previous history item, Page::didCommitLoad is called
    // after a new history item is created in FrameLoader.
    // See crbug.com/642279
    GetVisualViewport().SetScrollOffset(
        ScrollOffset(), mojom::blink::ScrollType::kProgrammatic,
        cc::ScrollSourceType::kNone, mojom::blink::ScrollBehavior::kInstant);
  }
  // A LocalFrame::UpdateAdHighlight() call was here, to re-apply DevTools'
  // "Highlight ads" setting after a commit that races the setting change.
  // The overlay is gone with ad tagging.
}

void Page::AcceptLanguagesChanged() {
  HeapVector<Member<LocalFrame>> frames;

  // Even though we don't fire an event from here, the LocalDOMWindow's will
  // fire an event so we keep the frames alive until we are done.
  for (Frame* frame = MainFrame(); frame;
       frame = frame->Tree().TraverseNext()) {
    if (auto* local_frame = DynamicTo<LocalFrame>(frame))
      frames.push_back(local_frame);
  }

  for (unsigned i = 0; i < frames.size(); ++i)
    frames[i]->DomWindow()->AcceptLanguagesChanged();
}

void Page::Trace(Visitor* visitor) const {
  visitor->Trace(animator_);
  visitor->Trace(autoscroll_controller_);
  visitor->Trace(chrome_client_);
  visitor->Trace(drag_caret_);
  visitor->Trace(focus_controller_);
  visitor->Trace(page_scale_constraints_set_);
  visitor->Trace(page_visibility_observer_set_);
  visitor->Trace(browser_controls_);
  visitor->Trace(console_message_storage_);
  visitor->Trace(global_root_scroller_controller_);
  visitor->Trace(visual_viewport_);
  visitor->Trace(svg_document_resource_tracker_);
  visitor->Trace(main_frame_);
  visitor->Trace(previous_main_frame_for_local_swap_);
  visitor->Trace(next_related_page_);
  visitor->Trace(prev_related_page_);
  visitor->Trace(agent_group_scheduler_);
  visitor->Trace(opener_);
  Supplementable<Page>::Trace(visitor);
}

void Page::WillBeDestroyed() {
  Frame* main_frame = main_frame_;

  // TODO(https://crbug.com/838348): Sadly, there are situations where Blink may
  // attempt to detach a main frame twice due to a bug. That rewinds
  // FrameLifecycle from kDetached to kDetaching, but GetPage() will already be
  // null. Since Detach() has already happened, just skip the actual Detach()
  // call to try to limit the side effects of this bug on the rest of frame
  // detach.
  if (main_frame->GetPage()) {
    main_frame->Detach(FrameDetachType::kRemove);
  }

  // Only begin clearing state after JS has run, since running JS itself can
  // sometimes alter Page's state.
  DCHECK(AllPages().Contains(this));
  AllPages().erase(this);
  OrdinaryPages().erase(this);

  {
    // Before: ... -> prev -> this -> next -> ...
    // After: ... -> prev -> next -> ...
    // (this is ok even if |this| is the only element on the list).
    Page* prev = prev_related_page_;
    Page* next = next_related_page_;
    next->prev_related_page_ = prev;
    prev->next_related_page_ = next;
    prev_related_page_ = nullptr;
    next_related_page_ = nullptr;
  }


  GetChromeClient().ChromeDestroyed();
  main_frame_ = nullptr;

  for (auto& observer : page_visibility_observer_set_) {
    observer->ObserverSetWillBeCleared();
  }
  page_visibility_observer_set_.clear();

  page_scheduler_ = nullptr;
}

ScrollbarTheme& Page::GetScrollbarTheme() const {
  if (settings_->GetForceAndroidOverlayScrollbar())
    return ScrollbarThemeOverlayMobile::GetInstance();

  // Ensures that renderer preferences are set.
  DCHECK(main_frame_);
  return ScrollbarTheme::GetTheme();
}

AgentGroupScheduler& Page::GetAgentGroupScheduler() const {
  return *agent_group_scheduler_;
}

PageScheduler* Page::GetPageScheduler() const {
  DCHECK(page_scheduler_);
  return page_scheduler_.get();
}

bool Page::IsOrdinary() const {
  return is_ordinary_;
}

void Page::SetIsMainFrameFencedFrameRoot() {
  is_fenced_frame_tree_ = true;
}

bool Page::IsMainFrameFencedFrameRoot() const {
  return is_fenced_frame_tree_;
}

void Page::Animate(base::TimeTicks monotonic_frame_begin_time) {
  GetAutoscrollController().Animate();
  Animator().ServiceScriptedAnimations(monotonic_frame_begin_time);
}

const base::UnguessableToken& Page::BrowsingContextGroupToken() {
  return browsing_context_group_token_;
}

void Page::UpdateBrowsingContextGroup(
    const base::UnguessableToken& browsing_context_group_token) {
  browsing_context_group_token_ = browsing_context_group_token;
}

template class CORE_TEMPLATE_EXPORT Supplement<Page>;
// Ensure the 10 bits reserved for connected frame count in NodeRareData are
// sufficient.
static_assert(kMaxNumberOfFrames <
                  (1 << NodeRareData::kConnectedFrameCountBits),
              "Frame limit should fit in rare data count");

}  // namespace blink
