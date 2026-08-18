// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/accessibility/scoped_blink_ax_event_intent.h"

namespace blink {

// ScopedBlinkAXEventIntent used to forward the intents it carries into the
// active AXObjectCache's BlinkAXEventIntentsSet, so that accessibility
// events raised while it is alive could be annotated with them. The
// accessibility tree (AXObjectCache/AXObject) was removed for the
// screenshot engine -- a static render has no accessibility tree to
// annotate -- so there is no cache to forward intents to. The class is kept
// as a plain intent holder because editing code still constructs it around
// operations it wants to describe; ScopedBlinkAXEventIntentTest exercised
// the removed cache-forwarding behavior and was deleted along with it.

ScopedBlinkAXEventIntent::ScopedBlinkAXEventIntent(
    const BlinkAXEventIntent& intent,
    Document* document)
    : document_(document) {
  DCHECK(document_);
  DCHECK(document_->IsActive());

  if (!intent.is_initialized())
    return;
  intents_.push_back(intent);
}

ScopedBlinkAXEventIntent::ScopedBlinkAXEventIntent(
    const Vector<BlinkAXEventIntent>& intents,
    Document* document)
    : intents_(intents), document_(document) {
  DCHECK(document_);
  DCHECK(document_->IsActive());
}

ScopedBlinkAXEventIntent::~ScopedBlinkAXEventIntent() = default;

}  // namespace blink
