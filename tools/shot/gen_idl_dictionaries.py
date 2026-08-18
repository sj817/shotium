"""Generate blink's IDL dictionary classes from the .idl, without V8.

Companion to gen_idl_enums.py; read that one first, it is the simpler case and
the same reasoning applies.

An IDL dictionary is an options bag, and blink core builds them itself all over
the place -- it does not wait for a JavaScript caller:

    FocusOptions* options = FocusOptions::Create();
    ...
    if (options->preventScroll()) ...

The generator that produced those classes went with V8, so the classes vanished
and took `blink::FocusOptions`, `blink::ScrollIntoViewOptions` and friends with
them -- as incomplete types, several hundred errors deep inside core/dom.

What is dropped relative to upstream's output:

  * Create(v8::Isolate*, v8::Local<v8::Value>, ExceptionState&) and
    FillMembersFromV8Object -- the conversion *from* a script value.
  * TemplateKey()/FillTemplateProperties()/FillValues() -- the conversion *to*
    one. bindings::DictionaryBase no longer declares them (see
    platform/bindings/dictionary_base.h).

What is kept is what blink's own code calls: Create(), the per-member
has<Name>()/<name>()/set<Name>() triple, the defaults declared in the .idl, and
Trace().

Member types that only exist to carry a script value -- `any`, `object`,
`record<>`, `Promise<>`, callback types -- have no representation without V8.
A dictionary containing one is skipped whole and reported, rather than emitted
with a member quietly missing: a dictionary that silently lost a field would
compile and then behave wrong, which is worse than not compiling.

Usage:
  gen_idl_dictionaries.py [-n] [--only Name[,Name...]]

  -n       dry run
  --only   generate just these dictionaries (and whatever they inherit from)
"""

import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RENDERER = os.path.join(ROOT, "third_party", "blink", "renderer")

# Matches a double-quoted string OR a // comment, so the substitution
# below can keep the string and drop only the comment. Anchoring on //
# alone truncates every default value that happens to be a URL.
COMMENT = re.compile(r'("(?:[^"\\]|\\.)*")|//[^\n]*')
EXT_ATTR = re.compile(r"\[[^\]]*\]")
# An extended-attribute block runs for many lines and the declaration follows
# it on the *same* line as the closing bracket:
#
#     [
#       Exposed=Window,
#       ...
#     ] interface Element : Node {
#
# so anchoring these at the start of a line finds `dictionary FocusOptions`
# and misses every interface in core -- which then shows up as
# "unknown type Element" rather than as a parsing problem.
# DICT matches only the *head* of a dictionary. The body is taken by counting
# braces (dict_body below), not by a regex, because a member's default value can
# contain braces:
#
#   dictionary SetHTMLUnsafeOptions {
#     (Sanitizer or SanitizerConfig or SanitizerPresets) sanitizer = {};
#     boolean runScripts = false;
#   };
#
# `\{(.*?)\}\s*;` stops at the `};` of `= {};`, so the body was everything up to
# `sanitizer = {` and `runScripts` vanished -- silently, which is the one thing
# the module docstring above promises this generator does not do. It surfaced
# three waves later as "no member named 'runScripts' in SetHTMLUnsafeOptions"
# at a call site, pointing at the class rather than at the parse.
DICT = re.compile(
    r"(?:^|\]\s*)dictionary\s+([A-Za-z_]\w*)\s*(?::\s*([A-Za-z_]\w*)\s*)?\{",
    re.M)
IFACE = re.compile(
    r"(?:^|\]\s*)(?:partial\s+)?interface\s+(?:mixin\s+)?([A-Za-z_]\w*)", re.M)
ENUM = re.compile(r"^\s*enum\s+([A-Za-z_]\w*)\s*\{", re.M)


def dict_body(src, open_brace):
    """Text between the dictionary's `{` and its matching `}`.

    `open_brace` is the index just past the `{` that DICT matched. Returns None
    if the braces do not balance, which means the .idl is malformed and the
    caller should say so rather than guess.
    """
    depth = 1
    i = open_brace
    n = len(src)
    while i < n:
        c = src[i]
        if c == '"':
            i += 1
            while i < n and src[i] != '"':
                i += 2 if src[i] == "\\" else 1
        elif c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return src[open_brace:i]
        i += 1
    return None


