/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/animation/pending_animations.h"

#include "base/auto_reset.h"
#include "third_party/blink/renderer/core/animation/document_timeline.h"
#include "third_party/blink/renderer/core/animation/keyframe_effect.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"

namespace blink {

void PendingAnimations::Add(Animation* animation) {
  DCHECK(animation);
  DCHECK_EQ(pending_.Find(animation), kNotFound);
  pending_.push_back(animation);

  Document* document = animation->GetDocument();
  if (document->View()) {
    document->View()->ScheduleAnimation(
        cc::BeginMainFrameReason::kCSSAnimation);
  }

  bool visible = document->GetPage() && document->GetPage()->IsPageVisible();
  if (!visible && !timer_.IsActive()) {
    // Verify the timer is not activated in cycles.
    CHECK(!inside_timer_fired_);
    timer_.StartOneShot(base::TimeDelta(), FROM_HERE);
  }
}

void PendingAnimations::Update(bool update_timing) {
  HeapVector<Member<Animation>> waiting_for_start_time;
  HeapVector<Member<Animation>> animations;
  HeapVector<Member<Animation>> deferred;
  animations.swap(pending_);

  for (auto& animation : animations) {
    bool has_monotonic_timeline =
        animation->TimelineInternal() &&
        animation->TimelineInternal()->IsMonotonicallyIncreasing();
    if (animation->PreparePendingUpdate(update_timing)) {
      if (!animation->TimelineInternal() ||
          !animation->TimelineInternal()->IsActive()) {
        continue;
      }
      if (animation->Playing() && !animation->StartTimeInternal()) {
        // Scroll timelines resolve their start time during snapshot validation.
        if (has_monotonic_timeline) {
          waiting_for_start_time.push_back(animation.Get());
        }
      } else if (animation->PendingInternal()) {
        if (!has_monotonic_timeline && !animation->CurrentTimeInternal()) {
          deferred.push_back(animation);
        } else {
          DCHECK(animation->TimelineInternal()->CurrentTime() &&
                 animation->CurrentTimeInternal());
          animation->NotifyReady(
              animation->TimelineInternal()->CurrentTime().value());
        }
      }
    } else if (animation->CurrentTimeInternal()) {
      deferred.push_back(animation);
    }
  }

  for (auto& animation : waiting_for_start_time) {
    DCHECK(!animation->StartTimeInternal());
    DCHECK(animation->TimelineInternal()->IsActive() &&
           animation->TimelineInternal()->CurrentTime());
    animation->NotifyReady(
        animation->TimelineInternal()->CurrentTime().value());
  }

  for (auto& animation : animations) {
    animation->CompletePendingUpdate();
  }

  DCHECK(pending_.empty());
  DCHECK(update_timing || deferred.empty());
  for (auto& animation : deferred) {
    animation->SetPendingUpdate();
  }
  DCHECK_EQ(pending_.size(), deferred.size());
}

void PendingAnimations::Trace(Visitor* visitor) const {
  visitor->Trace(pending_);
  visitor->Trace(timer_);
}

void PendingAnimations::TimerFired(TimerBase*) {
  base::AutoReset<bool> mark_inside(&inside_timer_fired_, true);
  Update(false);
}

}  // namespace blink
