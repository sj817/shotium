// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/scheduler/task_attribution_task_state.h"

#include "third_party/blink/renderer/platform/bindings/script_forbidden_scope.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"
#include "third_party/blink/renderer/platform/wtf/thread_specific.h"

namespace blink {

namespace {

Persistent<TaskAttributionTaskState>& CurrentTaskStateSlot() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      ThreadSpecific<Persistent<TaskAttributionTaskState>>, slot, ());
  return *slot;
}

}  // namespace

// static
TaskAttributionTaskState* TaskAttributionTaskState::GetCurrent() {
  return CurrentTaskStateSlot().Get();
}

// static
void TaskAttributionTaskState::SetCurrent(
    TaskAttributionTaskState* task_state) {
  CurrentTaskStateSlot() = task_state;
}

}  // namespace blink
