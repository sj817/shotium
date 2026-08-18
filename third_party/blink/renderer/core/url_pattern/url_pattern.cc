// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/url_pattern/url_pattern.h"

#include <algorithm>

// base/notreached.h and base/strings/string_util.h went with generate(),
// compareComponent() and the private V8 Match(): NOTREACHED() and
// base::IsStringASCII() had no other callers here.
#include "base/types/expected.h"
#include "third_party/abseil-cpp/absl/status/status.h"
#include "third_party/blink/public/common/safe_url_pattern.h"
// The union headers (v8_union_urlpattern*.h) went with test()/exec()/From();
// the result dictionaries (v8_url_pattern_result.h,
// v8_url_pattern_component_result.h) went with exec().  to_v8_traits.h and
// script_regexp.h went with V8 itself.  What is left of this include block is
// the two dictionaries that are still built in C++, now emitted by
// tools/shot/gen_idl_dictionaries.py.
#include "third_party/blink/renderer/bindings/core/v8/v8_union_urlpattern_urlpatterninit_usvstring.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_union_urlpatterninit_usvstring.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_url_pattern_init.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_url_pattern_options.h"
#include "third_party/blink/renderer/core/url_pattern/url_pattern_canon.h"
#include "third_party/blink/renderer/core/url_pattern/url_pattern_component.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/string_to_number.h"
#include "third_party/blink/renderer/platform/wtf/text/string_utf8_adaptor.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/liburlpattern/constructor_string_parser.h"
#include "third_party/liburlpattern/pattern.h"
#include "third_party/liburlpattern/tokenize.h"
#include "third_party/liburlpattern/utils.h"

