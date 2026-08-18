// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TRUSTEDTYPES_TRUSTED_TYPES_UTIL_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TRUSTEDTYPES_TRUSTED_TYPES_UTIL_H_

#include <tuple>

#include "third_party/blink/renderer/bindings/core/v8/v8_set_html_options.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_set_html_unsafe_options.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_typedefs.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/script/script_element_base.h"
#include "third_party/blink/renderer/core/trustedtypes/trusted_types_names.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ExceptionState;
class ExecutionContext;
class FragmentParserOptions;
class QualifiedName;
class V8UnionStringLegacyNullToEmptyStringOrTrustedHTML;
class V8UnionStringLegacyNullToEmptyStringOrTrustedScript;
class V8UnionStringOrTrustedHTML;
class V8UnionStringOrTrustedScript;

enum class SpecificTrustedType {
  kNone,
  kHTML,
  kScript,
  kScriptURL,
};

// Perform Trusted Type checks, for a dynamically or statically determined
// type.
// Returns the effective value (which may have been modified by the "default"
// policy.
[[nodiscard]] AtomicString TrustedTypesCheckFor(
    SpecificTrustedType,
    AtomicString,
    const ExecutionContext*,
    const AtomicString& interface_name,
    const AtomicString& property_name,
    ExceptionState&);
[[nodiscard]] CORE_EXPORT String
TrustedTypesCheckForHTML(const String&,
                         const ExecutionContext*,
                         const AtomicString& interface_name,
                         const AtomicString& property_name,
                         ExceptionState&);
[[nodiscard]] CORE_EXPORT String
TrustedTypesCheckForScript(const String&,
                           const ExecutionContext*,
                           const AtomicString& interface_name,
                           const AtomicString& property_name,
                           ExceptionState&);
[[nodiscard]] CORE_EXPORT String
TrustedTypesCheckForScriptURL(const String&,
                              const ExecutionContext*,
                              const AtomicString& interface_name,
                              const AtomicString& property_name,
                              ExceptionState&);

// Union-typed overloads: unwrap the (String or TrustedX) IDL union and
// either pass the already-trusted string straight through, or run the
// same check as the String overload above.
[[nodiscard]] CORE_EXPORT String
TrustedTypesCheckForHTML(const V8UnionStringOrTrustedHTML*,
                         const ExecutionContext*,
                         const AtomicString& interface_name,
                         const AtomicString& property_name,
                         ExceptionState&);
[[nodiscard]] CORE_EXPORT String
TrustedTypesCheckForHTML(const V8UnionStringLegacyNullToEmptyStringOrTrustedHTML*,
                         const ExecutionContext*,
                         const AtomicString& interface_name,
                         const AtomicString& property_name,
                         ExceptionState&);
[[nodiscard]] CORE_EXPORT String
TrustedTypesCheckForScript(const V8UnionStringOrTrustedScript*,
                           const ExecutionContext*,
                           const AtomicString& interface_name,
                           const AtomicString& property_name,
                           ExceptionState&);
[[nodiscard]] CORE_EXPORT String
TrustedTypesCheckForScript(const V8UnionStringLegacyNullToEmptyStringOrTrustedScript*,
                           const ExecutionContext*,
                           const AtomicString& interface_name,
                           const AtomicString& property_name,
                           ExceptionState&);
// The (TrustedScriptURL or USVString) overload was here, for the setters of
// HTMLScriptElement.src, HTMLObjectElement.data and HTMLEmbedElement.src. Those
// are script entry points and are cut; SVG uses the String overload above.

// Check a fragment-parsing HTML value (used by innerHTML-family setters)
// against Trusted Types.
//
// The pre-V8ectomy version additionally ran `resolved_options` through a
// "default policy" createParserOptions() callback here (behind the
// TrustedTypesCreateParserOptionsEnabled flag). That default policy could
// only ever be installed via trustedTypes.createPolicy(), which is reachable
// from script alone, so there is never a default policy to run; the options
// are returned unchanged.
[[nodiscard]] CORE_EXPORT String
TrustedTypesCheckForFragment(const V8UnionStringOrTrustedHTML*,
                             FragmentParserOptions&,
                             const ExecutionContext*,
                             const AtomicString& interface_name,
                             const AtomicString& property_name,
                             ExceptionState&);

// Same as above, for the legacy (null-to-empty-string) union used by a few
// older setters.
[[nodiscard]] CORE_EXPORT std::tuple<String, FragmentParserOptions>
TrustedTypesCheckForLegacyFragment(
    const V8UnionStringLegacyNullToEmptyStringOrTrustedHTML*,
    const ExecutionContext*,
    const AtomicString& interface_name,
    const AtomicString& property_name,
    ExceptionState&);

// Functionally equivalent to TrustedTypesCheckForScript(const String&, ...),
// but with setup & error handling suitable for the asynchronous execution
// cases.
String TrustedTypesCheckForJavascriptURLinNavigation(const String&,
                                                     ExecutionContext*);
CORE_EXPORT String GetStringForScriptExecution(const String&,
                                               ScriptElementBase::Type,
                                               ExecutionContext*);

// Functionally equivalent to TrustedTypesCheckForHTML(const String&, ...),
// but with separate enable flag and use counter, to ensure this won't break
// existing sites before enabling it in full.
[[nodiscard]] CORE_EXPORT String
TrustedTypesCheckForExecCommand(const String&,
                                const ExecutionContext*,
                                ExceptionState&);

// Determine whether a Trusted Types check is needed in this execution context.
//
// Note: All methods above handle this internally and will return success if a
// check is not required. However, in cases where not-required doesn't
// immediately imply "okay" this method can be used.
// Example: To determine whether 'eval' may pass, one needs to also take CSP
// into account.
CORE_EXPORT bool RequireTrustedTypesCheck(const ExecutionContext*);

// Determine whether an attribute is considered an event handler by Trusted
// Types.
//
// Note: This is different from Element::IsEventHandlerAttribute, because
// Element only needs this distinction for built-in attributes, but not for
// user-defined property names. But Trusted Types needs this for any built-in or
// user-defined attribute/property, and thus must check against a list of known
// event handlers.
bool IsTrustedTypesEventHandlerAttribute(const QualifiedName&);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TRUSTEDTYPES_TRUSTED_TYPES_UTIL_H_
