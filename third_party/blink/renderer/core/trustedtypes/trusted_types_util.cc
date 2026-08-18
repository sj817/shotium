// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/trustedtypes/trusted_types_util.h"

#include <optional>

#include "base/compiler_specific.h"
#include "base/notreached.h"
#include "base/unguessable_token.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom-blink-forward.h"
#include "third_party/blink/public/mojom/reporting/reporting.mojom-blink.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_sanitizer_config.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_set_html_options.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_set_html_unsafe_options.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_union_sanitizer_sanitizerconfig_sanitizerpresets.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_union_sethtmlunsafeoptions_trustedparseroptions.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_union_string_trustedhtml.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_union_string_trustedscript.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_union_stringlegacynulltoemptystring_trustedhtml.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_union_stringlegacynulltoemptystring_trustedscript.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_union_trustedhtml_trustedscript_trustedscripturl.h"
// v8_union_trustedscripturl_usvstring.h was included here for the
// (TrustedScriptURL or USVString) union, which no surviving IDL declares
// and no code in this file names.
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/html/parser/fragment_parser.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/script/script_element_base.h"
#include "third_party/blink/renderer/core/trustedtypes/trusted_html.h"
#include "third_party/blink/renderer/core/trustedtypes/trusted_parser_options.h"
#include "third_party/blink/renderer/core/trustedtypes/trusted_script.h"
#include "third_party/blink/renderer/core/trustedtypes/trusted_script_url.h"
#include "third_party/blink/renderer/core/trustedtypes/trusted_type_policy_factory.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {

