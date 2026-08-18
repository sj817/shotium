// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/view_transition/page_swap_event.h"

#include "third_party/blink/public/common/page_state/page_state.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_page_swap_event_init.h"
#include "third_party/blink/renderer/core/event_interface_names.h"
#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/frame/dom_window.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/loader/history_item.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/uuid.h"

namespace blink {

// The pageswap event fires on the outgoing Document of a cross-document
// navigation that has a view transition. Everything it used to carry came from
// the Navigation API: it looked up the NavigationHistoryEntry the navigation
// was heading to (creating one for push/replace, finding the existing one for
// traverse, reusing the current one for reload) and wrapped it, plus the entry
// being left, in a NavigationActivation. core/navigation_api is cut, so the
// constructor has nothing left to compute and the event is now type-only.
//
// It is kept rather than deleted because it is still dispatched -- by
// LocalFrameMojoHandler::DispatchPageSwap and ViewTransitionSupplement -- and
// an event with no attributes is a smaller and more honest change than
// unpicking those two call paths in the same pass.

PageSwapEvent::PageSwapEvent(
    Document& document,
    mojom::blink::PageSwapEventParamsPtr page_swap_event_params)
    : Event(event_type_names::kPageswap, Bubbles::kNo, Cancelable::kNo) {
  CHECK(RuntimeEnabledFeatures::PageSwapEventEnabled());
}

PageSwapEvent::PageSwapEvent(const AtomicString& type,
                             const PageSwapEventInit* initializer)
    : Event(type, initializer) {}

PageSwapEvent::~PageSwapEvent() = default;

const AtomicString& PageSwapEvent::InterfaceName() const {
  return event_interface_names::kPageSwapEvent;
}

void PageSwapEvent::Trace(Visitor* visitor) const {
  Event::Trace(visitor);
}

}  // namespace blink
