// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/timing/not_restored_reasons.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace blink {
NotRestoredReasons::NotRestoredReasons(
    String src,
    String id,
    String name,
    String url,
    HeapVector<Member<NotRestoredReasonDetails>>* reasons,
    HeapVector<Member<NotRestoredReasons>>* children)
    : src_(src), id_(id), name_(name), url_(url) {
  if (reasons) {
    for (const auto& reason : *reasons) {
      reasons_.push_back(reason);
    }
  }
  if (children) {
    for (const auto& child : *children) {
      children_.push_back(child);
    }
  }
}

void NotRestoredReasons::Trace(Visitor* visitor) const {
  visitor->Trace(reasons_);
  visitor->Trace(children_);
  ScriptWrappable::Trace(visitor);
}

NotRestoredReasons::NotRestoredReasons(const NotRestoredReasons& other)
    : src_(other.src_),
      id_(other.id_),
      name_(other.name_),
      url_(other.url_),
      reasons_(other.reasons_),
      children_(other.children_) {}

const std::optional<HeapVector<Member<NotRestoredReasonDetails>>>
NotRestoredReasons::reasons() const {
  if (!url_) {
    // If `url_` is null, this is for cross-origin and reasons should be masked.
    return std::nullopt;
  }
  return reasons_;
}

const std::optional<HeapVector<Member<NotRestoredReasons>>>
NotRestoredReasons::children() const {
  if (!url_) {
    // If `url_` is null, this is for cross-origin and children should be
    // masked.
    return std::nullopt;
  }
  return children_;
}

}  // namespace blink
