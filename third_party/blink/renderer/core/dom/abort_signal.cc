// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/abort_signal.h"

#include <optional>
#include <utility>

#include "base/auto_reset.h"
#include "base/check_deref.h"
#include "base/debug/crash_logging.h"
#include "base/functional/callback.h"
#include "base/time/time.h"
#include "third_party/blink/renderer/core/dom/abort_signal_composition_manager.h"
#include "third_party/blink/renderer/core/dom/abort_signal_composition_type.h"
#include "third_party/blink/renderer/core/dom/abort_signal_registry.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/event_target_names.h"
#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/platform/bindings/exception_code.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_linked_hash_set.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_vector.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"
#include "third_party/blink/renderer/platform/heap/visitor.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

namespace {

class OnceCallbackAlgorithm final : public AbortSignal::Algorithm {
 public:
  explicit OnceCallbackAlgorithm(base::OnceClosure callback)
      : callback_(std::move(callback)) {}
  ~OnceCallbackAlgorithm() override = default;

  void Run() override { std::move(callback_).Run(); }

 private:
  base::OnceClosure callback_;
};

}  // namespace

AbortSignal::AbortSignal(ExecutionContext* execution_context)
    : ExecutionContextLifecycleObserver(execution_context),
      signal_type_(SignalType::kComposite) {
  InitializeCompositeSignal(HeapVector<Member<AbortSignal>>());
}

AbortSignal::AbortSignal(ExecutionContext* execution_context,
                         SignalType signal_type)
    : ExecutionContextLifecycleObserver(execution_context),
      signal_type_(signal_type),
      composition_manager_(MakeGarbageCollected<SourceSignalCompositionManager>(
          *this,
          AbortSignalCompositionType::kAbort)) {
  DCHECK_NE(signal_type, SignalType::kComposite);
}

AbortSignal::AbortSignal(ExecutionContext* execution_context,
                         const HeapVector<Member<AbortSignal>>& source_signals)
    : ExecutionContextLifecycleObserver(execution_context),
      signal_type_(SignalType::kComposite) {
  // If any of the signals are aborted, skip the linking and just abort this
  // signal.
  for (auto& source : source_signals) {
    CHECK(source.Get());
    if (source->aborted()) {
      abort_reason_ = source->reason();
      break;
    }
  }
  InitializeCompositeSignal(aborted() ? HeapVector<Member<AbortSignal>>()
                                      : source_signals);
}

void AbortSignal::InitializeCompositeSignal(
    const HeapVector<Member<AbortSignal>>& source_signals) {
  CHECK_EQ(signal_type_, SignalType::kComposite);
  composition_manager_ =
      MakeGarbageCollected<DependentSignalCompositionManager>(
          *this, AbortSignalCompositionType::kAbort, source_signals);
  // Ensure the registry isn't created during GC, e.g. during an abort
  // controller's prefinalizer.
  AbortSignalRegistry::From(CHECK_DEREF(GetExecutionContext()));
}

AbortSignal::~AbortSignal() = default;

const AtomicString& AbortSignal::InterfaceName() const {
  return event_target_names::kAbortSignal;
}

ExecutionContext* AbortSignal::GetExecutionContext() const {
  return ExecutionContextLifecycleObserver::GetExecutionContext();
}

AbortSignal::AlgorithmHandle* AbortSignal::AddAlgorithm(Algorithm* algorithm) {
  if (aborted() || composition_manager_->IsSettled()) {
    return nullptr;
  }
  CHECK(!is_running_abort_steps_);
  auto* handle = MakeGarbageCollected<AlgorithmHandle>(algorithm, this);
  CHECK(!abort_algorithms_.Contains(handle));
  // This always appends since `handle` is not already in the collection.
  abort_algorithms_.insert(handle);
  return handle;
}