namespace {

enum TrustedTypeViolationKind {
  kTrustedHTMLAssignment,
  kTrustedScriptAssignment,
  kTrustedScriptURLAssignment,
  kTrustedHTMLAssignmentAndDefaultPolicyFailed,
  kTrustedHTMLAssignmentAndNoDefaultPolicyExisted,
  kTrustedScriptAssignmentAndDefaultPolicyFailed,
  kTrustedScriptAssignmentAndNoDefaultPolicyExisted,
  kTrustedScriptURLAssignmentAndDefaultPolicyFailed,
  kTrustedScriptURLAssignmentAndNoDefaultPolicyExisted,
  kNavigateToJavascriptURL,
  kNavigateToJavascriptURLAndDefaultPolicyFailed,
  kNavigateToJavascriptURLAndDefaultPolicyCreatedInvalidURL,
  kScriptExecution,
  kScriptExecutionAndDefaultPolicyFailed,
  kTrustedHTMLParserOptionsTransform,
  kTrustedHTMLParserOptionsTransformAndNoDefaultPolicyExisted,
  kTrustedHTMLParserOptionsTransformAndDefaultPolicyFailed,
};

// Strings to support building a sample, used in:
// https://www.w3.org/TR/trusted-types/#should-block-sink-type-mismatch
const char* kFunctionAnonymousPrefix = "(function anonymous";
const char* kAsyncFunctionAnonymousPrefix = "(async function anonymous";
const char* kGeneratorAnonymousPrefix = "(function* anonymous";
const char* kAsyncGeneratorAnonymousPrefix = "(async function* anonymous";

const char kFunctionConstructorFailureConsoleMessage[] =
    "The JavaScript Function constructor does not accept TrustedString "
    "arguments. See https://github.com/w3c/webappsec-trusted-types/wiki/"
    "Trusted-Types-for-function-constructor for more information.";

const char kScriptExecutionTrustedTypeFailConsoleMessage[] =
    "This document requires 'TrustedScript' assignment. "
    "An HTMLScriptElement was directly modified and will not be executed.";

const char* GetMessage(TrustedTypeViolationKind kind) {
  switch (kind) {
    case kTrustedHTMLAssignment:
      return "This document requires 'TrustedHTML' assignment.";
    case kTrustedScriptAssignment:
      return "This document requires 'TrustedScript' assignment.";
    case kTrustedScriptURLAssignment:
      return "This document requires 'TrustedScriptURL' assignment.";
    case kTrustedHTMLAssignmentAndDefaultPolicyFailed:
      return "This document requires 'TrustedHTML' assignment and the "
             "'default' policy failed to execute.";
    case kTrustedHTMLAssignmentAndNoDefaultPolicyExisted:
      return "This document requires 'TrustedHTML' assignment and no "
             "'default' policy for 'TrustedHTML' has been defined.";
    case kTrustedScriptAssignmentAndDefaultPolicyFailed:
      return "This document requires 'TrustedScript' assignment and the "
             "'default' policy failed to execute.";
    case kTrustedScriptAssignmentAndNoDefaultPolicyExisted:
      return "This document requires 'TrustedScript' assignment and no "
             "'default' policy for 'TrustedScript' has been defined.";
    case kTrustedScriptURLAssignmentAndDefaultPolicyFailed:
      return "This document requires 'TrustedScriptURL' assignment and the "
             "'default' policy failed to execute.";
    case kTrustedScriptURLAssignmentAndNoDefaultPolicyExisted:
      return "This document requires 'TrustedScriptURL' assignment and no "
             "'default' policy for 'TrustedScriptURL' has been defined.";
    case kNavigateToJavascriptURL:
      return "This document requires 'TrustedScript' assignment. "
             "Navigating to a javascript:-URL is equivalent to a "
             "'TrustedScript' assignment.";
    case kNavigateToJavascriptURLAndDefaultPolicyFailed:
      return "This document requires 'TrustedScript' assignment. "
             "Navigating to a javascript:-URL is equivalent to a "
             "'TrustedScript' assignment and the 'default' policy failed to"
             "execute.";
    case kNavigateToJavascriptURLAndDefaultPolicyCreatedInvalidURL:
      return "This document requires 'TrustedScript' assignment. "
             "Navigating to a javascript:-URL is equivalent to a "
             "'TrustedScript' assignment and the 'default' policy created an "
             "invalid URL.";
    case kScriptExecution:
      return "This document requires 'TrustedScript' assignment. "
             "This script element was modified without use of TrustedScript "
             "assignment.";
    case kScriptExecutionAndDefaultPolicyFailed:
      return "This document requires 'TrustedScript' assignment. "
             "This script element was modified without use of TrustedScript "
             "assignment and the 'default' policy failed to execute.";
    case kTrustedHTMLParserOptionsTransform:
      CHECK(RuntimeEnabledFeatures::NewHTMLSettingMethodsEnabled());
      return "This document requires 'TrustedParserOptions' assignment.";
    case kTrustedHTMLParserOptionsTransformAndNoDefaultPolicyExisted:
      CHECK(RuntimeEnabledFeatures::NewHTMLSettingMethodsEnabled());
      return "The TrustedParserOptions parser options transform failed and no "
             "'default' policy for 'TrustedParserOptions' has been defined.";
    case kTrustedHTMLParserOptionsTransformAndDefaultPolicyFailed:
      CHECK(RuntimeEnabledFeatures::NewHTMLSettingMethodsEnabled());
      return "The TrustedParserOptions parser options transform failed and the "
             "'default' policy failed to execute.";
  }
  NOTREACHED();
}

String GetSamplePrefix(const AtomicString& interface_name,
                       const AtomicString& property_name,
                       const String& value) {
  // We have two sample formats, one for eval and one for assignment.
  // If we don't have the required values being passed in, just leave the
  // sample empty.
  StringBuilder sample_prefix;
  if (interface_name.empty()) {
    // No interface name? Then we have no prefix to use.
  } else if (interface_name == trusted_types_names::kEval) {
    bool is_function =
        RuntimeEnabledFeatures::TrustedTypesHTMLEnabled()
            ? (value.starts_with(kFunctionAnonymousPrefix) ||
               value.starts_with(kAsyncFunctionAnonymousPrefix) ||
               value.starts_with(kGeneratorAnonymousPrefix) ||
               value.starts_with(kAsyncGeneratorAnonymousPrefix))
            : value.starts_with(kFunctionAnonymousPrefix);
    sample_prefix.Append(is_function ? trusted_types_names::kFunction
                                     : trusted_types_names::kEval);
  } else if ((interface_name == trusted_types_names::kWorker ||
              interface_name == trusted_types_names::kSharedWorker) &&
             !property_name.empty()) {
    // Worker/SharedWorker constructor has nullptr as property_name.
    sample_prefix.Append(interface_name);
    sample_prefix.Append(" constructor");
  } else if (!interface_name.empty() && !property_name.empty()) {
    sample_prefix.Append(interface_name);
    sample_prefix.Append(" ");
    sample_prefix.Append(property_name);
  }
  return sample_prefix.ToString();
}

const char* GetElementName(const ScriptElementBase::Type type) {
  switch (type) {
    case ScriptElementBase::Type::kHTMLScriptElement:
      return "HTMLScriptElement";
    case ScriptElementBase::Type::kSVGScriptElement:
      return "SVGScriptElement";
  }
  NOTREACHED();
}

// Handle failure of a Trusted Type assignment.
//
// If trusted type assignment fails, we need to
// - report the violation via CSP
// - increment the appropriate counter,
// - raise a JavaScript exception (if enforced).
//
// Returns whether the failure should be enforced.
bool TrustedTypeFail(TrustedTypeViolationKind kind,
                     const ExecutionContext* execution_context,
                     const AtomicString& interface_name,
                     const AtomicString& property_name,
                     ExceptionState& exception_state,
                     const String& value) {
  if (!execution_context) {
    return true;
  }

  // Test case docs (Document::CreateForTest()) might not have a window
  // and hence no TrustedTypesPolicyFactory.
  if (execution_context->GetTrustedTypes()) {
    execution_context->GetTrustedTypes()->CountTrustedTypeAssignmentError();
  }

  String prefix = GetSamplePrefix(interface_name, property_name, value);

  // https://www.w3.org/TR/trusted-types/#should-block-sink-type-mismatch step 3
  size_t strip = 0;
  if (prefix == "Function") {
    if (value.starts_with(kFunctionAnonymousPrefix)) {
      strip = strlen(kFunctionAnonymousPrefix);
    } else if (value.starts_with(kAsyncFunctionAnonymousPrefix)) {
      strip = strlen(kAsyncFunctionAnonymousPrefix);
    } else if (value.starts_with(kGeneratorAnonymousPrefix)) {
      strip = strlen(kGeneratorAnonymousPrefix);
    } else if (value.starts_with(kAsyncGeneratorAnonymousPrefix)) {
      strip = strlen(kAsyncGeneratorAnonymousPrefix);
    }
  }

  // This issue_id is used to generate a link in the DevTools front-end from
  // the JavaScript TypeError to the inspector issue which is reported by
  // ContentSecurityPolicy::ReportViolation via the call to
  // AllowTrustedTypeAssignmentFailure below.
  base::UnguessableToken issue_id = base::UnguessableToken::Create();
  bool allow =
      execution_context->GetContentSecurityPolicy()
          ->AllowTrustedTypeAssignmentFailure(
              GetMessage(kind),
              strip ? value.substr(static_cast<wtf_size_t>(strip)) : value,
              prefix, issue_id);

  // TODO(1087743): Add a console message for Trusted Type-related Function
  // constructor failures, to warn the developer of the outstanding issues
  // with TT and Function  constructors. This should be removed once the
  // underlying issue has been fixed.
  if (prefix == "Function" && !allow &&
      !RuntimeEnabledFeatures::TrustedTypesUseCodeLikeEnabled()) {
    DCHECK(kind == kTrustedScriptAssignment ||
           kind == kTrustedScriptAssignmentAndDefaultPolicyFailed ||
           kind == kTrustedScriptAssignmentAndNoDefaultPolicyExisted);
    execution_context->GetContentSecurityPolicy()->LogToConsole(
        MakeGarbageCollected<ConsoleMessage>(
            mojom::blink::ConsoleMessageSource::kRecommendation,
            mojom::blink::ConsoleMessageLevel::kInfo,
            kFunctionConstructorFailureConsoleMessage));
  }
  probe::OnContentSecurityPolicyViolation(
      const_cast<ExecutionContext*>(execution_context),
      ContentSecurityPolicyViolationType::kTrustedTypesSinkViolation);

  if (!allow) {
    exception_state.ThrowTypeError(GetMessage(kind));
  }
  return !allow;
}

}  // namespace

