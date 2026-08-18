// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/highlight/highlight.h"

#include "base/notimplemented.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/highlight/highlight_registry.h"

namespace blink {

Highlight* Highlight::Create(const HeapVector<Member<AbstractRange>>& ranges) {
  return MakeGarbageCollected<Highlight>(ranges);
}

Highlight::Highlight(const HeapVector<Member<AbstractRange>>& ranges) {
  for (const auto& range : ranges)
    highlight_ranges_.insert(range);
}

Highlight::~Highlight() = default;

void Highlight::Trace(blink::Visitor* visitor) const {
  visitor->Trace(highlight_ranges_);
  visitor->Trace(containing_highlight_registries_);
  ScriptWrappable::Trace(visitor);
}

void Highlight::ScheduleRepaintsInContainingHighlightRegistries() const {
  for (const auto& entry : containing_highlight_registries_) {
    DCHECK_GT(entry.value, 0u);
    Member<HighlightRegistry> highlight_registry = entry.key;
    highlight_registry->ScheduleRepaint();
  }
}

wtf_size_t Highlight::size() const {
  return highlight_ranges_.size();
}

void Highlight::setPriority(const int32_t& priority) {
  priority_ = priority;
  ScheduleRepaintsInContainingHighlightRegistries();
}

bool Highlight::Contains(AbstractRange* range) const {
  return highlight_ranges_.Contains(range);
}

void Highlight::RegisterIn(HighlightRegistry* highlight_registry) {
  auto map_iterator = containing_highlight_registries_.find(highlight_registry);
  if (map_iterator == containing_highlight_registries_.end()) {
    containing_highlight_registries_.insert(highlight_registry, 1);
  } else {
    DCHECK_GT(map_iterator->value, 0u);
    map_iterator->value++;
  }
}

void Highlight::DeregisterFrom(HighlightRegistry* highlight_registry) {
  auto map_iterator = containing_highlight_registries_.find(highlight_registry);
  CHECK_NE(map_iterator, containing_highlight_registries_.end());
  DCHECK_GT(map_iterator->value, 0u);
  if (--map_iterator->value == 0)
    containing_highlight_registries_.erase(map_iterator);
}

}  // namespace blink