AbortSignal::AlgorithmHandle* AbortSignal::AddAlgorithm(
    base::OnceClosure algorithm) {
  if (aborted() || composition_manager_->IsSettled()) {
    return nullptr;
  }
  CHECK(!is_running_abort_steps_);
  auto* callback_algorithm =
      MakeGarbageCollected<OnceCallbackAlgorithm>(std::move(algorithm));
  auto* handle =
      MakeGarbageCollected<AlgorithmHandle>(callback_algorithm, this);
  CHECK(!abort_algorithms_.Contains(handle));
  // This always appends since `handle` is not already in the collection.
  abort_algorithms_.insert(handle);
  return handle;
}

void AbortSignal::RemoveAlgorithm(AlgorithmHandle* handle) {
  if (aborted() || composition_manager_->IsSettled()) {
    return;
  }
  CHECK(!is_running_abort_steps_);
  abort_algorithms_.erase(handle);
}

void AbortSignal::SignalAbort(DOMException* reason, SignalAbortPassKey) {
  CHECK(reason);
  if (aborted()) {
    return;
  }

  CHECK(composition_manager_);
  auto* source_signal_manager =
      DynamicTo<SourceSignalCompositionManager>(composition_manager_.Get());
  // `SignalAbort` can only be called on source signals.
  CHECK(source_signal_manager);
  HeapVector<Member<AbortSignal>> dependent_signals_to_abort;
  dependent_signals_to_abort.ReserveInitialCapacity(
      source_signal_manager->GetDependentSignals().size());

  // Set the abort reason for this signal and any unaborted dependent signals so
  // that all dependent signals are aborted before JS runs in abort algorithms
  // or event dispatch.
  SetAbortReason(reason);

  for (auto& signal : source_signal_manager->GetDependentSignals()) {
    CHECK(signal.Get());
    if (!signal->aborted()) {
      signal->SetAbortReason(abort_reason_.Get());
      dependent_signals_to_abort.push_back(signal);
    }
  }

  RunAbortSteps();

  for (auto& signal : dependent_signals_to_abort) {
    signal->RunAbortSteps();
    signal->composition_manager_->Settle();
  }

  composition_manager_->Settle();
}

void AbortSignal::SetAbortReason(DOMException* reason) {
  CHECK(!aborted());
  CHECK(reason);
  abort_reason_ = reason;
}

void AbortSignal::RunAbortSteps() {
  CHECK(!is_running_abort_steps_);
  base::AutoReset<bool> scope(&is_running_abort_steps_, true);
  SCOPED_CRASH_KEY_NUMBER("AbortSignal", "size_before",
                          static_cast<int>(abort_algorithms_.size()));

  // TODO(crbug.com/40068730): Remove after root-causing the bug. This is meant
  // to help determine if running the algorithms is changing the set or
  // invalidating the iterator unexpectedly.
  for (AbortSignal::AlgorithmHandle* handle : abort_algorithms_) {
    CHECK(handle);
  }

  for (AbortSignal::AlgorithmHandle* handle : abort_algorithms_) {
    SCOPED_CRASH_KEY_NUMBER("AbortSignal", "current_size",
                            static_cast<int>(abort_algorithms_.size()));
    CHECK(handle);
    CHECK(handle->GetAlgorithm());
    handle->GetAlgorithm()->Run();
  }

  DispatchEvent(*Event::Create(event_type_names::kAbort));
}

void AbortSignal::Trace(Visitor* visitor) const {
  visitor->Trace(abort_reason_);
  visitor->Trace(abort_algorithms_);
  visitor->Trace(composition_manager_);
  EventTarget::Trace(visitor);
  ExecutionContextLifecycleObserver::Trace(visitor);
}

AbortSignalCompositionManager* AbortSignal::GetCompositionManager(
    AbortSignalCompositionType type) {
  if (type == AbortSignalCompositionType::kAbort) {
    return composition_manager_.Get();
  }
  return nullptr;
}

