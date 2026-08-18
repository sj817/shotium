// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/trustedtypes/trusted_type_policy_factory.h"

#include "third_party/blink/public/mojom/use_counter/metrics/web_feature.mojom-blink.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/event_target_names.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/mathml_names.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/svg_names.h"
#include "third_party/blink/renderer/core/trustedtypes/trusted_html.h"
#include "third_party/blink/renderer/core/trustedtypes/trusted_script.h"
#include "third_party/blink/renderer/core/trustedtypes/trusted_types_util.h"
#include "third_party/blink/renderer/core/xlink_names.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/text/string_hash.h"

namespace blink {

namespace {

const char* kHtmlNamespace = "http://www.w3.org/1999/xhtml";

struct AttributeTypeEntry {
  QualifiedName element;
  QualifiedName attribute;
  SpecificTrustedType type;
};

typedef Vector<AttributeTypeEntry> AttributeTypeVector;

AttributeTypeVector BuildAttributeVector() {
  const QualifiedName any_element(g_null_atom, g_star_atom, g_null_atom);
  const struct {
    const QualifiedName& element;
    const QualifiedName attribute;
    SpecificTrustedType type;
  } kTypeTable[] = {{html_names::kEmbedTag, html_names::kSrcAttr,
                     SpecificTrustedType::kScriptURL},
                    {html_names::kIFrameTag, html_names::kSrcdocAttr,
                     SpecificTrustedType::kHTML},
                    {html_names::kObjectTag, html_names::kCodebaseAttr,
                     SpecificTrustedType::kScriptURL},
                    {html_names::kObjectTag, html_names::kDataAttr,
                     SpecificTrustedType::kScriptURL},
                    {html_names::kScriptTag, html_names::kSrcAttr,
                     SpecificTrustedType::kScriptURL},
                    {svg_names::kScriptTag, svg_names::kHrefAttr,
                     SpecificTrustedType::kScriptURL},
                    {svg_names::kScriptTag, xlink_names::kHrefAttr,
                     SpecificTrustedType::kScriptURL},


      // One row per event-handler content attribute -- onclick, onload, and the
      // couple of hundred others -- marking each as a script sink, was expanded
      // here from the generated EVENT_HANDLER_LIST. That header came out of the
      // Web IDL database, which this build no longer produces.
      //
      // Dropping the rows changes nothing any rendering can see. The table is
      // consulted when an attribute is set through setAttribute() under a
      // `require-trusted-types-for 'script'` policy, so that assigning a bare
      // string to onclick is refused. Parsed markup does not take that path,
      // and setAttribute() is only reachable from script. With no script
      // engine an on* attribute is inert text either way.
  };

  AttributeTypeVector table;
  for (const auto& entry : kTypeTable) {
    // In legacy-Trusted-Types, we didn't record SVG elements properly in
    // this function. So we can now use this to retain the old behaviour, until
    // TrustedTypesHTML is perma-launched.
    if (!RuntimeEnabledFeatures::TrustedTypesHTMLEnabled() &&
        entry.element.NamespaceURI() == svg_names::kNamespaceURI) {
      continue;
    }

    // Attribute comparisons are case-insensitive, for both element and
    // attribute name. We rely on the fact that they're stored as lowercase.
    DCHECK(entry.element.LocalName().ContainsNoAsciiUpper());
    DCHECK(entry.attribute.LocalName().ContainsNoAsciiUpper());
    table.push_back(
        AttributeTypeEntry{entry.element, entry.attribute, entry.type});
  }
  return table;
}

const AttributeTypeVector& GetAttributeTypeVector() {
  DEFINE_STATIC_LOCAL(AttributeTypeVector, attribute_table_,
                      (BuildAttributeVector()));
  return attribute_table_;
}

AttributeTypeVector BuildPropertyVector() {
  const QualifiedName any_element(g_null_atom, g_star_atom, g_null_atom);
  const struct {
    const QualifiedName& element;
    const char* property;
    SpecificTrustedType type;
  } kTypeTable[] = {
      {html_names::kEmbedTag, "src", SpecificTrustedType::kScriptURL},
      {html_names::kIFrameTag, "srcdoc", SpecificTrustedType::kHTML},
      {html_names::kObjectTag, "codeBase", SpecificTrustedType::kScriptURL},
      {html_names::kObjectTag, "data", SpecificTrustedType::kScriptURL},
      {html_names::kScriptTag, "innerText", SpecificTrustedType::kScript},
      {html_names::kScriptTag, "src", SpecificTrustedType::kScriptURL},
      {html_names::kScriptTag, "text", SpecificTrustedType::kScript},
      {html_names::kScriptTag, "textContent", SpecificTrustedType::kScript},
      {svg_names::kScriptTag, "href", SpecificTrustedType::kScriptURL},
      {any_element, "innerHTML", SpecificTrustedType::kHTML},
      {any_element, "outerHTML", SpecificTrustedType::kHTML},
  };
  AttributeTypeVector table;
  for (const auto& entry : kTypeTable) {
    // In legacy-Trusted-Types, we didn't record SVG elements properly in
    // this function. So we can now use this to retain the old behaviour, until
    // TrustedTypesHTML is perma-launched.
    if (!RuntimeEnabledFeatures::TrustedTypesHTMLEnabled() &&
        entry.element.NamespaceURI() == svg_names::kNamespaceURI) {
      continue;
    }

    // Elements are case-insensitive, but property names are not.
    // Properties don't have a namespace, so we're leaving that blank.
    DCHECK(entry.element.LocalName().ContainsNoAsciiUpper());
    table.push_back(AttributeTypeEntry{
        entry.element, QualifiedName(AtomicString(entry.property)),
        entry.type});
  }
  return table;
}

const AttributeTypeVector& GetPropertyTypeVector() {
  DEFINE_STATIC_LOCAL(AttributeTypeVector, property_table_,
                      (BuildPropertyVector()));
  return property_table_;
}

// Find an entry matching `attribute` on any element in an AttributeTypeVector.
// Assumes that argument normalization has already happened.
SpecificTrustedType FindUnboundAttributeInAttributeTypeVector(
    const AttributeTypeVector& attribute_type_vector,
    const AtomicString& attribute) {
  for (const auto& entry : attribute_type_vector) {
    bool entry_matches = entry.attribute.LocalName() == attribute &&
                         entry.attribute.NamespaceURI() == g_null_atom &&
                         entry.element == g_star_atom;
    if (entry_matches) {
      return entry.type;
    }
  }
  return SpecificTrustedType::kNone;
}

// Find a matching entry in an AttributeTypeVector. Assumes that argument
// normalization has already happened.
SpecificTrustedType FindEntryInAttributeTypeVector(
    const AttributeTypeVector& attribute_type_vector,
    const AtomicString& element,
    const AtomicString& attribute,
    const AtomicString& element_namespace,
    const AtomicString& attribute_namespace) {
  // https://w3c.github.io/trusted-types/dist/spec/#abstract-opdef-get-trusted-type-data-for-attribute,
  // step 2, matches event handlers only against the HTML-known namespaces,
  // not against any namespace.
  //
  // For legacy behaviour and for property type vectors, "*" should match any
  // namespace. For attributes, it should only match HTML, SVG, and MathML.
  bool matches_star_atom =
      !RuntimeEnabledFeatures::TrustedTypesHTMLEnabled() ||
      (&attribute_type_vector == &GetPropertyTypeVector()) ||
      (element_namespace == html_names::xhtmlNamespaceURI ||
       element_namespace == svg_names::kNamespaceURI ||
       element_namespace == mathml_names::kNamespaceURI);
  for (const auto& entry : attribute_type_vector) {
    bool element_matches =
        (entry.element.LocalName() == element &&
         entry.element.NamespaceURI() == element_namespace) ||
        (entry.element == g_star_atom && matches_star_atom);
    bool attribute_matches =
        entry.attribute.LocalName() == attribute &&
        entry.attribute.NamespaceURI() == attribute_namespace;
    if (element_matches && attribute_matches) {
      return entry.type;
    }
  }
  return SpecificTrustedType::kNone;
}

// Find a matching entry in an AttributeTypeVector. Converts arguments to
// AtomicString and does spec-mandated mapping of empty strings as namespaces.
SpecificTrustedType FindEntryInAttributeTypeVector(
    const AttributeTypeVector& attribute_type_vector,
    const String& element,
    const String& attribute,
    const String& element_namespace,
    const String& attribute_namespace) {
  return FindEntryInAttributeTypeVector(
      attribute_type_vector, AtomicString(element), AtomicString(attribute),
      element_namespace.empty() ? AtomicString(kHtmlNamespace)
                                : AtomicString(element_namespace),
      attribute_namespace.empty() ? AtomicString()
                                  : AtomicString(attribute_namespace));
}

}  // anonymous namespace

TrustedTypePolicyFactory::TrustedTypePolicyFactory(ExecutionContext* context)
    : ExecutionContextClient(context),
      empty_html_(MakeGarbageCollected<TrustedHTML>("")),
      empty_script_(MakeGarbageCollected<TrustedScript>("")) {}

TrustedHTML* TrustedTypePolicyFactory::emptyHTML() const {
  return empty_html_.Get();
}

TrustedScript* TrustedTypePolicyFactory::emptyScript() const {
  return empty_script_.Get();
}

String getTrustedTypeName(SpecificTrustedType type) {
  switch (type) {
    case SpecificTrustedType::kHTML:
      return "TrustedHTML";
    case SpecificTrustedType::kScript:
      return "TrustedScript";
    case SpecificTrustedType::kScriptURL:
      return "TrustedScriptURL";
    case SpecificTrustedType::kNone:
      return String();
  }
}

String TrustedTypePolicyFactory::getPropertyType(
    const String& tagName,
    const String& propertyName,
    const String& elementNS) const {
  return getTrustedTypeName(FindEntryInAttributeTypeVector(
      GetPropertyTypeVector(), tagName.ToAsciiLower(), propertyName, elementNS,
      String()));
}

String TrustedTypePolicyFactory::getAttributeType(
    const String& tagName,
    const String& attributeName,
    const String& tagNS,
    const String& attributeNS) const {
  return getTrustedTypeName(FindEntryInAttributeTypeVector(
      GetAttributeTypeVector(), tagName.ToAsciiLower(),
      attributeName.ToAsciiLower(), tagNS, attributeNS));
}

void TrustedTypePolicyFactory::CountTrustedTypeAssignmentError() {
  if (!hadAssignmentError) {
    UseCounter::Count(GetExecutionContext(),
                      WebFeature::kTrustedTypesAssignmentError);
    hadAssignmentError = true;
  }
}

const AtomicString& TrustedTypePolicyFactory::InterfaceName() const {
  return event_target_names::kTrustedTypePolicyFactory;
}

ExecutionContext* TrustedTypePolicyFactory::GetExecutionContext() const {
  return ExecutionContextClient::GetExecutionContext();
}

void TrustedTypePolicyFactory::Trace(Visitor* visitor) const {
  EventTarget::Trace(visitor);
  ExecutionContextClient::Trace(visitor);
  visitor->Trace(empty_html_);
  visitor->Trace(empty_script_);
}

// Ensure that the qualified names are constructed on the main thread to avoid
// race conditions in the QualifiedNameCache (crbug.com/503618702).
// static
void TrustedTypePolicyFactory::EagerlyInitializeOnMainThread() {
  DCHECK(IsMainThread());
  GetAttributeTypeVector();
  GetPropertyTypeVector();
}

inline bool FindEventHandlerAttributeInTable(
    const AtomicString& attributeName) {
  return SpecificTrustedType::kScript ==
         FindUnboundAttributeInAttributeTypeVector(GetAttributeTypeVector(),
                                                   attributeName);
}

bool TrustedTypePolicyFactory::IsEventHandlerAttributeName(
    const AtomicString& attributeName) {
  // Check that the "on" prefix indeed filters out only non-event handlers.
  DCHECK(!FindEventHandlerAttributeInTable(attributeName) ||
         attributeName.StartsWithIgnoringAsciiCase("on"));

  return attributeName.StartsWithIgnoringAsciiCase("on") &&
         FindEventHandlerAttributeInTable(attributeName);
}

}  // namespace blink