# IDL primitive -> (C++ type, needs Member<>, default initialiser)
PRIMITIVES = {
    "boolean": ("bool", False, "false"),
    "byte": ("int8_t", False, "0"),
    "octet": ("uint8_t", False, "0"),
    "short": ("int16_t", False, "0"),
    "unsigned short": ("uint16_t", False, "0"),
    "long": ("int32_t", False, "0"),
    "unsigned long": ("uint32_t", False, "0"),
    "long long": ("int64_t", False, "0"),
    "unsigned long long": ("uint64_t", False, "0"),
    "float": ("float", False, "0"),
    "unrestricted float": ("float", False, "0"),
    "double": ("double", False, "0"),
    "unrestricted double": ("double", False, "0"),
    "DOMHighResTimeStamp": ("double", False, "0"),
    "DOMString": ("String", False, None),
    "USVString": ("String", False, None),
    "ByteString": ("String", False, None),
    "CSSOMString": ("String", False, None),
}

# No representation without a script engine.
UNSUPPORTED = ("any", "object", "record<", "Promise<", "ArrayBuffer",
               "ArrayBufferView", "BufferSource", "Function")

HEADER = '''// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Generated by tools/shot/gen_idl_dictionaries.py from %(idl)s.
// Do not edit; edit the .idl and re-run the generator.

#ifndef %(guard)s
#define %(guard)s

%(includes)s
namespace blink {

%(forwards)s// IDL dictionary %(name)s.
class %(export)s%(cls)s : public %(base)s {
 public:
  static %(cls)s* Create() { return MakeGarbageCollected<%(cls)s>(); }

  %(cls)s() = default;
  ~%(cls)s() override = default;

%(accessors)s
  // Defined in the .cc: tracing a Member<T> needs T complete, and including
  // T's header here would close a cycle back through v8_typedefs.h.
  void Trace(Visitor* visitor) const override;

 private:
%(fields)s};

}  // namespace blink

#endif  // %(guard)s
'''


IMPL = """// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Generated by tools/shot/gen_idl_dictionaries.py from %(idl)s.
// Do not edit; edit the .idl and re-run the generator.

#include "%(header)s"

%(includes)s
namespace blink {

void %(cls)s::Trace(Visitor* visitor) const {
%(traces)s  %(base)s::Trace(visitor);
}

}  // namespace blink
"""


def strip_comments(text):
    """Remove // comments without touching string literals."""
    return COMMENT.sub(lambda m: m.group(1) or "", text)

def snake(name):
    out = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", name)
    out = re.sub(r"(?<=[A-Z])(?=[A-Z][a-z])", "_", out)
    return out.lower()


def upper_first(name):
    return name[:1].upper() + name[1:]


def scan():
    """Collect every dictionary, interface and enum declared in the tree."""
    dicts, ifaces, enums, idl_of = {}, set(), set(), {}
    for dirpath, _dirs, files in os.walk(RENDERER):
        for fname in files:
            if not fname.endswith(".idl"):
                continue
            path = os.path.join(dirpath, fname)
            src = strip_comments(io.open(path, encoding="utf-8",
                                          errors="replace").read())
            rel = os.path.relpath(path, ROOT).replace(os.sep, "/")
            for m in DICT.finditer(src):
                body = dict_body(src, m.end())
                if body is None:
                    raise Unsupported("%s: unbalanced braces in dictionary %s"
                                      % (rel, m.group(1)))
                dicts.setdefault(m.group(1), (rel, m.group(2), body))
                idl_of.setdefault(m.group(1), rel)
            for m in IFACE.finditer(src):
                ifaces.add(m.group(1))
                idl_of.setdefault(m.group(1), rel)
            for m in ENUM.finditer(src):
                enums.add(m.group(1))
                idl_of.setdefault(m.group(1), rel)
    return dicts, ifaces, enums, idl_of


class Unsupported(Exception):
    pass


_UNION_INDEX = None


