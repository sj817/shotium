// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/custom/custom_element_registry.h"

#include <limits>

#include "base/auto_reset.h"
#include "third_party/blink/public/web/web_custom_element.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_element_definition_options.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/html/custom/ce_reactions_scope.h"
#include "third_party/blink/renderer/core/html/custom/custom_element.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_definition.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_descriptor.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_reaction_stack.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_registry_assignment.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_upgrade_sorter.h"
#include "third_party/blink/renderer/core/html_element_type_helpers.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/wtf/text/strcat.h"

namespace blink {

namespace {

void CollectUpgradeCandidateInNode(CustomElementRegistry* registry,
                                   Node& root,
                                   HeapVector<Member<Element>>& candidates) {
  // 1-1. If candidate is not an Element node, then continue.
  // 1-2. If candidate's custom element registry is not this, then continue.
  if (auto* root_element = DynamicTo<Element>(root)) {
    if (root_element->GetCustomElementState() ==
            CustomElementState::kUndefined &&
        root_element->customElementRegistry() == registry) {
      candidates.push_back(root_element);
    }
    if (auto* shadow_root = root_element->GetShadowRoot()) {
      if (shadow_root->GetMode() != ShadowRootMode::kUserAgent) {
        CollectUpgradeCandidateInNode(registry, *shadow_root, candidates);
      }
    }
  }
  for (auto& element : Traversal<HTMLElement>::ChildrenOf(root))
    CollectUpgradeCandidateInNode(registry, element, candidates);
}

}  // namespace

CustomElementRegistry* CustomElementRegistry::DefaultRegistry(
    Document& document) {
  return document.customElementRegistry();
}

CustomElementRegistry::CustomElementRegistry(const LocalDOMWindow* owner,
                                             int32_t world_id)
    : world_id_(world_id),
      owner_(owner),
      upgrade_candidates_(MakeGarbageCollected<UpgradeCandidateMap>()),
      associated_documents_(MakeGarbageCollected<AssociatedDocumentSet>()) {}

Vector<AtomicString> CustomElementRegistry::DefinedNames() const {
  Vector<AtomicString> names;
  for (const auto& name : name_map_.Keys()) {
    names.push_back(name);
  }
  return names;
}

void CustomElementRegistry::Trace(Visitor* visitor) const {
  visitor->Trace(name_map_);
  visitor->Trace(owner_);
  visitor->Trace(upgrade_candidates_);
  visitor->Trace(associated_documents_);
  ScriptWrappable::Trace(visitor);
  NodeRareDataField::Trace(visitor);
}

// https://html.spec.whatwg.org/C/#look-up-a-custom-element-definition
// At this point, what the spec calls 'is' is 'name' from desc
CustomElementDefinition* CustomElementRegistry::DefinitionFor(
    const CustomElementDescriptor& desc) const {
  // desc.name() is 'is' attribute
  // 4. If definition in registry with name equal to local name...
  CustomElementDefinition* definition = DefinitionForName(desc.LocalName());
  // 5. If definition in registry with name equal to name...
  if (!definition)
    definition = DefinitionForName(desc.GetName());
  // 4&5. ...and local name equal to localName, return that definition
  if (definition and definition->Descriptor().LocalName() == desc.LocalName()) {
    return definition;
  }
  // 6. Return null
  return nullptr;
}

bool CustomElementRegistry::NameIsDefined(const AtomicString& name) const {
  return name_map_.Contains(name);
}

CustomElementDefinition* CustomElementRegistry::DefinitionForName(
    const AtomicString& name) const {
  const auto it = name_map_.find(name);
  if (it == name_map_.end())
    return nullptr;
  return it->value.Get();
}

void CustomElementRegistry::AddCandidate(Element& candidate) {
  AtomicString name = candidate.localName();
  if (!CustomElement::IsValidName(name)) {
    const AtomicString& is = candidate.IsValue();
    if (!is.IsNull())
      name = is;
  }
  if (NameIsDefined(name))
    return;
  UpgradeCandidateMap::iterator it = upgrade_candidates_->find(name);
  UpgradeCandidateSet* set;
  if (it != upgrade_candidates_->end()) {
    set = it->value;
  } else {
    set = upgrade_candidates_
              ->insert(name, MakeGarbageCollected<UpgradeCandidateSet>())
              .stored_value->value;
  }
  set->insert(&candidate);
}

void CustomElementRegistry::CollectCandidates(
    const CustomElementDescriptor& desc,
    HeapVector<Member<Element>>* elements) {
  UpgradeCandidateMap::iterator it = upgrade_candidates_->find(desc.GetName());
  if (it == upgrade_candidates_->end())
    return;
  CustomElementUpgradeSorter sorter;
  for (Element* element : *it.Get()->value) {
    if (!element || !desc.Matches(*element))
      continue;
    if ((*element).customElementRegistry() != this) {
      // The element has been moved away from the original tree scope and no
      // longer uses this registry.
      continue;
    }
    sorter.Add(element);
  }

  upgrade_candidates_->erase(it);

  for (Document* document : *associated_documents_) {
    if (document && document->GetFrame()) {
      sorter.Sorted(elements, document);
    }
  }
}

// https://html.spec.whatwg.org/C/#dom-customelementregistry-upgrade
void CustomElementRegistry::upgrade(Node* root) {
  DCHECK(root);

  // 1. For each shadow-including inclusive descendant candidate of root
  // in shadow-including tree order:
  HeapVector<Member<Element>> candidates;
  CollectUpgradeCandidateInNode(this, *root, candidates);

  // 1-3. For each candidate of candidates, try to upgrade candidate.
  for (auto& candidate : candidates)
    CustomElement::TryToUpgrade(*candidate);
}

void CustomElementRegistry::AssociatedWith(Document& document) {
  associated_documents_->insert(&document);
}

// Entry point of "Custom Element Registry initialization".
// https://html.spec.whatwg.org/multipage/custom-elements.html#dom-customelementregistry-initialize
void CustomElementRegistry::initialize(Node* root,
                                       ExceptionState& exception_state) {
  // 1. If this's "is scoped" is false and either root is a Document node or
  // root's node document's custom element registry is not this, then throw a
  // "NotSupportedError" DOMException.
  if (IsGlobalRegistry() &&
      (root->GetDocument().customElementRegistry() != this)) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kNotSupportedError,
        "The registry provided is a global registry from another document");
    return;
  }

  // An iframe may not be aware of the existence of a scoped registry since the
  // the created scoped registry's local dom window is not tied to the iframe's
  // document. In such case, when we initialize nodes in the iframe with scoped
  // registry using CustomElementRegistry::initialize, we should let the
  // iframe's document know that scoped registry is used.
  if (!IsGlobalRegistry()) {
    root->GetDocument().SetScopedCustomElementRegistryUsed();
  }

  // 2. If root is a Document node whose custom element registry is null, then
  // set root's custom element registry to this.
  // 3. Otherwise, if root is a ShadowRoot node whose custom element registry is
  // null, then set root's custom element registry to this.
  if (auto* document = DynamicTo<Document>(root);
      document && !document->customElementRegistry()) {
    document->SetCustomElementRegistry(
        CustomElementRegistryAssignment::Explicit(this));
  } else if (auto* shadow_root = DynamicTo<ShadowRoot>(root);
             shadow_root && !shadow_root->customElementRegistry()) {
    shadow_root->SetCustomElementRegistry(
        CustomElementRegistryAssignment::Explicit(this));
  }

  // 4. For each inclusive descendant inclusiveDescendant of root, in tree
  // order.
  for (Node& descendant : NodeTraversal::InclusiveDescendantsOf(*root)) {
    Element* descendant_element = DynamicTo<Element>(descendant);

    // 4-1. If inclusiveDescendant is an Element node, then continue.
    if (!descendant_element) {
      continue;
    }

    // 4-2. If inclusiveDescendant's custom element registry is null, then:
    if (!descendant_element->customElementRegistry()) {
      // 4-2-1. Set inclusiveDescendant's custom element registry to this.
      descendant_element->SetCustomElementRegistry(
          CustomElementRegistryAssignment::Explicit(this));
      // 4-2-2. If this's "is scoped" is true, then append inclusiveDescendant's
      // node document to this's scoped document set.
      if (!this->IsGlobalRegistry()) {
        this->AssociatedWith(descendant_element->GetDocument());
      }
    }

    // 4-3. If inclusiveDescendant's custom element registry is not this, then
    // continue.
    if (descendant_element->customElementRegistry() != this) {
      continue;
    }

    // 4-4. Try to upgrade inclusiveDescendant.
    if (descendant_element->GetCustomElementState() ==
        CustomElementState::kUndefined) {
      CustomElement::TryToUpgrade(*descendant_element);
    }
  }
}

}  // namespace blink
