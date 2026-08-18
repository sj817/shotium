// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_CUSTOM_ELEMENT_REGISTRY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_CUSTOM_ELEMENT_REGISTRY_H_

#include "base/gtest_prod_util.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/node_rare_data_field.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_definition.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_map.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_set.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_linked_hash_set.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string_hash.h"

namespace blink {

class CustomElementDescriptor;
class Document;
class Element;
class ExceptionState;
class LocalDOMWindow;

class CORE_EXPORT CustomElementRegistry final : public ScriptWrappable,
                                                public NodeRareDataField {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static CustomElementRegistry* DefaultRegistry(Document& document);

  CustomElementRegistry(const LocalDOMWindow*, int32_t world_id);
  CustomElementRegistry(const CustomElementRegistry&) = delete;
  CustomElementRegistry& operator=(const CustomElementRegistry&) = delete;
  ~CustomElementRegistry() override = default;

  // Access to custom element definitions. Name here refers to the
  // https://whatwg.org/C/#concept-custom-element-definition-name.
  //
  // Nothing populates the definition tables any more: a custom element
  // definition can only be created by CustomElementRegistry.define(), which was
  // a JavaScript entry point backed by ScriptCustomElementDefinition. With no
  // script engine there are no definitions, so every lookup below correctly
  // reports "not a custom element" and elements stay in the "undefined" state.
  // getName(constructor) was here: the reverse lookup from a script
  // constructor back to its registered name.
  bool NameIsDefined(const AtomicString& name) const;
  Vector<AtomicString> DefinedNames() const;
  CustomElementDefinition* DefinitionForName(const AtomicString& name) const;

  // TODO(dominicc): Switch most callers of definitionForName to
  // definitionFor when implementing type extensions.
  CustomElementDefinition* DefinitionFor(const CustomElementDescriptor&) const;

  // TODO(dominicc): Consider broadening this API when type extensions are
  // implemented.
  void AddCandidate(Element&);
  void upgrade(Node* root);

  const LocalDOMWindow* GetOwnerWindow() const { return owner_.Get(); }

  bool IsGlobalRegistry() const { return is_global_registry_; }
  void MarkAsGlobalRegistry() { is_global_registry_ = true; }

  int32_t GetWorldId() const { return world_id_; }

  void AssociatedWith(Document& document);

  void initialize(Node* root, ExceptionState&);

  void Trace(Visitor*) const override;

 private:
  void CollectCandidates(const CustomElementDescriptor&,
                         HeapVector<Member<Element>>*);

  bool is_global_registry_ = false;
  // Preserves the value of the old per-world "invalid world" sentinel.
  static constexpr int32_t kInvalidWorldId = -1;
  int32_t world_id_ = kInvalidWorldId;


  using NameMap = HeapHashMap<AtomicString, Member<CustomElementDefinition>>;
  NameMap name_map_;

  Member<const LocalDOMWindow> owner_;

  using UpgradeCandidateSet = GCedHeapHashSet<WeakMember<Element>>;
  using UpgradeCandidateMap =
      GCedHeapHashMap<AtomicString, Member<UpgradeCandidateSet>>;

  // Candidate elements that can be upgraded with this registry later.
  // To make implementation simpler, we maintain a superset here, and remove
  // non-candidates before upgrading.
  Member<UpgradeCandidateMap> upgrade_candidates_;

  // Weak ordered set of all documents where this registry is used, in the order
  // of association between this registry and any tree scope in the document.
  using AssociatedDocumentSet = GCedHeapLinkedHashSet<WeakMember<Document>>;
  Member<AssociatedDocumentSet> associated_documents_;

  FRIEND_TEST_ALL_PREFIXES(
      CustomElementTest,
      CreateElement_TagNameCaseHandlingCreatingCustomElement);
  friend class CustomElementRegistryTest;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_CUSTOM_ELEMENT_REGISTRY_H_