def union_index():
    """(member token tuple) -> (class name, header path), plus the Names object
    needed to turn a written union back into that tuple.

    Built by asking gen_idl_unions.py, so the naming rule lives in exactly one
    place. Keying on tokens rather than on the written spelling is what makes
    `(double or CSSNumericValue)` and `(double or CSSNumberish)` -- one class,
    two spellings -- both resolve.
    """
    global _UNION_INDEX
    if _UNION_INDEX is not None:
        return _UNION_INDEX
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import gen_idl_unions as U

    sources = {}
    for path in U.idl_files():
        sources[path] = io.open(path, encoding="utf-8",
                                errors="replace").read()
    defs, typedefs = U.collect_definitions_and_typedefs(sources)
    names = U.Names(typedefs)
    unions, _problems = U.collect_unions(sources, names)

    index = {}
    for tokens, union in unions.items():
        header = ("third_party/blink/renderer/bindings/%s/v8/%s.h"
                  % (U.component_of(union, defs, names), union.basename))
        if not os.path.exists(os.path.join(ROOT, header.replace("/", os.sep))):
            continue                     # the union generator skipped this one
        index[tokens] = (union.cls, header)
    _UNION_INDEX = (U, names, index, typedefs, defs)
    return _UNION_INDEX


def resolve_typedef_union(name):
    """A typedef name whose target is a union -> (alias class, header)."""
    U, names, index, typedefs, _defs = union_index()
    if name not in typedefs:
        return None, None
    node = typedefs[name][0]
    while node.kind == "nullable":
        node = node.args[0]
    if node.kind != "union":
        return None, None
    try:
        tokens = names.member_tokens(node)
    except RecursionError:
        return None, None
    if tokens not in index:
        return None, None
    cls, header = index[tokens]
    # The union's own class name and header, not the alias and v8_typedefs.h.
    #
    # `typedef (DOMString or SanitizerAttributeNamespace) SanitizerAttribute;`
    # makes V8SanitizerAttribute an alias for
    # V8UnionSanitizerAttributeNamespaceOrString, and both spell the same type,
    # so a call site may use either. A *generated* file may not use the alias,
    # because reaching it means including v8_typedefs.h, and v8_typedefs.h
    # includes the union headers -- which (since unions include the generated
    # dictionaries they hold) can lead back here. Then the second visit to
    # v8_typedefs.h is a no-op on the include guard and the alias is not defined
    # yet at the point of use. The diagnostic is 1452 copies of
    #     use of undeclared identifier 'V8SanitizerAttribute'
    # in a generated header that plainly includes the header defining it.
    #
    # Naming the union directly keeps generated headers free of v8_typedefs.h
    # entirely, which is what makes the graph among them acyclic.
    return cls, header


def resolve_union(text):
    """Written union type -> (class name, header path), or (None, None)."""
    U, names, index, _typedefs, _defs = union_index()
    parsed = U.TypeParser(U.lex(text)).union(0)
    if parsed is None:
        return None, None
    try:
        tokens = names.member_tokens(parsed[0])
    except RecursionError:
        return None, None
    return index.get(tokens, (None, None))


def cpp_type(idl_type, dicts, ifaces, enums, idl_of, includes):
    """IDL type -> (C++ type, is_garbage_collected, default initialiser)."""
    t = idl_type.strip()
    nullable = t.endswith("?")
    if nullable:
        t = t[:-1].strip()

    if any(u in t for u in UNSUPPORTED):
        raise Unsupported(t)
    if t.startswith("("):
        # A union member. The class name and header path are gen_idl_unions.py's
        # rule -- member tokens flattened, unwrapped, sorted, joined with "Or"
        # -- and it is reused rather than restated here. Two copies of that rule
        # would agree right up until one of them was fixed.
        cls, header = resolve_union(t)
        if cls is None:
            raise Unsupported("union %s" % t)
        includes["h"].add(header)
        return cls, True, None

    m = re.fullmatch(r"sequence<(.+)>", t)
    if m:
        inner, gc, _d = cpp_type(m.group(1), dicts, ifaces, enums, idl_of,
                                 includes)
        if gc:
            includes["h"].add("third_party/blink/renderer/platform/heap/"
                              "collection_support/heap_vector.h")
            return "HeapVector<Member<%s>>" % inner, True, None
        includes["h"].add("third_party/blink/renderer/platform/wtf/vector.h")
        return "Vector<%s>" % inner, False, None

    if t in PRIMITIVES:
        cpp, gc, default = PRIMITIVES[t]
        if nullable and cpp not in ("String",):
            return "std::optional<%s>" % cpp, False, None
        return cpp, gc, default

    if t in enums:
        includes["h"].add(
            "third_party/blink/renderer/bindings/core/v8/v8_%s.h"
            % snake(t))
        return "V8%s" % t, False, None

    if t in dicts:
        header = header_for(t, dicts, idl_of)
        if header is None:
            raise Unsupported("no header for %s" % t)
        includes["h"].add(header)
        return t, True, None

    if t in ifaces:
        header = header_for(t, dicts, idl_of)
        if header is None:
            raise Unsupported("no header for %s" % t)
        # The IDL interface name is not always the C++ class name.
        # core/frame/window.idl declares `interface Window` with
        # `[ImplementedAs=DOMWindow]`, so UIEventInit's `view` member is a
        # DOMWindow*. Emitting `class Window;` next to an include of
        # dom_window.h gave a Member<> of a type nothing ever defines, and the
        # diagnostic was "invalid application of 'sizeof' to an incomplete type
        # 'blink::Window'" inside cppgc's visitor.h. The union generator has
        # always honoured [ImplementedAs]; this one now reads the same table.
        _U, _names, _index, _typedefs, defs = union_index()
        d = defs.get(t)
        cpp = d.impl if d is not None else t
        # An interface header is the cycle: element.h reaches back here through
        # v8_typedefs.h. Forward-declare, include in the .cc.
        includes["cc"].add(header)
        includes["fwd"].add(cpp)
        return cpp, True, None

    cls, header = resolve_typedef_union(t)
    if cls is not None:
        includes["h"].add(header)
        return cls, True, None

    raise Unsupported("unknown type %s" % t)