void AbortSignal::DetachFromController() {
  if (aborted()) {
    return;
  }
  CHECK(!is_running_abort_steps_);
  composition_manager_->Settle();
}

void AbortSignal::OnSignalSettled(AbortSignalCompositionType type) {
  if (type == AbortSignalCompositionType::kAbort) {
    CHECK(!is_running_abort_steps_);
    abort_algorithms_.clear();
  }
  if (signal_type_ == SignalType::kComposite && GetExecutionContext()) {
    AbortSignalRegistry::From(*GetExecutionContext())
        ->UnregisterSignal(*this, type);
  }
}

bool AbortSignal::CanAbort() const {
  if (aborted()) {
    return false;
  }
  return !composition_manager_->IsSettled();
}

void AbortSignal::AddedEventListener(
    const AtomicString& event_type,
    RegisteredEventListener& registered_listener) {
  EventTarget::AddedEventListener(event_type, registered_listener);
  OnEventListenerAddedOrRemoved(event_type, AddRemoveType::kAdded);
}

void AbortSignal::RemovedEventListener(
    const AtomicString& event_type,
    const RegisteredEventListener& registered_listener) {
  EventTarget::RemovedEventListener(event_type, registered_listener);
  OnEventListenerAddedOrRemoved(event_type, AddRemoveType::kRemoved);
}

void AbortSignal::OnEventListenerAddedOrRemoved(const AtomicString& event_type,
                                                AddRemoveType add_or_remove) {
  if (signal_type_ != SignalType::kComposite) {
    return;
  }
  std::optional<AbortSignalCompositionType> composition_type;
  if (event_type == event_type_names::kAbort) {
    composition_type = AbortSignalCompositionType::kAbort;
  } else if (event_type == event_type_names::kPrioritychange) {
    composition_type = AbortSignalCompositionType::kPriority;
  } else {
    return;
  }
  if (IsSettledFor(*composition_type)) {
    // Signals are unregistered when they're settled for `composition_type`
    // since the event will no longer be propagated. In that case, the signal
    // doesn't need to be unregistered on removal, and it shouldn't be
    // registered on adding a listener, since that could leak it.
    return;
  }
  if (add_or_remove == AddRemoveType::kRemoved &&
      HasEventListeners(event_type)) {
    // Unsettled composite signals need to be kept alive while they have active
    // event listeners for `event_type`, so only unregister the signal if
    // removing the last one.
    return;
  }
  // `manager` will be null if this signal doesn't handle composition for
  // `composition_type`.
  if (GetCompositionManager(*composition_type)) {
    // `EventTarget` ignores adding event listeners to detached contexts, and
    // all listeners are cleared on detach, so the context should always exist
    // here.
    auto* registry =
        AbortSignalRegistry::From(CHECK_DEREF(GetExecutionContext()));
    switch (add_or_remove) {
      case AddRemoveType::kAdded:
        registry->RegisterSignal(*this, *composition_type);
        break;
      case AddRemoveType::kRemoved:
        registry->UnregisterSignal(*this, *composition_type);
        break;
    }
  }
}

bool AbortSignal::IsSettledFor(
    AbortSignalCompositionType composition_type) const {
  return composition_type == AbortSignalCompositionType::kAbort &&
         composition_manager_->IsSettled();
}

void AbortSignal::ContextDestroyed() {
  EventTarget::RemoveAllEventListeners();
}

AbortSignal::AlgorithmHandle::AlgorithmHandle(AbortSignal::Algorithm* algorithm,
                                              AbortSignal* signal)
    : algorithm_(algorithm), signal_(signal) {
  CHECK(algorithm_);
  CHECK(signal_);
}

AbortSignal::AlgorithmHandle::~AlgorithmHandle() = default;

void AbortSignal::AlgorithmHandle::Trace(Visitor* visitor) const {
  visitor->Trace(algorithm_);
  visitor->Trace(signal_);
}

}  // namespace blink