namespace blink {

using url_pattern::Component;
using url_pattern::ValueType;

namespace {

// Utility function to determine if a pathname is absolute or not.  For
// kURL values this mainly consists of a check for a leading slash.  For
// patterns we do some additional checking for escaped or grouped slashes.
bool IsAbsolutePathname(const String& pathname, ValueType type) {
  if (pathname.empty())
    return false;

  if (pathname[0] == '/')
    return true;

  if (type == ValueType::kURL)
    return false;

  if (pathname.length() < 2)
    return false;

  // Patterns treat escaped slashes and slashes within an explicit grouping as
  // valid leading slashes.  For example, "\/foo" or "{/foo}".  Patterns do
  // not consider slashes within a custom regexp group as valid for the leading
  // pathname slash for now.  To support that we would need to be able to
  // detect things like ":name_123(/foo)" as a valid leading group in a pattern,
  // but that is considered too complex for now.
  if ((pathname[0] == '\\' || pathname[0] == '{') && pathname[1] == '/') {
    return true;
  }

  return false;
}

// Utility function to determine if the default port for the given protocol
// matches the given port number.
bool IsProtocolDefaultPort(const String& protocol, const String& port) {
  if (protocol.empty() || port.empty())
    return false;

  std::optional<int32_t> port_number = StringToInt(port, {});
  if (!port_number) {
    return false;
  }

  StringUtf8Adaptor protocol_utf8(protocol);
  int default_port = url::DefaultPortForScheme(protocol_utf8.AsStringView());
  return default_port != url::PORT_UNSPECIFIED && default_port == *port_number;
}

// Base URL values that include pattern string characters should not blow
// up pattern parsing.  Automatically escape them.  We must not escape inputs
// for non-pattern base URLs, though.
String EscapeBaseURLString(const StringView& input, ValueType type) {
  if (input.empty()) {
    return g_empty_string;
  }

  if (type != ValueType::kPattern) {
    return input.ToString();
  }

  std::string result;
  result.reserve(input.length());

  StringUtf8Adaptor utf8(input);
  liburlpattern::EscapePatternStringAndAppend(utf8.AsStringView(), result);

  return String::FromUtf8(result);
}

// A utility method that takes a URLPatternInit, splits it apart, and applies
// the individual component values in the given set of strings.  The strings
// are only applied if a value is present in the init structure.
void ApplyInit(const URLPatternInit* init,
               ValueType type,
               String& protocol,
               String& username,
               String& password,
               String& hostname,
               String& port,
               String& pathname,
               String& search,
               String& hash,
               ExceptionState& exception_state) {
  // If there is a baseURL we need to apply its component values first.  The
  // rest of the URLPatternInit structure will then later override these
  // values.  Note, the baseURL will always set either an empty string or
  // longer value for each considered component.  We do not allow null strings
  // to persist for these components past this phase since they should no
  // longer be treated as wildcards.
  KURL base_url;
  if (init->hasBaseURL()) {
    base_url = KURL(init->baseURL());
    if (!base_url.IsValid() || base_url.IsEmpty()) {
      exception_state.ThrowTypeError(
          StrCat({"Invalid baseURL '", init->baseURL(), "'."}));
      return;
    }

    // Components are only inherited from the base URL if no "earlier" component
    // is specified in |init|.  Furthermore, when the base URL is being used as
    // the basis of a pattern (not a URL being matched against), usernames and
    // passwords are always wildcarded unless explicitly specified otherwise,
    // because they usually do not affect which resource is requested (though
    // they do often affect whether access is authorized).
    //
    // Even though they appear earlier than the hostname in a URL, the username
    // and password are treated as appearing after it because they typically
    // refer to credentials within a realm on an origin, rather than being used
    // across all hostnames.
    //
    // This partial ordering is represented by the following diagram:
    //
    //                                 +-> pathname --> search --> hash
    // protocol --> hostname --> port -|
    //                                 +-> username --> password
    protocol = init->hasProtocol()
                   ? String()
                   : EscapeBaseURLString(base_url.Protocol(), type);
    username = (type == ValueType::kPattern ||
                (init->hasProtocol() || init->hasHostname() ||
                 init->hasPort() || init->hasUsername()))
                   ? String()
                   : EscapeBaseURLString(base_url.User(), type);
    password = (type == ValueType::kPattern ||
                (init->hasProtocol() || init->hasHostname() ||
                 init->hasPort() || init->hasUsername() || init->hasPassword()))
                   ? String()
                   : EscapeBaseURLString(base_url.Pass(), type);
    hostname = (init->hasProtocol() || init->hasHostname())
                   ? String()
                   : EscapeBaseURLString(base_url.Host(), type);
    port = (init->hasProtocol() || init->hasHostname() || init->hasPort())
               ? String()
           : base_url.Port() > 0 ? String::Number(base_url.Port())
                                 : g_empty_string;
    pathname = (init->hasProtocol() || init->hasHostname() || init->hasPort() ||
                init->hasPathname())
                   ? String()
                   : EscapeBaseURLString(base_url.GetPath(), type);
    search = (init->hasProtocol() || init->hasHostname() || init->hasPort() ||
              init->hasPathname() || init->hasSearch())
                 ? String()
                 : EscapeBaseURLString(base_url.Query(), type);
    hash = (init->hasProtocol() || init->hasHostname() || init->hasPort() ||
            init->hasPathname() || init->hasSearch() || init->hasHash())
               ? String()
           : base_url.HasFragmentIdentifier()
               ? EscapeBaseURLString(base_url.FragmentIdentifier(), type)
               : g_empty_string;
  }

  // Apply the URLPatternInit component values on top of the default and
  // baseURL values.
  if (init->hasProtocol()) {
    protocol = url_pattern::CanonicalizeProtocol(init->protocol(), type,
                                                 exception_state);
    if (exception_state.HadException())
      return;
  }
  if (init->hasUsername() || init->hasPassword()) {
    String init_username = init->hasUsername() ? init->username() : String();
    String init_password = init->hasPassword() ? init->password() : String();
    url_pattern::CanonicalizeUsernameAndPassword(init_username, init_password,
                                                 type, username, password,
                                                 exception_state);
    if (exception_state.HadException())
      return;
  }
  if (init->hasHostname()) {
    hostname = url_pattern::CanonicalizeHostname(init->hostname(), type,
                                                 exception_state);
    if (exception_state.HadException())
      return;
  }
  if (init->hasPort()) {
    port = url_pattern::CanonicalizePort(init->port(), type, protocol,
                                         exception_state);
    if (exception_state.HadException())
      return;
  }
  if (init->hasPathname()) {
    pathname = init->pathname();
    if (base_url.IsValid() && base_url.IsHierarchical() &&
        !IsAbsolutePathname(pathname, type)) {
      // Find the last slash in the baseURL pathname.  Since the URL is
      // hierarchical it should have a slash to be valid, but we are cautious
      // and check.  If there is no slash then we cannot use resolve the
      // relative pathname and just treat the init pathname as an absolute
      // value.
      String base_path = EscapeBaseURLString(base_url.GetPath(), type);
      auto slash_index = base_path.rfind('/');
      if (slash_index != kNotFound) {
        // Extract the baseURL path up to and including the first slash.  Append
        // the relative init pathname to it.
        pathname =
            StrCat({StringView(base_path, 0, slash_index + 1), pathname});
      }
    }
    pathname = url_pattern::CanonicalizePathname(protocol, pathname, type,
                                                 exception_state);
    if (exception_state.HadException())
      return;
  }
  if (init->hasSearch()) {
    search =
        url_pattern::CanonicalizeSearch(init->search(), type, exception_state);
    if (exception_state.HadException())
      return;
  }
  if (init->hasHash()) {
    hash = url_pattern::CanonicalizeHash(init->hash(), type, exception_state);
    if (exception_state.HadException())
      return;
  }
}

// MakeURLPatternComponentResult() was here.  It is the clearest example of
// what exec() could not survive: its whole job was to turn a null blink::String
// -- a capture group that did not participate in the match -- into
// v8::Undefined, because URLPatternComponentResult.groups is a
// `record<USVString, any>` and `any` is a script value by definition.

URLPatternInit* MakeURLPatternInit(
    const liburlpattern::ConstructorStringParser::Result& result) {
  auto* init = URLPatternInit::Create();
  if (result.protocol) {
    init->setProtocol(String::FromUtf8(*result.protocol));
  }
  if (result.username) {
    init->setUsername(String::FromUtf8(*result.username));
  }
  if (result.password) {
    init->setPassword(String::FromUtf8(*result.password));
  }
  if (result.hostname) {
    init->setHostname(String::FromUtf8(*result.hostname));
  }
  if (result.port) {
    init->setPort(String::FromUtf8(*result.port));
  }
  if (result.pathname) {
    init->setPathname(String::FromUtf8(*result.pathname));
  }
  if (result.search) {
    init->setSearch(String::FromUtf8(*result.search));
  }
  if (result.hash) {
    init->setHash(String::FromUtf8(*result.hash));
  }
  return init;
}

}  // namespace

// URLPattern::From(v8::Isolate*, const V8URLPatternCompatible*, ...) was here.
// It existed to normalise the `(USVString or URLPatternInit or URLPattern)`
// argument that WebIDL APIs elsewhere in blink accepted in place of a
// URLPattern.  Those APIs are cut, and with no WebIDL there is no such
// argument to normalise; a C++ caller picks the Create() overload it wants.

URLPattern* URLPattern::Create(const String& input_string,
                               const String& base_url,
                               const URLPatternOptions* options,
                               ExceptionState& exception_state) {
  // The "Invalid second argument baseURL ... provided with a URLPatternInit
  // input" TypeError used to be thrown here, after unwrapping the union.  With
  // the string and the init on separate overloads it cannot be reached: the
  // overload that takes an init takes no base URL.

  const StringUtf8Adaptor utf8_string(input_string);
  liburlpattern::ConstructorStringParser constructor_string_parser(
      utf8_string.AsStringView());

  Component* protocol_component = nullptr;
  absl::Status status = constructor_string_parser.Parse(
      [=, &protocol_component,
       &exception_state](std::string_view protocol_string)
          -> base::expected<bool, absl::Status> {
        protocol_component = Component::Compile(
            String::FromUtf8(protocol_string), Component::Type::kProtocol,
            /*protocol_component=*/nullptr, *options, exception_state);
        if (exception_state.HadException()) {
          return base::unexpected(
              absl::InvalidArgumentError("Failed to compile protocol"));
        }
        return protocol_component &&
               protocol_component->ShouldTreatAsStandardURL();
      });

  if (exception_state.HadException()) {
    return nullptr;
  }
  if (!status.ok()) {
    exception_state.ThrowTypeError(
        StrCat({"Invalid input string '", input_string,
                "'. It unexpectedly fails to tokenize."}));
    return nullptr;
  }
  URLPatternInit* init =
      MakeURLPatternInit(constructor_string_parser.GetResult());

  if (!base_url && !init->hasProtocol()) {
    exception_state.ThrowTypeError(
        StrCat({"Relative constructor string '", input_string,
                "' must have a base URL passed as the second argument."}));
    return nullptr;
  }

  if (base_url)
    init->setBaseURL(base_url);

  return Create(init, protocol_component, options, exception_state);
}

URLPattern* URLPattern::Create(const String& input_string,
                               const String& base_url,
                               ExceptionState& exception_state) {
  return Create(input_string, base_url,
                MakeGarbageCollected<URLPatternOptions>(), exception_state);
}

// Upstream's Create(isolate, input, options, es) and Create(isolate, input, es)
// -- the union with no base URL -- collapse into this one: with the arms
// separated, "input is a URLPatternInit" is a choice the caller already made,
// and "input is a string with no base URL" is the two-argument string overload
// above with a null `base_url`.
URLPattern* URLPattern::Create(const URLPatternInit* init,
                               ExceptionState& exception_state) {
  return Create(init, /*precomputed_protocol_component=*/nullptr,
                MakeGarbageCollected<URLPatternOptions>(), exception_state);
}

URLPattern* URLPattern::Create(const URLPatternInit* init,
                               Component* precomputed_protocol_component,
                               const URLPatternOptions* options,
                               ExceptionState& exception_state) {
  // Each component defaults to a wildcard matching any input.  We use
  // the null string as a shorthand for the default.
  String protocol;
  String username;
  String password;
  String hostname;
  String port;
  String pathname;
  String search;
  String hash;

  // Apply the input URLPatternInit on top of the default values.
  ApplyInit(init, ValueType::kPattern, protocol, username, password, hostname,
            port, pathname, search, hash, exception_state);
  if (exception_state.HadException())
    return nullptr;

  // Manually canonicalize port patterns that exactly match the default
  // port for the protocol.  We must do this separately from the compile
  // since the liburlpattern::Parse() method will invoke encoding callbacks
  // for partial values within the pattern and this transformation must apply
  // to the entire value.
  if (IsProtocolDefaultPort(protocol, port))
    port = "";

  // Compile each component pattern into a Component structure that
  // can be used for matching.

  auto* protocol_component = precomputed_protocol_component;
  if (!protocol_component) {
    protocol_component = Component::Compile(
        protocol, Component::Type::kProtocol,
        /*protocol_component=*/nullptr, *options, exception_state);
  }
  if (exception_state.HadException())
    return nullptr;

  auto* username_component =
      Component::Compile(username, Component::Type::kUsername,
                         protocol_component, *options, exception_state);
  if (exception_state.HadException())
    return nullptr;

  auto* password_component =
      Component::Compile(password, Component::Type::kPassword,
                         protocol_component, *options, exception_state);
  if (exception_state.HadException())
    return nullptr;

  auto* hostname_component =
      Component::Compile(hostname, Component::Type::kHostname,
                         protocol_component, *options, exception_state);
  if (exception_state.HadException())
    return nullptr;

  auto* port_component =
      Component::Compile(port, Component::Type::kPort,
                         protocol_component, *options, exception_state);
  if (exception_state.HadException())
    return nullptr;

  auto* pathname_component =
      Component::Compile(pathname, Component::Type::kPathname,
                         protocol_component, *options, exception_state);
  if (exception_state.HadException())
    return nullptr;

  auto* search_component =
      Component::Compile(search, Component::Type::kSearch,
                         protocol_component, *options, exception_state);
  if (exception_state.HadException())
    return nullptr;

  auto* hash_component =
      Component::Compile(hash, Component::Type::kHash,
                         protocol_component, *options, exception_state);
  if (exception_state.HadException())
    return nullptr;

  auto urlpattern_options = Options::FromV8URLPatternOptions(options);

  return MakeGarbageCollected<URLPattern>(
      protocol_component, username_component, password_component,
      hostname_component, port_component, pathname_component, search_component,
      hash_component, urlpattern_options, base::PassKey<URLPattern>());
}

URLPattern::URLPattern(Component* protocol,
                       Component* username,
                       Component* password,
                       Component* hostname,
                       Component* port,
                       Component* pathname,
                       Component* search,
                       Component* hash,
                       const Options& options,
                       base::PassKey<URLPattern> key)
    : protocol_(protocol),
      username_(username),
      password_(password),
      hostname_(hostname),
      port_(port),
      pathname_(pathname),
      search_(search),
      hash_(hash),
      options_(options) {}

// test() and exec() were here.  Both were thin wrappers around the private
// Match(v8::Isolate*, const V8URLPatternInput*, ...) below, and both took the
// WebIDL union as their input.  exec() additionally built a URLPatternResult,
// which cannot exist without a script value -- see
// MakeURLPatternComponentResult above.  Match(const KURL&, MatchResult*) is
// what a C++ caller wants from either of them.

// generate() was here.  Its signature named V8URLPatternComponent, which the
// enum generator can still produce, but also VectorOfPairs<String, String>
// from bindings/core/v8/idl_types.h, which left with the bindings generator
// and has no replacement.  url_pattern::Component::Generate() -- everything
// generate() actually did -- is untouched next door.

String URLPattern::protocol() const {
  return protocol_->GeneratePatternString();
}

String URLPattern::username() const {
  return username_->GeneratePatternString();
}

String URLPattern::password() const {
  return password_->GeneratePatternString();
}

String URLPattern::hostname() const {
  return hostname_->GeneratePatternString();
}

String URLPattern::port() const {
  return port_->GeneratePatternString();
}

String URLPattern::pathname() const {
  return pathname_->GeneratePatternString();
}

String URLPattern::search() const {
  return search_->GeneratePatternString();
}

String URLPattern::hash() const {
  return hash_->GeneratePatternString();
}

bool URLPattern::hasRegExpGroups() const {
  const url_pattern::Component* components[] = {protocol_, username_, password_,
                                                hostname_, port_,     pathname_,
                                                search_,   hash_};
  return std::ranges::any_of(components,
                             &url_pattern::Component::HasRegExpGroups);
}

// compareComponent() was here: a switch over V8URLPatternComponent dispatching
// to url_pattern::Component::Compare(), which is untouched.  It was reachable
// only from JS (`[RuntimeEnabled=URLPatternCompareComponent] static short
// compareComponent(...)`) and no C++ in the tree calls it.

bool URLPattern::Match(const KURL& url, MatchResult* result) const {
  if (!url.IsValid() || url.IsEmpty()) {
    return false;
  }
  MatchInput input;
  URLToMatchInput(url, input);
  return Match(input, result);
}

std::optional<SafeUrlPattern> URLPattern::ToSafeUrlPattern(
    ExceptionState& exception_state) const {
  String components_with_regexp;
  for (auto&& [component, name] : ComponentsWithNames()) {
    if (component->HasRegExpGroups()) {
      components_with_regexp =
          StrCat({components_with_regexp,
                  (components_with_regexp.IsNull() ? "" : ", "), name, " (",
                  component->GeneratePatternString(), ")"});
    }
  }
  if (!components_with_regexp.IsNull()) {
    exception_state.ThrowTypeError(
        StrCat({"The pattern cannot contain regexp groups, but did in the "
                "following components: ",
                components_with_regexp}));
    return std::nullopt;
  }
  CHECK(!hasRegExpGroups());

  SafeUrlPattern safe_url_pattern;
  safe_url_pattern.protocol = protocol_->PartList();
  safe_url_pattern.username = username_->PartList();
  safe_url_pattern.password = password_->PartList();
  safe_url_pattern.hostname = hostname_->PartList();
  safe_url_pattern.port = port_->PartList();
  safe_url_pattern.pathname = pathname_->PartList();
  safe_url_pattern.search = search_->PartList();
  safe_url_pattern.hash = hash_->PartList();
  safe_url_pattern.options.ignore_case = options_.ignore_case;

  return safe_url_pattern;
}

String URLPattern::ToString() const {
  StringBuilder builder;
  builder.Append("(");
  Vector<String> components = {protocol(), username(), password(), hostname(),
                               port(),     pathname(), search(),   hash()};
  builder.AppendRange(components, ",", [](const auto& component) {
    return component == g_empty_string ? " " : component;
  });
  builder.Append(")");
  return builder.ReleaseString();
}

void URLPattern::Trace(Visitor* visitor) const {
  visitor->Trace(protocol_);
  visitor->Trace(username_);
  visitor->Trace(password_);
  visitor->Trace(hostname_);
  visitor->Trace(port_);
  visitor->Trace(pathname_);
  visitor->Trace(search_);
  visitor->Trace(hash_);
  ScriptWrappable::Trace(visitor);
}

// The private Match(v8::Isolate*, const V8URLPatternInput*, base_url,
// URLPatternResult*, ExceptionState&) was here: the shared body of test() and
// exec().  It destructured the WebIDL union into a MatchInput -- either by
// running ApplyInit() over a URLPatternInit under a v8::TryCatch, or by
// resolving a string against a base URL -- and then, when a result was asked
// for, filled a URLPatternResult with the union values it had been handed plus
// eight URLPatternComponentResults.  Every one of those steps is either a
// script value or a caller that no longer exists.  URLPattern::Match(const
// KURL&, MatchResult*) above reaches the same MatchInput from a parsed URL.

void URLPattern::URLToMatchInput(const KURL& url, MatchInput& input) {
  if (url.Protocol()) {
    input.protocol = url.Protocol();
  }
  if (!url.User().empty()) {
    input.username = url.User().ToString();
  }
  if (!url.Pass().empty()) {
    input.password = url.Pass().ToString();
  }
  if (!url.Host().empty()) {
    input.hostname = url.Host().ToString();
  }
  if (url.Port() > 0) {
    input.port = String::Number(url.Port());
  }
  if (!url.GetPath().empty()) {
    input.pathname = url.GetPath().ToString();
  }
  if (!url.Query().empty()) {
    input.search = url.Query().ToString();
  }
  if (url.HasFragmentIdentifier()) {
    input.hash = url.FragmentIdentifier().ToString();
  }
}

bool URLPattern::Match(const MatchInput& input, MatchResult* result) const {
  CHECK(protocol_);
  CHECK(username_);
  CHECK(password_);
  CHECK(hostname_);
  CHECK(port_);
  CHECK(pathname_);
  CHECK(search_);
  CHECK(hash_);

  // Each component of the pattern must match the corresponding component of the
  // input. If we are not generating a full result, we don't need to populate
  // group lists. Then just pass nullptr.
  auto* protocol_result = result ? &result->protocol : nullptr;
  auto* username_result = result ? &result->username : nullptr;
  auto* password_result = result ? &result->password : nullptr;
  auto* hostname_result = result ? &result->hostname : nullptr;
  auto* port_result = result ? &result->port : nullptr;
  auto* pathname_result = result ? &result->pathname : nullptr;
  auto* search_result = result ? &result->search : nullptr;
  auto* hash_result = result ? &result->hash : nullptr;
  return protocol_->Match(input.protocol, protocol_result) &&
         username_->Match(input.username, username_result) &&
         password_->Match(input.password, password_result) &&
         hostname_->Match(input.hostname, hostname_result) &&
         port_->Match(input.port, port_result) &&
         pathname_->Match(input.pathname, pathname_result) &&
         search_->Match(input.search, search_result) &&
         hash_->Match(input.hash, hash_result);
}

}  // namespace blink