def header_for(name, dicts, idl_of):
    """Where the C++ declaration of an IDL-declared type lives."""
    if name in dicts:
        part = "modules" if "/renderer/modules/" in dicts[name][0] else "core"
        return ("third_party/blink/renderer/bindings/%s/v8/v8_%s.h"
                % (part, snake(name)))
    rel = idl_of.get(name)
    if not rel:
        return None
    candidate = rel[:-4] + ".h"          # sibling header of the .idl
    if os.path.exists(os.path.join(ROOT, candidate.replace("/", os.sep))):
        return candidate
    # [ImplementedAs=X] renames the C++ class, and the header follows the class
    # rather than the .idl: window.idl declares `interface Window` but the
    # header next to it is dom_window.h. Without this, UIEventInit and
    # everything below it is skipped for "no header for Window".
    impl = implemented_as(rel, name)
    if impl:
        sibling = os.path.join(os.path.dirname(rel), snake(impl) + ".h")
        if os.path.exists(os.path.join(ROOT, sibling.replace("/", os.sep))):
            return sibling
    return None


def implemented_as(idl_rel, name):
    """The C++ class name an interface is [ImplementedAs=...] declared with."""
    src = io.open(os.path.join(ROOT, idl_rel.replace("/", os.sep)),
                  encoding="utf-8", errors="replace").read()
    m = re.search(r"\[([^\]]*)\]\s*(?:partial\s+)?interface\s+(?:mixin\s+)?"
                  + re.escape(name) + r"\b", src, re.S)
    if not m:
        return None
    a = re.search(r"ImplementedAs\s*=\s*(\w+)", m.group(1))
    return a.group(1) if a else None


def members_of(body):
    """Parse a dictionary body into (required, idl_type, name, cpp_name,
    default)."""
    out = []
    for raw in body.split(";"):
        impl = re.search(r"ImplementedAs\s*=\s*(\w+)", raw)
        decl = EXT_ATTR.sub("", raw).strip()
        if not decl:
            continue
        default = None
        if "=" in decl:
            decl, default = decl.split("=", 1)
            decl, default = decl.strip(), default.strip()
        required = decl.startswith("required ")
        if required:
            decl = decl[len("required "):].strip()
        parts = decl.rsplit(None, 1)
        if len(parts) != 2:
            raise Unsupported("cannot parse member %r" % raw.strip())
        name = parts[1].strip()
        out.append((required, parts[0].strip(), name,
                    impl.group(1) if impl else name, default))
    return out


