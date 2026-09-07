// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/page/page_animator.h"

#include "base/auto_reset.h"
#include "base/time/time.h"
#include "third_party/blink/renderer/core/animation/document_animations.h"
#include "third_party/blink/renderer/core/animation/document_timeline.h"
#include "third_party/blink/renderer/core/css/css_value.h"
#include "third_party/blink/renderer/core/dom/document_lifecycle.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/scripted_animation_controller.h"
#include "third_party/blink/renderer/core/event_interface_names.h"
#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/page/scrolling/sync_scroll_attempt_heuristic.h"
#include "third_party/blink/renderer/core/paint/paint_layer_scrollable_area.h"
#include "third_party/blink/renderer/core/svg/svg_document_extensions.h"
#include "third_party/blink/renderer/core/timing/time_clamper.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

namespace {

// We walk through all the frames in DOM tree order and get all the documents
DocumentsVector GetAllDocuments(Frame* main_frame) {
  DocumentsVector documents;
  for (Frame* frame = main_frame; frame; frame = frame->Tree().TraverseNext()) {
    if (auto* local_frame = DynamicTo<LocalFrame>(frame)) {
      Document* document = local_frame->GetDocument();
      bool can_throttle =
          document->View() && document->View()->CanThrottleRendering();
      documents.emplace_back(std::make_pair(document, can_throttle));
    }
  }
  return documents;
}

void UpdateAnimationsForDocument(
    Document* document,
    bool can_throttle,
    base::TimeTicks monotonic_animation_start_time) {
  if (!document->View()) {
    document->GetDocumentAnimations().UpdateAnimationTimingForAnimationFrame();
  } else if (!can_throttle) {
    document->View()->ServiceScrollAnimations(monotonic_animation_start_time);
  }
}

}  // namespace

PageAnimator::PageAnimator(Page& page)
    : page_(page),
      servicing_animations_(false),
      updating_layout_and_style_for_painting_(false) {}

void PageAnimator::Trace(Visitor* visitor) const {
  visitor->Trace(page_);
}

void PageAnimator::ServiceScriptedAnimations(
    base::TimeTicks monotonic_animation_start_time) {
  base::AutoReset<bool> servicing(&servicing_animations_, true);

  // Once we are inside a frame's lifecycle, the AnimationClock should hold its
  // time value until the end of the frame.
  Clock().SetAllowedToDynamicallyUpdateTime(false);
  Clock().UpdateTime(monotonic_animation_start_time);

  DocumentsVector documents = GetAllDocuments(page_->MainFrame());
  for (const auto& [document, can_throttle] : documents) {
    static TimeClamper time_clamper;
    base::TimeTicks animation_time = document->Timeline().CalculateZeroTime();
    if (monotonic_animation_start_time > animation_time) {
      animation_time += time_clamper.ClampTimeResolution(
          monotonic_animation_start_time - animation_time,
          document->domWindow()->CrossOriginIsolatedCapability());
    }
    document->GetAnimationClock().SetAllowedToDynamicallyUpdateTime(false);
    // TODO(crbug.com/1497922) timestamps outside rendering updates should also
    // be coarsened.
    document->GetAnimationClock().UpdateTime(animation_time);
  }

  TRACE_EVENT0("blink,rail", "PageAnimator::serviceScriptedAnimations");
  if (!RuntimeEnabledFeatures::EventTimingMatchingHTMLEnabled()) {
    for (const auto& [document, can_throttle] : documents) {
      UpdateAnimationsForDocument(document, can_throttle,
                                  monotonic_animation_start_time);
    }
  }

  ControllersVector controllers{};
  for (const auto& document : documents) {
    controllers.emplace_back(document.first->GetScriptedAnimationController(),
                             document.second);
  }
  // TODO(crbug.com/1499981): This should be removed once synchronized scrolling
  // impact is understood.
  SyncScrollAttemptHeuristic heuristic(page_->MainFrame());
  ServiceScriptedAnimations(monotonic_animation_start_time, controllers);
}

