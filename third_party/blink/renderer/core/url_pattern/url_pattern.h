// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_URL_PATTERN_URL_PATTERN_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_URL_PATTERN_URL_PATTERN_H_

#include <array>
#include <utility>

#include "base/types/pass_key.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_typedefs.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_url_pattern_component.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/url_pattern/url_pattern_component.h"
#include "third_party/blink/renderer/core/url_pattern/url_pattern_options.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "third_party/liburlpattern/parse.h"

namespace blink {

class ExceptionState;
class KURL;
struct SafeUrlPattern;
class URLPatternInit;
class URLPatternOptions;

// URLPattern survives V8's removal because CSS needs it: `url-pattern()` in a
// <navigation-location> and the `pattern`/`protocol`/`hostname`/... descriptors
// of `@location` both compile to one of these on the ordinary style-resolution
// path (see core/css/navigation_query.cc and core/css/style_rule_location.cc).
// What did not survive is everything on the JS side of the class:
//
//   From(V8URLPatternCompatible*)  -- converts a WebIDL argument that could be
//       a URLPattern, a URLPatternInit or a string.  There is no WebIDL
//       argument; every caller is gone with the APIs that took one.
//   test() / exec()  -- exec() returns a URLPatternResult whose per-component
//       `groups` is a `record<USVString, any>`; upstream filled it with
//       ScriptValues holding v8::Undefined for the groups that did not match.
//       There is no `any` without a script value, so the dictionary cannot be
//       built, and Match(const KURL&, MatchResult*) below is the same query
//       asked in C++ terms.  It is what core/css calls.
//   generate()  -- takes a V8URLPatternComponent and a VectorOfPairs<> from
//       bindings/core/v8/idl_types.h, which is one of the headers that left
//       with the bindings generator.  url_pattern::Component::Generate(), the
//       part that does the work, is untouched.
//   compareComponent()  -- also keyed by V8URLPatternComponent, and never
//       called from C++.
//
// The Create() overloads below are upstream's, with the V8URLPatternInput
// union -- the WebIDL `(USVString or URLPatternInit)` argument -- split back
// into the two things it could hold.  See url_pattern.idl.
class CORE_EXPORT URLPattern : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

  using Options = url_pattern::Options;
  using Component = url_pattern::Component;

 public:
  // Compile a URLPattern constructor string -- "https://example.com/foo/:id",
  // the thing `url-pattern()` and `@location { pattern: ... }` hold -- relative
  // to `base_url`.  `base_url` may be null, in which case the string must
  // carry its own protocol.
  //
  // Upstream this was Create(v8::Isolate*, const V8URLPatternInput*, ...): the
  // isolate was only ever forwarded to Component::Compile to reach V8's RegExp
  // engine, and the union was the WebIDL `(USVString or URLPatternInit)`
  // argument, which the first thing the function did was destructure with
  // GetContentType() -- throwing a TypeError if it turned out to hold the
  // dictionary rather than the string.  Both callers in core/css knew perfectly
  // well which arm they had; they were wrapping a plain String in a
  // MakeGarbageCollected<V8URLPatternInput> so that this function could unwrap
  // it again.  Taking the String is the same call without the round trip, and
  // it makes the "second argument baseURL provided with a URLPatternInit"
  // TypeError unrepresentable rather than merely unreachable.
  static URLPattern* Create(const String& input_string,
                            const String& base_url,
                            const URLPatternOptions* options,
                            ExceptionState& exception_state);

  static URLPattern* Create(const String& input_string,
                            const String& base_url,
                            ExceptionState& exception_state);

  // Compile the components given individually.  This is the other arm of the
  // old union, and `URLPatternInit::baseURL` is where the base URL goes.
  static URLPattern* Create(const URLPatternInit* init,
                            ExceptionState& exception_state);

  static URLPattern* Create(const URLPatternInit* init,
                            Component* precomputed_protocol_component,
                            const URLPatternOptions* options,
                            ExceptionState& exception_state);

  URLPattern(Component* protocol,
             Component* username,
             Component* password,
             Component* hostname,
             Component* port,
             Component* pathname,
             Component* search,
             Component* hash,
             const Options& options,
             base::PassKey<URLPattern> key);

  // test(), exec() and generate() were here.  See the class comment.

  String protocol() const;
  String username() const;
  String password() const;
  String hostname() const;
  String port() const;
  String pathname() const;
  String search() const;
  String hash() const;

  bool hasRegExpGroups() const;

  // compareComponent() was here.  See the class comment;
  // url_pattern::Component::Compare(), which it dispatched to, is untouched.

  // Matched :group values. This is essentially a C++ version of
  // the URLPatternResult API (but without inputs).
  struct MatchResult {
    Vector<std::pair<String, String>> protocol;
    Vector<std::pair<String, String>> username;
    Vector<std::pair<String, String>> password;
    Vector<std::pair<String, String>> hostname;
    Vector<std::pair<String, String>> port;
    Vector<std::pair<String, String>> pathname;
    Vector<std::pair<String, String>> search;
    Vector<std::pair<String, String>> hash;
  };

  // Match the specified URL against this pattern. Return true if it's a match,
  // false otherwise. Populate `result` with matched :group values in each
  // component, if not nullptr.
  bool Match(const KURL& url, MatchResult* = nullptr) const;

  // Throws a TypeError if the pattern does not meet the requirements to be
  // safe. i.e. has no regexp groups.
  std::optional<SafeUrlPattern> ToSafeUrlPattern(
      ExceptionState& exception_state) const;

  // Used for testing and debugging.
  String ToString() const;

  void Trace(Visitor* visitor) const override;

 private:
  // The private Match(v8::Isolate*, const V8URLPatternInput*, base_url,
  // URLPatternResult*, ExceptionState&) was here.  It was the shared body of
  // test() and exec(): it resolved the WebIDL input into a MatchInput and then
  // marshalled the group values into a URLPatternResult.  With both of its
  // callers gone, what is left of it is Match(const MatchInput&) below.

  // String representations of each component of a URL used to match against a
  // URLPattern.
  struct MatchInput {
    String protocol = g_empty_string;
    String username = g_empty_string;
    String password = g_empty_string;
    String hostname = g_empty_string;
    String port = g_empty_string;
    String pathname = g_empty_string;
    String search = g_empty_string;
    String hash = g_empty_string;
  };

  static void URLToMatchInput(const KURL&, MatchInput&);
  bool Match(const MatchInput&, MatchResult* = nullptr) const;

  std::array<std::pair<const Member<Component>&, const char*>, 8>
  ComponentsWithNames() const {
    return {{{protocol_, "protocol"},
             {username_, "username"},
             {password_, "password"},
             {hostname_, "hostname"},
             {port_, "port"},
             {pathname_, "pathname"},
             {search_, "search"},
             {hash_, "hash"}}};
  }

  bool ShouldTreatAsStandardURL() const {
    CHECK(protocol_);
    return protocol_->ShouldTreatAsStandardURL();
  }

  // The compiled patterns for each URL component.
  Member<Component> protocol_;
  Member<Component> username_;
  Member<Component> password_;
  Member<Component> hostname_;
  Member<Component> port_;
  Member<Component> pathname_;
  Member<Component> search_;
  Member<Component> hash_;
  const Options options_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_URL_PATTERN_URL_PATTERN_H_
