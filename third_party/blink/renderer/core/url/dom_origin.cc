// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/url/dom_origin.h"

#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

// Static `::Create()` methods:
DOMOrigin* DOMOrigin::Create() {
  return DOMOrigin::Create(SecurityOrigin::CreateUniqueOpaque());
}

// static
DOMOrigin* DOMOrigin::Create(scoped_refptr<const SecurityOrigin> origin) {
  return MakeGarbageCollected<DOMOrigin>(base::PassKey<DOMOrigin>(),
                                         std::move(origin));
}

// Constructor
DOMOrigin::DOMOrigin(base::PassKey<DOMOrigin>,
                     scoped_refptr<const SecurityOrigin> origin)
    : origin_(std::move(origin)) {}

// Destructor
DOMOrigin::~DOMOrigin() = default;


bool DOMOrigin::opaque() const {
  return origin_->IsOpaque();
}

bool DOMOrigin::isSameOrigin(const DOMOrigin* other) const {
  return origin_->IsSameOriginWith(other->origin_.get());
}

bool DOMOrigin::isSameSite(const DOMOrigin* other) const {
  return origin_->IsSameSiteWith(other->origin_.get());
}

void DOMOrigin::Trace(Visitor* visitor) const {
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