bool RequireTrustedTypesCheck(const ExecutionContext* execution_context) {
  return execution_context && execution_context->RequireTrustedTypes() &&
         !ContentSecurityPolicy::ShouldBypassMainWorldDeprecated(
             execution_context);
}

String TrustedTypesCheckForHTML(const String& html,
                                const ExecutionContext* execution_context,
                                const AtomicString& interface_name,
                                const AtomicString& property_name,
                                ExceptionState& exception_state) {
  // https://w3c.github.io/trusted-types/dist/spec/#abstract-opdef-process-value-with-a-default-policy
  //
  // The "default policy" leg of this algorithm is gone: a TrustedTypePolicy
  // could only ever be created through trustedTypes.createPolicy(), which was
  // reachable from script alone, so there is never a default policy to run.
  // What is left is the plain enforcement decision, unchanged: report the CSP
  // violation and either block the assignment or let the raw string through.
  if (!RequireTrustedTypesCheck(execution_context)) {
    return html;
  }
  if (TrustedTypeFail(kTrustedHTMLAssignment, execution_context, interface_name,
                      property_name, exception_state, html)) {
    return g_empty_string;
  }
  return html;
}

String TrustedTypesCheckForScript(const String& script,
                                const ExecutionContext* execution_context,
                                const AtomicString& interface_name,
                                const AtomicString& property_name,
                                ExceptionState& exception_state) {
  // https://w3c.github.io/trusted-types/dist/spec/#abstract-opdef-process-value-with-a-default-policy
  //
  // The "default policy" leg of this algorithm is gone: a TrustedTypePolicy
  // could only ever be created through trustedTypes.createPolicy(), which was
  // reachable from script alone, so there is never a default policy to run.
  // What is left is the plain enforcement decision, unchanged: report the CSP
  // violation and either block the assignment or let the raw string through.
  if (!RequireTrustedTypesCheck(execution_context)) {
    return script;
  }
  if (TrustedTypeFail(kTrustedScriptAssignment, execution_context, interface_name,
                      property_name, exception_state, script)) {
    return g_empty_string;
  }
  return script;
}