void PageAnimator::ServiceScriptedAnimations(
    base::TimeTicks monotonic_time_now,
    const ControllersVector& controllers) {
  Vector<wtf_size_t> active_controllers_ids{};
  HeapVector<Member<ScriptedAnimationController>> active_controllers{};
  for (wtf_size_t i = 0; i < controllers.size(); ++i) {
    auto& [controller, can_throttle] = controllers[i];
    if (!controller->GetExecutionContext() ||
        controller->GetExecutionContext()->IsContextFrozenOrPaused()) {
      continue;
    }

    LocalDOMWindow* window = controller->GetWindow();
    auto* loader = window->document()->Loader();
    if (!loader) {
      continue;
    }

    controller->SetCurrentFrameTimeMs(
        window->document()->Timeline().CurrentTimeMilliseconds().value());
    controller->SetCurrentFrameLegacyTimeMs(
        loader->GetTiming()
            .MonotonicTimeToPseudoWallTime(monotonic_time_now)
            .InMillisecondsF());
    if (can_throttle) {
      continue;
    }
    if (!controller->HasScheduledFrameTasks()) {
      continue;
    }
    active_controllers_ids.emplace_back(i);
    active_controllers.emplace_back(controller);
  }

  Vector<base::TimeDelta> time_intervals(active_controllers.size());
  // TODO(rendering-dev): calls to Now() are expensive on ARM architectures.
  // We can avoid some of these calls by filtering out calls to controllers
  // where the function() invocation won't do any work (e.g., because there
  // are no events to dispatch).
  const auto run_for_all_active_controllers_with_timing =
      [&](const auto& function) {
        auto start_time = base::TimeTicks::Now();
        for (wtf_size_t i = 0; i < active_controllers.size(); ++i) {
          function(i);
          auto end_time = base::TimeTicks::Now();
          time_intervals[i] += end_time - start_time;
          start_time = end_time;
        }
      };

  // Unlike run_for_all_active_controllers_with_timing, this iterates over all
  // controllers (not just active ones). We explicitly track timing: accumulate
  // elapsed time for active controllers, and report to the frame scheduler for
  // others.
  const auto run_for_all_controllers_with_timing = [&](const auto& function) {
    wtf_size_t active_controller_id = 0;
    auto start_time = base::TimeTicks::Now();
    for (wtf_size_t i = 0; i < controllers.size(); ++i) {
      function(i);
      auto end_time = base::TimeTicks::Now();
      if (active_controller_id < active_controllers_ids.size() &&
          i == active_controllers_ids[active_controller_id]) {
        time_intervals[active_controller_id++] += end_time - start_time;
      } else {
        // For non active controllers (e.g. which can throttle)
        // that's the only timing we need to measure.
        if (const auto* window = controllers[i].first->GetWindow()) {
          if (auto* frame = window->document()->GetFrame()) {
            frame->GetFrameScheduler()->AddTaskTime(end_time - start_time);
          }
        }
      }
      start_time = end_time;
    }
  };

  // https://html.spec.whatwg.org/multipage/webappapis.html#event-loop-processing-model

  // 7. For each doc of docs, flush autofocus candidates for doc if its node
  // navigable is a top-level traversable.
  run_for_all_active_controllers_with_timing([&](wtf_size_t i) {
    if (const auto* window = active_controllers[i]->GetWindow()) {
      window->document()->FlushAutofocusCandidates();
    }
  });

  // 8. For each doc of docs, run the resize steps for doc.
  run_for_all_controllers_with_timing([&](wtf_size_t i) {
    controllers[i].first->DispatchEvents(BindRepeating([](Event* event) {
      return event->type() == event_type_names::kResize;
    }));
  });

  // 9. For each doc of docs, run the scroll steps for doc.
  run_for_all_active_controllers_with_timing([&](wtf_size_t i) {
    auto scope = SyncScrollAttemptHeuristic::GetScrollHandlerScope();
    active_controllers[i]->DispatchEvents(BindRepeating([](Event* event) {
      return event->type() == event_type_names::kScroll ||
             event->type() == event_type_names::kScrollsnapchange ||
             event->type() == event_type_names::kScrollsnapchanging ||
             event->type() == event_type_names::kScrollend;
    }));
  });

  // 10. For each doc of docs, evaluate media queries and report changes for
  // doc.
  // CallMediaQueryListListeners handles legacy mql.addListener() callbacks,
  // while DispatchEvents handles standard mql.addEventListener('change')
  // events. Both must be dispatched in Step 10 per spec.
  run_for_all_active_controllers_with_timing([&](wtf_size_t i) {
    active_controllers[i]->CallMediaQueryListListeners();
    if (RuntimeEnabledFeatures::EventTimingMatchingHTMLEnabled()) {
      active_controllers[i]->DispatchEvents(BindRepeating([](Event* event) {
        return event->InterfaceName() ==
               event_interface_names::kMediaQueryListEvent;
      }));
    }
  });

  // 11. For each doc of docs, update animations and send events for doc,
  // passing in relative high resolution time given frameTimestamp and doc's
  // relevant global object as the timestamp.
  if (!RuntimeEnabledFeatures::EventTimingMatchingHTMLEnabled()) {
    run_for_all_active_controllers_with_timing(
        [&](wtf_size_t i) { active_controllers[i]->DispatchEvents(); });
  } else {
    run_for_all_controllers_with_timing([&](wtf_size_t i) {
      auto& [controller, can_throttle] = controllers[i];
      LocalDOMWindow* window = controller->GetWindow();
      Document* document = window ? window->document() : nullptr;
      if (document) {
        UpdateAnimationsForDocument(document, can_throttle, monotonic_time_now);
      }
      if (!can_throttle) {
        // While the HTML spec mentions only animation events for this step,
        // we are sending animation events as well as any event types not
        // covered by the HTML steps (e.g., <dialog> 'close' events or text
        // control 'select' and 'change' events).
        controller->DispatchEvents(BindRepeating([](Event* event) {
          return event->type() != event_type_names::kScroll &&
                 event->type() != event_type_names::kScrollsnapchange &&
                 event->type() != event_type_names::kScrollsnapchanging &&
                 event->type() != event_type_names::kScrollend &&
                 event->type() != event_type_names::kResize &&
                 event->InterfaceName() !=
                     event_interface_names::kMediaQueryListEvent;
        }));
      }
    });
  }

  // 12. For each doc of docs, run the fullscreen steps for doc.
  run_for_all_active_controllers_with_timing(
      [&](wtf_size_t i) { active_controllers[i]->RunTasks(); });

  // Run the fulfilled HTMLVideoELement.requestVideoFrameCallback() callbacks.
  // See https://wicg.github.io/video-rvfc/.
  run_for_all_active_controllers_with_timing([&](wtf_size_t i) {
    active_controllers[i]->ExecuteVideoFrameCallbacks();
  });

  // 14. For each doc of docs, run the animation frame callbacks for doc,
  // passing in the relative high resolution time given frameTimestamp and
  // doc's relevant global object as the timestamp.
  run_for_all_active_controllers_with_timing([&](wtf_size_t i) {
    auto scope = SyncScrollAttemptHeuristic::GetRequestAnimationFrameScope();
    active_controllers[i]->ExecuteFrameCallbacks();
    if (!active_controllers[i]->GetExecutionContext()) {
      return;
    }
    auto* animator = active_controllers[i]->GetPageAnimator();
    if (animator && active_controllers[i]->HasFrameCallback()) {
      animator->SetNextFrameHasPendingRaf();
    }
    // See LocalFrameView::RunPostLifecycleSteps() for 15 and 16.
    active_controllers[i]->ScheduleAnimationIfNeeded(
        cc::BeginMainFrameReason::kServiceScriptedAnimations);
  });

  // Add task timings.
  for (wtf_size_t i = 0; i < active_controllers.size(); ++i) {
    if (const auto* window = active_controllers[i]->GetWindow()) {
      if (auto* frame = window->document()->GetFrame()) {
        frame->GetFrameScheduler()->AddTaskTime(time_intervals[i]);
      }
    }
  }
}