def first_enum_entry(name, idl_of):
    """The first enumerator of an IDL enum, for zero-initialising a member."""
    rel = idl_of.get(name)
    if not rel:
        return None
    src = io.open(os.path.join(ROOT, rel.replace("/", os.sep)),
                  encoding="utf-8", errors="replace").read()
    src = strip_comments(src)
    m = re.search(r"enum\s+" + re.escape(name) + r"\s*\{(.*?)\}", src, re.S)
    if not m:
        return None
    values = re.findall(r'"([^"]*)"', m.group(1))
    return enum_entry(values[0]) if values else None

def literal(default, cpp, enum_type):
    """IDL default value -> C++ initialiser."""
    if default in ("true", "false"):
        return default
    if default == "null":
        return None
    if default.startswith('"'):
        if enum_type:
            return None                  # handled by the caller: needs Enum::k
        return "%s" % default            # String from a literal
    if default in ("[]", "{}"):
        return None
    return default                       # numeric


def generate(name, dicts, ifaces, enums, idl_of, dry):
    idl_rel, parent, body = dicts[name]
    # Three buckets: what the header includes, what the .cc includes, and what
    # the header forward-declares in place of a .cc include.
    includes = {"h": set(), "cc": set(), "fwd": set()}
    accessors, fields, traces = [], [], []

    if parent:
        if parent not in dicts:
            raise Unsupported("parent %s is not a dictionary" % parent)
        base = parent
        includes["h"].add(header_for(parent, dicts, idl_of))
    else:
        base = "bindings::DictionaryBase"
        includes["h"].add("third_party/blink/renderer/platform/bindings/"
                          "dictionary_base.h")

    for required, idl_type, member, cpp_name, default in members_of(body):
        cpp, gc, zero = cpp_type(idl_type, dicts, ifaces, enums, idl_of,
                                 includes)
        field = "member_%s_" % snake(member)
        has = "has_%s_" % snake(member)
        cap = upper_first(cpp_name)

        init = ""
        present = "false"
        if default is not None:
            enum_default = idl_type.strip().rstrip("?") in enums
            if enum_default and default.startswith(chr(34)):
                init = " = V8%s(V8%s::Enum::%s)" % (
                    idl_type.strip().rstrip("?"), idl_type.strip().rstrip("?"),
                    enum_entry(default.strip(chr(34))))
                present = "true"
            else:
                lit = literal(default, cpp, enum_default)
                if lit is not None:
                    init = " = %s" % lit
                    present = "true"
        elif zero is not None:
            init = " = %s" % zero
        elif cpp.startswith("V8") and not gc:
            # An IDL enum: no default constructor, so pick value 0. has_..._
            # stays false, so nothing reads it until it is set.
            enum_name = idl_type.strip().rstrip("?")
            first = first_enum_entry(enum_name, idl_of)
            if first is None:
                raise Unsupported("no enumerator for %s" % enum_name)
            init = " = %s(%s::Enum::%s)" % (cpp, cpp, first)

        if gc:
            # A HeapVector is already a heap collection: it is traced directly
            # and is not itself a GarbageCollected type, so wrapping one in
            # Member<> trips cppgc's IsGarbageCollectedOrMixinType assertion.
            decl = cpp if cpp.startswith("HeapVector<") else "Member<%s>" % cpp
            fields.append("  %s %s;\n" % (decl, field))
            traces.append("  visitor->Trace(%s);\n" % field)
            if cpp.startswith("HeapVector<"):
                ret = "const %s&" % cpp
                arg = "const %s& value" % cpp
                get, setv = field, "%s = value" % field
            else:
                ret, arg = "%s*" % cpp, "%s* value" % cpp
                get, setv = "%s.Get()" % field, "%s = value" % field
        else:
            fields.append("  %s %s%s;\n" % (cpp, field, init))
            ref = "" if cpp in ("bool", "double", "float") or \
                cpp.startswith(("int", "uint")) else "const "
            ret = cpp
            arg = "%s%s%s value" % (ref, cpp, "&" if ref else "")
            get, setv = field, "%s = value" % field
        fields.append("  bool %s = %s;\n" % (has, present))

        accessors.append(
            "  bool has%s() const { return %s; }\n"
            "  %s %s() const { return %s; }\n"
            "  void set%s(%s) { %s; %s = true; }\n"
            % (cap, has, ret, cpp_name, get, cap, arg, setv, has))
        # getFooOr(fallback): upstream emits one for every member, and call
        # sites use it wherever "absent" and "present with this value" mean the
        # same thing --
        #     shadow_root_init_dict->getSerializableOr(false)
        #     config->getCommentsOr(allowCommentsAndDataAttributes)
        #     options->getEventTypeOr(String())
        # Leaving it out turns every such call site into a build error that
        # reads as a missing dictionary member rather than a missing accessor,
        # which is what sent one of these to the wrong file entirely.
        accessors.append(
            "  %s get%sOr(%s) const { return %s ? %s : value; }\n\n"
            % (ret, cap, arg, has, get))
        if required:
            # Upstream enforces required-ness while converting from a script
            # value, which is the code path that is gone. Nothing else can
            # check it, so record it and move on.
            accessors.append("  // %s is `required` in %s.\n\n"
                             % (member, os.path.basename(idl_rel)))

    part = "modules" if "/renderer/modules/" in idl_rel else "core"
    out_dir = os.path.join(RENDERER, "bindings", part, "v8")
    guard = ("THIRD_PARTY_BLINK_RENDERER_BINDINGS_%s_V8_V8_%s_H_"
             % (part.upper(), snake(name).upper()))

    inc_lines = ('#include "third_party/blink/renderer/platform/heap/'
                 'garbage_collected.h"\n'
                 '#include "third_party/blink/renderer/platform/heap/member.h"\n'
                 '#include "third_party/blink/renderer/platform/wtf/'
                 'text/wtf_string.h"\n')
    inc_lines += "".join('#include "%s"\n' % i
                         for i in sorted(includes["h"]) if i)
    inc_lines += ('#include "third_party/blink/renderer/%s/%s_export.h"\n\n'
                  % (part, part))
    fwd_lines = "".join("class %s;\n" % f for f in sorted(includes["fwd"]))
    if fwd_lines:
        fwd_lines += "\n"

    subs = {
        "idl": idl_rel,
        "guard": guard,
        "name": name,
        "cls": name,
        "base": base,
        "export": "CORE_EXPORT " if part == "core" else "MODULES_EXPORT ",
        "includes": inc_lines,
        "forwards": fwd_lines,
        "accessors": "".join(accessors),
        "fields": "".join(fields),
        "traces": "".join(traces),
        "header": "third_party/blink/renderer/bindings/%s/v8/v8_%s.h"
                  % (part, snake(name)),
    }
    header_text = HEADER % subs
    impl_subs = dict(subs)
    impl_subs["includes"] = "".join('#include "%s"\n' % i
                                    for i in sorted(includes["cc"]) if i)
    impl_text = IMPL % impl_subs

    if not dry:
        os.makedirs(out_dir, exist_ok=True)
        for suffix, text in ((".h", header_text), (".cc", impl_text)):
            io.open(os.path.join(out_dir, "v8_%s%s" % (snake(name), suffix)),
                    "w", encoding="utf-8", newline="\n").write(text)
    return True