String TrustedTypesCheckForScriptURL(const String& script_url,
                                const ExecutionContext* execution_context,
                                const AtomicString& interface_name,
                                const AtomicString& property_name,
                                ExceptionState& exception_state) {
  // https://w3c.github.io/trusted-types/dist/spec/#abstract-opdef-process-value-with-a-default-policy
  //
  // The "default policy" leg of this algorithm is gone: a TrustedTypePolicy
  // could only ever be created through trustedTypes.createPolicy(), which was
  // reachable from script alone, so there is never a default policy to run.
  // What is left is the plain enforcement decision, unchanged: report the CSP
  // violation and either block the assignment or let the raw string through.
  if (!RequireTrustedTypesCheck(execution_context)) {
    return script_url;
  }
  if (TrustedTypeFail(kTrustedScriptURLAssignment, execution_context, interface_name,
                      property_name, exception_state, script_url)) {
    return g_empty_string;
  }
  return script_url;
}

AtomicString TrustedTypesCheckFor(SpecificTrustedType type,
                                  AtomicString trusted,
                                  const ExecutionContext* execution_context,
                                  const AtomicString& interface_name,
                                  const AtomicString& property_name,
                                  ExceptionState& exception_state) {
  if (type == SpecificTrustedType::kNone) {
    return trusted;
  }

  switch (type) {
    case SpecificTrustedType::kHTML:
      return AtomicString(TrustedTypesCheckForHTML(
          std::move(trusted), execution_context, interface_name, property_name,
          exception_state));
    case SpecificTrustedType::kScript:
      return AtomicString(TrustedTypesCheckForScript(
          std::move(trusted), execution_context, interface_name, property_name,
          exception_state));
    case SpecificTrustedType::kScriptURL:
      return AtomicString(TrustedTypesCheckForScriptURL(
          std::move(trusted), execution_context, interface_name, property_name,
          exception_state));
    case SpecificTrustedType::kNone:
      NOTREACHED();  // This case is handled above.
  }
  NOTREACHED();
}