void PageAnimator::PostAnimate() {
  // If we don't have an imminently incoming frame, we need to let the
  // AnimationClock update its own time to properly service out-of-lifecycle
  // events such as setInterval (see https://crbug.com/995806). This isn't a
  // perfect heuristic, but at the very least we know that if there is a pending
  // RAF we will be getting a new frame and thus don't need to unlock the clock.
  if (!next_frame_has_pending_raf_) {
    Clock().SetAllowedToDynamicallyUpdateTime(true);
    DocumentsVector documents = GetAllDocuments(page_->MainFrame());
    for (const auto& [document, can_throttle] : documents) {
      document->GetAnimationClock().SetAllowedToDynamicallyUpdateTime(true);
    }
  }
  next_frame_has_pending_raf_ = false;
}

void PageAnimator::SetSuppressFrameRequestsWorkaroundFor704763Only(
    bool suppress_frame_requests) {
  // If we are enabling the suppression and it was already enabled then we must
  // have missed disabling it at the end of a previous frame.
  DCHECK(!suppress_frame_requests_workaround_for704763_only_ ||
         !suppress_frame_requests);
  suppress_frame_requests_workaround_for704763_only_ = suppress_frame_requests;
}

void PageAnimator::SetNextFrameHasPendingRaf() {
  next_frame_has_pending_raf_ = true;
}

DISABLE_CFI_PERF
void PageAnimator::ScheduleVisualUpdate(LocalFrame* frame,
                                        cc::BeginMainFrameReason reason) {
  if (servicing_animations_ || updating_layout_and_style_for_painting_ ||
      suppress_frame_requests_workaround_for704763_only_) {
    return;
  }
  page_->GetChromeClient().ScheduleAnimation(frame->View(), reason);
}

void PageAnimator::UpdateAllLifecyclePhases(LocalFrame& root_frame,
                                            DocumentUpdateReason reason) {
  LocalFrameView* view = root_frame.View();
  base::AutoReset<bool> servicing(&updating_layout_and_style_for_painting_,
                                  true);
  view->UpdateAllLifecyclePhases(reason);
}

void PageAnimator::UpdateAllLifecyclePhasesExceptPaint(
    LocalFrame& root_frame,
    DocumentUpdateReason reason) {
  LocalFrameView* view = root_frame.View();
  base::AutoReset<bool> servicing(&updating_layout_and_style_for_painting_,
                                  true);
  view->UpdateAllLifecyclePhasesExceptPaint(reason);
}

void PageAnimator::UpdateLifecycleToLayoutClean(LocalFrame& root_frame,
                                                DocumentUpdateReason reason) {
  LocalFrameView* view = root_frame.View();
  base::AutoReset<bool> servicing(&updating_layout_and_style_for_painting_,
                                  true);
  view->UpdateLifecycleToLayoutClean(reason);
}

HeapVector<Member<Animation>> PageAnimator::GetAnimations(
    const TreeScope& tree_scope) {
  HeapVector<Member<Animation>> animations;
  DocumentsVector documents = GetAllDocuments(page_->MainFrame());
  for (auto& [document, can_throttle] : documents) {
    document->GetDocumentAnimations().GetAnimationsTargetingTreeScope(
        animations, tree_scope);
  }
  return animations;
}

}  // namespace blink