def enum_entry(value):
    if value == "":
        return "kEmptyString"
    parts = [p for p in re.split(r"[^A-Za-z0-9]+", value) if p]
    return "k" + "".join("SRGB" if p.lower() == "srgb"
                         else (p if p[:2].isupper() else upper_first(p))
                         for p in parts)


def main(argv):
    dry = "-n" in argv
    argv = [a for a in argv if a != "-n"]
    only = None
    if "--only" in argv:
        i = argv.index("--only")
        only = set(argv[i + 1].split(","))

    dicts, ifaces, enums, idl_of = scan()
    names = sorted(only) if only else sorted(dicts)

    done = skipped = 0
    reasons = {}
    queue = list(names)
    seen = set()
    while queue:
        name = queue.pop(0)
        if name in seen:
            continue
        seen.add(name)
        if name not in dicts:
            reasons[name] = "not a dictionary in any .idl"
            skipped += 1
            continue
        parent = dicts[name][1]
        if parent and parent not in seen:
            queue.append(parent)
        try:
            generate(name, dicts, ifaces, enums, idl_of, dry)
            done += 1
        except Unsupported as e:
            reasons[name] = str(e)
            skipped += 1

    print("%d dictionaries generated, %d skipped%s"
          % (done, skipped, " (dry run)" if dry else ""))
    for name in sorted(reasons):
        print("  skip   %-42s %s" % (name, reasons[name]))


if __name__ == "__main__":
    main(sys.argv[1:])