String TrustedTypesCheckForHTML(const V8UnionStringOrTrustedHTML* value,
                                const ExecutionContext* execution_context,
                                const AtomicString& interface_name,
                                const AtomicString& property_name,
                                ExceptionState& exception_state) {
  if (!value) {
    return TrustedTypesCheckForHTML(g_empty_string, execution_context,
                                    interface_name, property_name,
                                    exception_state);
  }
  switch (value->GetContentType()) {
    case V8UnionStringOrTrustedHTML::ContentType::kString:
      return TrustedTypesCheckForHTML(value->GetAsString(), execution_context,
                                      interface_name, property_name,
                                      exception_state);
    case V8UnionStringOrTrustedHTML::ContentType::kTrustedHTML:
      return value->GetAsTrustedHTML()->toString();
  }
  NOTREACHED();
}

String TrustedTypesCheckForHTML(
    const V8UnionStringLegacyNullToEmptyStringOrTrustedHTML* value,
    const ExecutionContext* execution_context,
    const AtomicString& interface_name,
    const AtomicString& property_name,
    ExceptionState& exception_state) {
  if (!value) {
    return TrustedTypesCheckForHTML(g_empty_string, execution_context,
                                    interface_name, property_name,
                                    exception_state);
  }
  switch (value->GetContentType()) {
    case V8UnionStringLegacyNullToEmptyStringOrTrustedHTML::ContentType::
        kStringLegacyNullToEmptyString:
      return TrustedTypesCheckForHTML(
          value->GetAsStringLegacyNullToEmptyString(), execution_context,
          interface_name, property_name, exception_state);
    case V8UnionStringLegacyNullToEmptyStringOrTrustedHTML::ContentType::
        kTrustedHTML:
      return value->GetAsTrustedHTML()->toString();
  }
  NOTREACHED();
}

String TrustedTypesCheckForScript(const V8UnionStringOrTrustedScript* value,
                                  const ExecutionContext* execution_context,
                                  const AtomicString& interface_name,
                                  const AtomicString& property_name,
                                  ExceptionState& exception_state) {
  // To remain compatible with legacy behaviour, HTMLElement uses extended IDL
  // attributes to allow for nullable union of (DOMString or TrustedScript).
  // Thus, this method is required to handle the case where |!value|, unlike
  // the various similar methods in this file.
  if (!value) {
    return TrustedTypesCheckForScript(g_empty_string, execution_context,
                                      interface_name, property_name,
                                      exception_state);
  }

  switch (value->GetContentType()) {
    case V8UnionStringOrTrustedScript::ContentType::kString:
      return TrustedTypesCheckForScript(value->GetAsString(), execution_context,
                                        interface_name, property_name,
                                        exception_state);
    case V8UnionStringOrTrustedScript::ContentType::kTrustedScript:
      return value->GetAsTrustedScript()->toString();
  }

  NOTREACHED();
}

