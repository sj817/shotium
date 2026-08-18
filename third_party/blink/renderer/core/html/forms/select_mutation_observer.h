// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_SELECT_MUTATION_OBSERVER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_SELECT_MUTATION_OBSERVER_H_

#include "third_party/blink/renderer/core/dom/mutation_observer.h"
#include "third_party/blink/renderer/core/html/forms/html_select_element.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_set.h"

namespace blink {

// This is similar to SummaryDescendantsObserver which fills a similar purpose
// for <summary>.  They could in theory share a small amount of common code,
// but such a refactoring would probably harm code readability too much.
class SelectMutationObserver : public MutationObserver::Delegate {
 public:
  explicit SelectMutationObserver(HTMLSelectElement& select);

  ExecutionContext* GetExecutionContext() const override;
  void Deliver(const MutationRecordVector& records, MutationObserver&) override;
  void Trace(Visitor* visitor) const override;

  void Disconnect();

  static bool IsInteractiveElement(const Node& node);
  static bool HasTabIndexAttribute(const Node& node);
  static bool IsContenteditable(const Node& node);
  static bool IsWhitespaceOrEmpty(const Node& node);

 private:
  void CheckAddedNodes(MutationRecord* record,
                       HeapHashSet<Member<Node>>& visited_nodes);
  void CheckRemovedNodes(MutationRecord* record);
  void TraverseNodeDescendants(const Node* node,
                               HeapHashSet<Member<Node>>& visited_nodes);
  void AddDescendantDisallowedErrorToNode(
      Node& node,
      HeapHashSet<Member<Node>>& visited_nodes);
  bool IsAllowedInteractiveElement(Node& node);
  // Returns true if `descendant` is a valid child in its position, false if
  // it violates <select>'s content model. This used to also return a more
  // specific ElementAccessibilityIssueReason for DevTools Issues panel
  // reporting and WebFeature UseCounter metrics (see the removed
  // RecordIssueByType() and the AuditsIssue::ReportElementAccessibilityIssue
  // call in select_mutation_observer.cc); AuditsIssue (core/inspector) is
  // gone, so a plain bool is all that's needed now.
  bool CheckForIssue(const Node& descendant);
  bool IsAllowedDescendantOfSelect(const Node& descendant, const Node& parent);
  bool IsAllowedDescendantOfOptgroup(const Node& descendant,
                                     const Node& parent);
  bool IsAllowedDescendantOfButton(const Node& descendant);
  bool CheckDescedantOfOption(const Node& descendant);
  bool TraverseAncestorsAndCheckDescendant(const Node& descendant);
  bool IsAllowedPhrasingContent(const Node& node);
  bool IsAutonomousCustomElement(const Node& node);

  Member<HTMLSelectElement> select_;
  Member<MutationObserver> observer_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_SELECT_MUTATION_OBSERVER_H_
