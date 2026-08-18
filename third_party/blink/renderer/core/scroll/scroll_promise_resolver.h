// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SCROLL_SCROLL_PROMISE_RESOLVER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SCROLL_SCROLL_PROMISE_RESOLVER_H_

#include <memory>

#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

// Tracks when all active scrollers for one programmatic scroll request are
// done scrolling, by handing out one `ActiveScrollTracker` per affected
// scroller and counting them back in.
//
// This used to end in a JS promise: it held a
// script promise resolver for a ScrollResult and resolved it, with
// `ScrollResult.interrupted` set, once the last tracker went away. There is
// no JS left to hand a promise to, so the resolver and `CreateScriptPromise()`
// are gone; what remains is the tracker plumbing itself, which is threaded
// through ScrollableArea/RootFrameViewport/PaintLayerScrollableArea
// signatures across core/. Nothing observes the completion any more.
//
// TODO: once the last `CreateScriptPromise()` caller in core/frame is gone,
// this class and every `ActiveScrollTracker` parameter it is threaded through
// can be deleted outright.
class ScrollPromiseResolver : public GarbageCollected<ScrollPromiseResolver> {
 public:
  // An RAII-style inner class that tracks active (pending/ongoing) scrolling on
  // a particular scroller. A `Element.scrollIntoView` request can create
  // multiple instances of this while any other request creates at most one.
  class ActiveScrollTracker {
   public:
    explicit ActiveScrollTracker(ScrollPromiseResolver* scroll_promise_resolver)
        : scroll_promise_resolver_(scroll_promise_resolver) {}

    ~ActiveScrollTracker() {
      scroll_promise_resolver_->ActiveScrollTrackerRemoved();
    }

    void MarkInterrupted() {
      scroll_promise_resolver_->scroll_is_interrupted_ = true;
    }

   private:
    Persistent<ScrollPromiseResolver> scroll_promise_resolver_;
  };

  ScrollPromiseResolver();
  ~ScrollPromiseResolver();

  // Returns a tracker for an active scroll.
  std::unique_ptr<ActiveScrollTracker> CreateActiveScrollTracker();

  // True once every tracker handed out has been destroyed.
  bool IsIdle() const { return num_active_scrolls_ == 0; }

  // True if any of the scrolls this request started was interrupted.
  bool ScrollWasInterrupted() const { return scroll_is_interrupted_; }

  void Trace(Visitor* visitor) const;

 private:
  void ActiveScrollTrackerRemoved();

  wtf_size_t num_active_scrolls_ = 0;
  bool scroll_is_interrupted_ = false;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_SCROLL_SCROLL_PROMISE_RESOLVER_H_