String TrustedTypesCheckForScript(
    const V8UnionStringLegacyNullToEmptyStringOrTrustedScript* value,
    const ExecutionContext* execution_context,
    const AtomicString& interface_name,
    const AtomicString& property_name,
    ExceptionState& exception_state) {
  // To remain compatible with legacy behaviour, HTMLElement uses extended IDL
  // attributes to allow for nullable union of (DOMString or TrustedScript).
  // Thus, this method is required to handle the case where |!value|, unlike
  // the various similar methods in this file.
  if (!value) {
    return TrustedTypesCheckForScript(g_empty_string, execution_context,
                                      interface_name, property_name,
                                      exception_state);
  }

  switch (value->GetContentType()) {
    case V8UnionStringLegacyNullToEmptyStringOrTrustedScript::ContentType::
        kStringLegacyNullToEmptyString:
      return TrustedTypesCheckForScript(
          value->GetAsStringLegacyNullToEmptyString(), execution_context,
          interface_name, property_name, exception_state);
    case V8UnionStringLegacyNullToEmptyStringOrTrustedScript::ContentType::
        kTrustedScript:
      return value->GetAsTrustedScript()->toString();
  }

  NOTREACHED();
}

String TrustedTypesCheckForFragment(const V8UnionStringOrTrustedHTML* html,
                                    FragmentParserOptions& resolved_options,
                                    const ExecutionContext* execution_context,
                                    const AtomicString& interface_name,
                                    const AtomicString& property_name,
                                    ExceptionState& exception_state) {
  // See the header comment: the "default policy" parser-options transform
  // that used to run here required a TrustedTypePolicy created via
  // trustedTypes.createPolicy() (script-only), so it never fires and has
  // been dropped. `resolved_options` passes through unchanged.
  return TrustedTypesCheckForHTML(html, execution_context, interface_name,
                                  property_name, exception_state);
}

std::tuple<String, FragmentParserOptions> TrustedTypesCheckForLegacyFragment(
    const V8UnionStringLegacyNullToEmptyStringOrTrustedHTML* html,
    const ExecutionContext* execution_context,
    const AtomicString& interface_name,
    const AtomicString& property_name,
    ExceptionState& exception_state) {
  // See TrustedTypesCheckForFragment above: the default-policy parser-options
  // transform is gone, so this always returns default-constructed options.
  String compliant_string = TrustedTypesCheckForHTML(
      html, execution_context, interface_name, property_name, exception_state);
  if (exception_state.HadException()) {
    return {String(), FragmentParserOptions()};
  }
  return {compliant_string, FragmentParserOptions()};
}

String CORE_EXPORT
GetStringForScriptExecution(const String& script,
                            const ScriptElementBase::Type type,
                            ExecutionContext* context) {
  DummyExceptionStateForTesting exception_state;
  String value = TrustedTypesCheckForScript(
      script, context, AtomicString(GetElementName(type)),
      trusted_types_names::kText, exception_state);
  if (exception_state.HadException()) {
    value = String();
  }
  if (!script.IsNull() && value.IsNull()) {
    context->AddConsoleMessage(MakeGarbageCollected<ConsoleMessage>(
        mojom::blink::ConsoleMessageSource::kSecurity,
        mojom::blink::ConsoleMessageLevel::kError,
        kScriptExecutionTrustedTypeFailConsoleMessage));
  }
  return value;
}

String TrustedTypesCheckForJavascriptURLinNavigation(
    const String& javascript_url,
    ExecutionContext* context) {
  DummyExceptionStateForTesting exception_state;
  String value = TrustedTypesCheckForScript(
      javascript_url, context, trusted_types_names::kLocation,
      trusted_types_names::kHref, exception_state);
  return exception_state.HadException() ? String() : value;
}

String TrustedTypesCheckForExecCommand(
    const String& html,
    const ExecutionContext* execution_context,
    ExceptionState& exception_state) {
  return TrustedTypesCheckForHTML(
      html, execution_context, trusted_types_names::kDocument,
      trusted_types_names::kExecCommand, exception_state);
}

bool IsTrustedTypesEventHandlerAttribute(const QualifiedName& q_name) {
  return q_name.NamespaceURI().IsNull() &&
         TrustedTypePolicyFactory::IsEventHandlerAttributeName(
             q_name.LocalName());
}

}  // namespace blink
