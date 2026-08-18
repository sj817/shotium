"""Generate blink's V8Union<...> classes from the .idl files, without V8.

Companion to gen_idl_enums.py.  Same problem, same shape of answer: blink's own
C++ uses the bindings generator's union classes as its type vocabulary --

    V8UnionCSSNumericValueOrDouble* v = ...;
    if (v->IsCSSNumericValue()) return v->GetAsCSSNumericValue();
    MakeGarbageCollected<V8UnionStringOrTrustedHTML>(String("x"));

-- the generator that emitted them left with V8, and the .idl files it read
did not.  So the classes are regenerated from the same input, minus everything
whose only job was to talk to a JavaScript engine.

Dropped relative to upstream's output:

  * Create(v8::Isolate*, v8::Local<v8::Value>, ExceptionState&) and the whole
    union-conversion algorithm.  There is no JS value to convert.
  * ToV8() / DirectToV8() / bindings::OptimizedReturnProxy, and the
    NativeValueTraits / ToV8Traits specialisations.
  * The "subunion" accessors -- IsV8UnionAOrB()/GetAsV8UnionAOrB()/Set() that
    upstream adds to a union whose member set is a superset of another union's.
    Nothing in renderer/core or renderer/platform calls one (checked with
    `git grep -E '(GetAs|Is)V8Union'`, which finds nothing), and emitting them
    would make every union header include every smaller union's header.

Kept: the ContentType enum, one constructor per member type, GetContentType(),
Is<Member>()/GetAs<Member>()/Set(), Clear(), Trace() and UnionNameInIDL().
Everything is inline in the header, so unlike upstream there is no .cc and no
CORE_EXPORT: with no out-of-line definition there is nothing to export, this
tree builds is_component_build=false so CORE_EXPORT expands to nothing anyway,
and nothing outside renderer/core names a union (`git grep V8Union` over
renderer/platform finds nothing), so no component boundary is crossed.

THE NAMING RULE
---------------
This is the part that has to be exactly right, because the call sites were
written against upstream's generator.  The rule was read out of the generator's
own source at the pristine revision (it is still in git even though it is no
longer in the tree):

  bind_gen/blink_v8_bridge.py  blink_class_name()
      "V8Union" + "Or".join(union.member_tokens)
  web_idl/union.py             Union.unique_token()
      sorted(type_name_with_extended_attribute_key_values of each member,
             after recursively flattening nested unions and unwrapping
             typedefs and nullables), then "Null" appended -- after the sort,
             so it is always last -- if the union includes a nullable type.
  bind_gen/path_manager.py
      file name = ("v8_union_" + "_".join(member_tokens)).lower()
      i.e. lower-cased, NOT snake_cased: the underscore separates union
      members, so a member must not contain one.

and the type names themselves from web_idl/idl_type.py:

  SimpleType       NameStyleConverter(name).to_upper_camel_case(), except
                   DOMString -> String.  Note this means USVString stays
                   USVString and ByteString stays ByteString; only DOMString
                   collapses.  V8UnionOpaquePropertyOrUSVString and
                   v8_union_boolean_bytestringbytestringrecord.h at the real
                   call sites confirm it.
  DefinitionType   the identifier as written (CSSNumericValue, not CssNumericValue)
  sequence<T>      type_name(T) + "Sequence"     FrozenArray<T> -> T + "Array"
  record<K, V>     type_name(K) + type_name(V) + "Record"
  T?               type_name(T) + "OrNull"
  nested union     "Or".join(sorted(member type names))
  annotations      the type's extended-attribute keys, sorted, appended:
                   [LegacyNullToEmptyString] DOMString -> StringLegacyNullToEmptyString

The tokenizer is blink's own NameStyleConverter, imported from
renderer/build/scripts (still in the tree), rather than reimplemented -- that
is how "USVString" and "unsigned long" come out right without a table of
special cases.

Worked example, the nastiest one in the tree.  From
core/animation/base_property_indexed_keyframe.idl:

    (KeyframeOffset? or sequence<KeyframeOffset?>) offset = [];
    typedef (double or TimelineRangeOffset or DOMString) KeyframeOffset;

  member 1 unwraps to the union -> tokens Double, TimelineRangeOffset, String
  member 2 is a sequence, whose element is a nullable typedef of a union
           -> "DoubleOrStringOrTimelineRangeOffsetOrNull" + "Sequence"
  the union includes a nullable type -> trailing "Null"
  sorted + Null:
    Double, DoubleOrStringOrTimelineRangeOffsetOrNullSequence, String,
    TimelineRangeOffset, Null
  -> v8_union_double_doubleorstringortimelinerangeoffsetornullsequence_
       string_timelinerangeoffset_null.h

which is character-for-character the include upstream's core has.  --check is
what keeps all of this honest.

NAMED TYPEDEF UNIONS
--------------------
`typedef (double or CSSNumericValue) CSSNumberish;` does not get a class of its
own upstream either.  bind_gen/typedef.py emits, into a single header
bindings/<component>/v8/v8_typedefs.h,

    using V8CSSNumberish = V8UnionCSSNumericValueOrDouble;

plus aliases for unions written in terms of a typedef'ed member.  We generate
the same file, because that is the include core actually has (v8_typedefs.h
appears in ~30 core headers at the pristine revision) and restoring those
includes is a separate tool's job.

WHAT IS SKIPPED, AND WHY
------------------------
A union member that is an interface or dictionary is stored as Member<T> and
traced, so T must be complete: the header has to #include the member's own
header.  We resolve that the way upstream's PathManager does --

  interface    the .idl's sibling header, named after the *implementation*
               class, so [ImplementedAs=LocalDOMWindow] on interface Window
               resolves to core/frame/local_dom_window.h, not window.h
  enumeration  bindings/<component>/v8/v8_<snake>.h, produced by gen_idl_enums.py
  dictionary   bindings/<component>/v8/v8_<snake>.h, produced by
               gen_idl_dictionaries.py -- see below

-- and if the resolved header does not exist on disk, the union is skipped and
reported rather than emitted with an include that would fail somewhere else.

Dictionaries are the one exception to "skip if the header is missing".
gen_idl_dictionaries.py skips a dictionary whose members it cannot express, so
a handful of them have no header, and a union over one of those would be
dropped too.  The path is not a guess -- it is PathManager's rule, the same one
that puts the enum headers where gen_idl_enums.py already puts them -- so the
union is emitted with the include it will need and listed under PENDING, which
says plainly that it cannot compile until that dictionary exists.  Note the
dependency runs both ways: a dictionary with a union member needs the union
header, so re-running gen_idl_dictionaries.py after this script can turn a
PENDING into a real one.

Members that are `any`, `object` or a Promise are V8 values (ScriptValue,
ScriptObject, ScriptPromise) and their unions are skipped outright.

Usage:
  gen_idl_unions.py [-n] [-v] [--check]

  -n       dry run
  -v       list every union produced, with the header it went to
  --check  cross-check against the call sites: every V8Union.../V8<Typedef>
           identifier named in renderer/core and renderer/platform must be
           produced, and every ContentType::k... and GetAs...() must be a
           member of something we produce.
"""

import io
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RENDERER = os.path.join(ROOT, "third_party", "blink", "renderer")
PRISTINE = "c0bba1026178"  # upstream baseline: the clone root, no cuts.

# blink's own name tokenizer, the one bind_gen/name_style.py wraps.  Imported
# rather than reimplemented: it is what decides that "USVString" is USV+String
# and "unsigned long" is Unsigned+Long, and getting that subtly wrong is
# exactly the failure mode this script has to avoid.
sys.path.insert(0, os.path.join(RENDERER, "build", "scripts"))
try:
    from blinkbuild.name_style_converter import NameStyleConverter
except ImportError:  # pragma: no cover
    sys.exit("gen_idl_unions.py needs third_party/blink/renderer/build/scripts/"
             "blinkbuild/name_style_converter.py, which is not in the tree.")


def upper_camel(name):
    return NameStyleConverter(name).to_upper_camel_case()


def snake(name):
    return NameStyleConverter(name).to_snake_case()


# ---------------------------------------------------------------------------
# IDL lexing and type parsing
#
# This is deliberately a type-expression parser and not an IDL parser.  Every
# union in Web IDL is written "(" T ("or" T)+ ")", so it is enough to try to
# parse a union at every "(" in the file: an argument list or an
# [Exposed=(Window,Worker)] simply fails to parse and is discarded.  That
# catches unions wherever they appear -- argument, attribute, return type,
# dictionary member, typedef, and nested inside sequence<> or another union --
# without having to model the rest of the grammar.
# ---------------------------------------------------------------------------

BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.S)
LINE_COMMENT = re.compile(r"//[^\n]*")
TOKEN = re.compile(r'"[^"]*"|[A-Za-z_][0-9A-Za-z_]*|[0-9][0-9A-Za-z_.]*|\.\.\.|\S')
IDENT = re.compile(r"[A-Za-z_][0-9A-Za-z_]*\Z")

# SimpleType._VALID_TYPES from web_idl/idl_type.py.  These are spelled as
# built-in types, never resolved against a definition.
INTEGER_TYPES = ("byte", "octet", "short", "unsigned short", "long",
                 "unsigned long", "long long", "unsigned long long")
FLOAT_TYPES = ("float", "unrestricted float", "double", "unrestricted double")
NUMERIC_TYPES = FLOAT_TYPES + INTEGER_TYPES
STRING_TYPES = ("DOMString", "ByteString", "USVString")
TYPED_ARRAY_TYPES = ("Int8Array", "Int16Array", "Int32Array", "BigInt64Array",
                     "Uint8Array", "Uint16Array", "Uint32Array",
                     "BigUint64Array", "Uint8ClampedArray", "Float16Array",
                     "Float32Array", "Float64Array")
BUFFER_SOURCE_TYPES = ("ArrayBuffer", "ArrayBufferView",
                       "DataView") + TYPED_ARRAY_TYPES
MISC_TYPES = ("any", "bigint", "boolean", "object", "symbol", "undefined",
              "void")
SIMPLE_TYPES = frozenset(NUMERIC_TYPES + STRING_TYPES + BUFFER_SOURCE_TYPES +
                         MISC_TYPES)

# Words that start a declaration or modify one.  An identifier used as a type
# can never be one of these, and refusing them keeps the "try to parse a union
# here" scan from mistaking e.g. `(optional X y)` for a type.
NOT_A_TYPE_NAME = frozenset((
    "or", "optional", "attribute", "readonly", "static", "const", "stringifier",
    "getter", "setter", "deleter", "inherit", "required", "typedef",
    "interface", "dictionary", "enum", "namespace", "callback", "partial",
    "includes", "mixin", "iterable", "maplike", "setlike", "async",
    "constructor",
))

CONTAINERS = {
    "sequence": "sequence",
    "FrozenArray": "frozen_array",
    "ObservableArray": "observable_array",
    "Promise": "promise",
}


class Type(object):
    """A parsed IDL type expression.

    kind is one of: simple, named, sequence, frozen_array, observable_array,
    promise, record, union, nullable.  `args` holds the composed types
    (element, key/value, union members, nullable inner) and `ext` the
    extended attributes written on this type, as (key, values) pairs.
    """

    __slots__ = ("kind", "name", "args", "ext")

    def __init__(self, kind, name=None, args=(), ext=()):
        self.kind = kind
        self.name = name
        self.args = tuple(args)
        self.ext = tuple(ext)

    def __repr__(self):
        return "Type(%s,%s,%d)" % (self.kind, self.name, len(self.args))


def lex(src):
    src = BLOCK_COMMENT.sub(" ", src)
    src = LINE_COMMENT.sub(" ", src)
    return TOKEN.findall(src)


class TypeParser(object):
    def __init__(self, toks):
        self.t = toks

    def at(self, i):
        return self.t[i] if 0 <= i < len(self.t) else None

    def ext_attrs(self, i):
        """`[` ExtAttr ("," ExtAttr)* `]` -> ([(key, values)], next_index)."""
        assert self.at(i) == "["
        i += 1
        out = []
        while True:
            key = self.at(i)
            if key is None or not IDENT.match(key):
                return None
            i += 1
            values = []
            if self.at(i) == "=":
                i += 1
                if self.at(i) == "(":
                    i += 1
                    while self.at(i) not in (")", None):
                        if self.at(i) != ",":
                            values.append(self.at(i))
                        i += 1
                    if self.at(i) is None:
                        return None
                    i += 1
                else:
                    if self.at(i) is None:
                        return None
                    values.append(self.at(i))
                    i += 1
            if self.at(i) == "(":  # legacy [Constructor(...)]-style argument list
                depth = 0
                while self.at(i) is not None:
                    if self.at(i) == "(":
                        depth += 1
                    elif self.at(i) == ")":
                        depth -= 1
                        if depth == 0:
                            i += 1
                            break
                    i += 1
            out.append((key, tuple(values)))
            if self.at(i) == ",":
                i += 1
                continue
            if self.at(i) == "]":
                return out, i + 1
            return None

    def type(self, i):
        ext = ()
        if self.at(i) == "[":
            r = self.ext_attrs(i)
            if r is None:
                return None
            ext, i = r
            ext = tuple(ext)
        if self.at(i) == "(":
            r = self.union(i)
        else:
            r = self.single(i)
        if r is None:
            return None
        node, i = r
        node.ext = ext
        # A nullable wraps the annotated type: `[X] long?` is `([X] long)?`,
        # which is why the extended attributes land on the inner node.
        if self.at(i) == "?":
            node = Type("nullable", args=[node])
            i += 1
        return node, i

    def union(self, i):
        assert self.at(i) == "("
        i += 1
        members = []
        r = self.type(i)
        if r is None:
            return None
        node, i = r
        members.append(node)
        if self.at(i) != "or":
            return None  # an argument list, not a union
        while self.at(i) == "or":
            r = self.type(i + 1)
            if r is None:
                return None
            node, i = r
            members.append(node)
        if self.at(i) != ")":
            return None
        return Type("union", args=members), i + 1

    def single(self, i):
        tok = self.at(i)
        if tok is None:
            return None
        if tok in CONTAINERS:
            if self.at(i + 1) != "<":
                return None
            r = self.type(i + 2)
            if r is None:
                return None
            inner, i = r
            if self.at(i) != ">":
                return None
            return Type(CONTAINERS[tok], args=[inner]), i + 1
        if tok == "record":
            if self.at(i + 1) != "<":
                return None
            r = self.type(i + 2)
            if r is None:
                return None
            key, i = r
            if self.at(i) != ",":
                return None
            r = self.type(i + 1)
            if r is None:
                return None
            value, i = r
            if self.at(i) != ">":
                return None
            return Type("record", args=[key, value]), i + 1
        if tok == "unsigned":
            nxt = self.at(i + 1)
            if nxt == "short":
                return Type("simple", name="unsigned short"), i + 2
            if nxt == "long":
                if self.at(i + 2) == "long":
                    return Type("simple", name="unsigned long long"), i + 3
                return Type("simple", name="unsigned long"), i + 2
            return None
        if tok == "long":
            if self.at(i + 1) == "long":
                return Type("simple", name="long long"), i + 2
            return Type("simple", name="long"), i + 1
        if tok == "unrestricted":
            if self.at(i + 1) in ("float", "double"):
                return Type("simple",
                            name="unrestricted " + self.at(i + 1)), i + 2
            return None
        if tok in SIMPLE_TYPES:
            return Type("simple", name=tok), i + 1
        if IDENT.match(tok) and tok not in NOT_A_TYPE_NAME:
            return Type("named", name=tok), i + 1
        return None


# ---------------------------------------------------------------------------
# The IDL definition table: what each identifier names, and where it lives.
# ---------------------------------------------------------------------------

DEF_RE = re.compile(
    r"(?:(?P<ext>\[[^\]]*\])\s*)?"
    r"(?P<partial>partial\s+)?"
    r"(?P<kind>callback\s+interface|interface\s+mixin|interface|dictionary"
    r"|namespace|enum)\s+"
    r"(?P<name>[A-Za-z_][0-9A-Za-z_]*)"
    r"(?:\s*:\s*[A-Za-z_][0-9A-Za-z_]*)?\s*\{", re.S)
CALLBACK_FN_RE = re.compile(
    r"\bcallback\s+(?P<name>[A-Za-z_][0-9A-Za-z_]*)\s*=")
IMPLEMENTED_AS_RE = re.compile(r"\bImplementedAs\s*=\s*([A-Za-z_][0-9A-Za-z_]*)")


class Definition(object):
    __slots__ = ("name", "kind", "idl", "impl")

    def __init__(self, name, kind, idl, impl):
        self.name = name
        self.kind = kind      # interface|dictionary|enum|callback|namespace|mixin
        self.idl = idl        # repo-relative posix path
        self.impl = impl      # blink implementation class name


def idl_files():
    for dirpath, _dirs, files in os.walk(RENDERER):
        for name in sorted(files):
            if name.endswith(".idl"):
                yield os.path.join(dirpath, name)


def rel(path):
    return os.path.relpath(path, ROOT).replace(os.sep, "/")


DEF_KINDS = {
    "interface": "interface",
    "namespace": "namespace",
    "interface mixin": "mixin",
    "callback interface": "callback",
    "dictionary": "dictionary",
    "enum": "enum",
}


def collect_definitions_and_typedefs(sources):
    """-> ({name: Definition}, {name: (Type, idl)})."""
    defs = {}
    typedefs = {}
    for path, src in sources.items():
        text = LINE_COMMENT.sub(" ", BLOCK_COMMENT.sub(" ", src))
        for m in DEF_RE.finditer(text):
            kind = DEF_KINDS[re.sub(r"\s+", " ", m.group("kind"))]
            name = m.group("name")
            impl = name
            if kind in ("enum", "callback"):
                impl = "V8" + name
            elif m.group("ext"):
                ia = IMPLEMENTED_AS_RE.search(m.group("ext"))
                if ia:
                    impl = ia.group(1)
            if m.group("partial"):
                # A partial definition contributes members to a name; it does
                # not establish what that name *is*, and its [ImplementedAs] is
                # the C++ class implementing those members, not the interface's
                # own class. Letting one register the name produced
                #     explicit V8UnionElementOrHTMLCollection(
                #         ElementComputedStyleMap* value)
                # because core/css/cssom/element_computed_style_map.idl says
                #     [ImplementedAs=ElementComputedStyleMap]
                #     partial interface Element
                # and sorts before core/dom/element.idl. So partials are skipped
                # outright: if a name has no non-partial definition in this tree,
                # it is genuinely undefined and must be reported, not guessed.
                continue
            if name in defs and defs[name].kind == kind:
                continue
            defs[name] = Definition(name, kind, path, impl)
        for m in CALLBACK_FN_RE.finditer(text):
            name = m.group("name")
            if name not in defs:
                defs[name] = Definition(name, "callback", path, "V8" + name)

    for path, src in sources.items():
        toks = lex(src)
        parser = TypeParser(toks)
        for i, tok in enumerate(toks):
            if tok != "typedef":
                continue
            r = parser.type(i + 1)
            if r is None:
                continue
            node, j = r
            name = parser.at(j)
            if name is None or not IDENT.match(name) or parser.at(j + 1) != ";":
                continue
            typedefs.setdefault(name, (node, path))
    return defs, typedefs


# ---------------------------------------------------------------------------
# type_name, per web_idl/idl_type.py
# ---------------------------------------------------------------------------

class Names(object):
    """Computes upstream's type names.  Needs the typedef table to unwrap."""

    def __init__(self, typedefs):
        self.typedefs = typedefs

    def original(self, t):
        """The type a `named` node refers to, if it is a typedef."""
        if t.kind == "named" and t.name in self.typedefs:
            return self.typedefs[t.name][0]
        return None

    def annotations(self, t, depth=0):
        """effective_annotations: own extended attributes, plus, through a
        typedef, the ones written on the typedef'ed type."""
        if depth > 16:
            return ()
        if t.kind == "nullable":
            return self.annotations(t.args[0], depth + 1)
        orig = self.original(t)
        if orig is not None:
            return tuple(self.annotations(orig, depth + 1)) + tuple(t.ext)
        return tuple(t.ext)

    def bare(self, t, depth=0):
        """type_name_without_extended_attributes."""
        if depth > 32:
            raise RecursionError("cyclic typedef")
        k = t.kind
        if k == "simple":
            return upper_camel("String" if t.name == "DOMString" else t.name)
        if k == "named":
            orig = self.original(t)
            if orig is not None:
                return self.bare(orig, depth + 1)
            return t.name
        if k == "sequence":
            return self.full(t.args[0], depth + 1) + "Sequence"
        if k == "frozen_array":
            return self.full(t.args[0], depth + 1) + "Array"
        if k == "observable_array":
            return self.full(t.args[0], depth + 1) + "ObservableArray"
        if k == "promise":
            return self.full(t.args[0], depth + 1) + "Promise"
        if k == "record":
            return (self.full(t.args[0], depth + 1) +
                    self.full(t.args[1], depth + 1) + "Record")
        if k == "nullable":
            return self.bare(t.args[0], depth + 1) + "OrNull"
        if k == "union":
            return "Or".join(
                sorted(self.full(m, depth + 1) for m in t.args))
        raise AssertionError(k)

    def full(self, t, depth=0):
        """type_name: the bare name plus the sorted annotation keys."""
        keys = sorted(set(k for k, _v in self.annotations(t)))
        return self.bare(t, depth) + "".join(keys)

    def token(self, t, depth=0):
        """type_name_with_extended_attribute_key_values, the spelling that
        actually goes into a union class name."""
        ann = self.annotations(t)
        pieces = [self.bare(t, depth)]
        for key in sorted(set(k for k, _v in ann)):
            pieces.append(key)
            for k, values in ann:
                if k == key:
                    pieces.extend(values)
        return "".join(pieces)

    def unwrap(self, t, depth=0):
        """IdlType.unwrap(): strip nullable and typedef wrappers."""
        while depth < 32:
            if t.kind == "nullable":
                t = t.args[0]
            else:
                orig = self.original(t)
                if orig is None:
                    return t
                t = orig
            depth += 1
        return t

    def includes_nullable(self, t, depth=0):
        if depth > 32:
            return False
        if t.kind == "nullable":
            return True
        if t.kind == "union":
            return any(self.includes_nullable(m, depth + 1) for m in t.args)
        orig = self.original(t)
        if orig is not None:
            return self.includes_nullable(orig, depth + 1)
        return False

    def member_tokens(self, union):
        """Union.unique_token(): the sorted member tokens, plus a trailing
        "Null" -- after the sort -- if the union includes a nullable type."""
        pieces = []

        def collect(t, depth=0):
            if depth > 32:
                raise RecursionError("cyclic union")
            u = self.unwrap(t)
            if u.kind == "union":
                for m in u.args:
                    collect(m, depth + 1)
            else:
                pieces.append(self.token(u))

        collect(union)
        pieces = sorted(set(pieces))
        if self.includes_nullable(union):
            pieces.append("Null")
        return tuple(pieces)

    def flattened(self, union):
        """UnionType.flattened_member_types, ordered the way upstream orders
        them (by token), so members line up with member_tokens()."""
        out = {}

        def flatten(t, depth=0):
            if depth > 32:
                raise RecursionError("cyclic union")
            if t.kind == "union":
                for m in t.args:
                    flatten(m, depth + 1)
                return
            orig = self.original(t)
            if orig is not None:
                flatten(orig, depth + 1)
                return
            if t.kind == "nullable":
                flatten(t.args[0], depth + 1)
                return
            out.setdefault(self.token(t), t)

        flatten(union)
        return [out[k] for k in sorted(out)]

    def syntactic_form(self, t, depth=0):
        """IdlType.syntactic_form, for UnionNameInIDL()."""
        if depth > 32:
            return "?"
        k = t.kind
        if k == "simple":
            body = t.name
        elif k == "named":
            body = t.name
        elif k == "sequence":
            body = "sequence<%s>" % self.syntactic_form(t.args[0], depth + 1)
        elif k == "frozen_array":
            body = "FrozenArray<%s>" % self.syntactic_form(t.args[0], depth + 1)
        elif k == "observable_array":
            body = "ObservableArray<%s>" % self.syntactic_form(
                t.args[0], depth + 1)
        elif k == "promise":
            body = "Promise<%s>" % self.syntactic_form(t.args[0], depth + 1)
        elif k == "record":
            body = "record<%s, %s>" % (self.syntactic_form(t.args[0], depth + 1),
                                       self.syntactic_form(t.args[1], depth + 1))
        elif k == "union":
            return "(%s)" % " or ".join(
                self.syntactic_form(m, depth + 1) for m in t.args)
        elif k == "nullable":
            return "%s?" % self.syntactic_form(t.args[0], depth + 1)
        else:
            raise AssertionError(k)
        if t.ext:
            body = "[%s] %s" % (", ".join(k for k, _v in t.ext), body)
        return body


# ---------------------------------------------------------------------------
# blink_type_info, per bind_gen/blink_v8_bridge.py, minus the V8-only cases.
# ---------------------------------------------------------------------------

NUMERIC_CXX = {
    "byte": "int8_t", "octet": "uint8_t",
    "short": "int16_t", "unsigned short": "uint16_t",
    "long": "int32_t", "unsigned long": "uint32_t",
    "long long": "int64_t", "unsigned long long": "uint64_t",
    "float": "float", "unrestricted float": "float",
    "double": "double", "unrestricted double": "double",
}

WTF_STRING_H = "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
WTF_VECTOR_H = "third_party/blink/renderer/platform/wtf/vector.h"
HEAP_VECTOR_H = ("third_party/blink/renderer/platform/heap/collection_support/"
                 "heap_vector.h")
MEMBER_H = "third_party/blink/renderer/platform/heap/member.h"
TYPED_ARRAYS = "third_party/blink/renderer/core/typed_arrays/"


class Pending(Exception):
    """The member's header has a known generated path that nothing has written
    yet (dictionaries).  Carried as the path itself."""


class Unsupported(Exception):
    """A member type that cannot be expressed without V8, or whose header is
    not in this tree.  The message is what --check and the run log print."""


class Info(object):
    """The subset of upstream's TypeInfo the generated code actually uses."""

    __slots__ = ("value_t", "member_t", "member_ref_t", "is_gc", "is_traceable",
                 "is_move_effective", "clear", "deref", "init", "includes",
                 # A blink class that is only pointed at. Its header goes in the
                 # .cc and the .h forward-declares it instead, because
                 # including it from the .h is a cycle: css_numeric_value.h
                 # includes v8_typedefs.h, which includes every union header,
                 # one of which would include css_numeric_value.h again. The
                 # include guard wins that race and CSSNumericValue is then
                 # incomplete inside the union -- reported as "use of
                 # undeclared identifier 'CSSNumericValue'" in generated code.
                 "fwd", "impl_includes")

    def __init__(self, value_t, member_t=None, member_ref_t=None, is_gc=False,
                 is_traceable=False, is_move_effective=False, clear=None,
                 deref="", init="", includes=(), fwd=None, impl_includes=()):
        self.value_t = value_t
        self.member_t = member_t or value_t
        self.member_ref_t = member_ref_t or value_t
        self.is_gc = is_gc
        self.is_traceable = is_traceable or is_gc
        self.is_move_effective = is_move_effective
        self.clear = clear if clear is not None else "%s.Clear()"
        self.deref = deref            # "" or ".Get()"
        self.init = init              # default member initializer
        self.includes = tuple(includes)
        self.fwd = fwd
        self.impl_includes = tuple(impl_includes)


class TypeInfoBuilder(object):
    def __init__(self, defs, names):
        self.defs = defs
        self.names = names
        # tokens tuple -> (class name, header path) for unions that are
        # themselves producible.  A union can appear inside another one, most
        # often as sequence<(A or B)>, and is then stored as Member<V8UnionAOrB>
        # -- which needs V8UnionAOrB's header, so it needs V8UnionAOrB to exist.
        self.union_index = {}

    def header_for(self, d):
        """Where the class implementing definition |d| is declared.  Mirrors
        PathManager: colocated with the .idl for interfaces, in the generated
        bindings directory for enums, dictionaries and callbacks."""
        idl_rel = rel(d.idl)
        if d.kind in ("interface", "namespace"):
            path = "%s/%s.h" % (os.path.dirname(idl_rel), snake(d.impl))
            if not os.path.exists(os.path.join(ROOT, path)):
                raise Unsupported("%s: no implementation header at %s"
                                  % (d.name, path))
            return path
        component = ("modules" if "/renderer/modules/" in idl_rel else "core")
        base = "third_party/blink/renderer/bindings/%s/v8/" % component
        candidates = ["v8_" + snake(d.name)]
        # gen_idl_enums.py keeps digit runs attached (v8_rgba_float16.h, not
        # v8_rgba_float_16.h) where NameStyleConverter splits them.  The enum
        # headers on disk are that script's output, so accept either spelling
        # and prefer the one that is really there.
        alt = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", d.name)
        alt = re.sub(r"(?<=[A-Z])(?=[A-Z][a-z])", "_", alt).lower()
        if alt not in candidates:
            candidates.append("v8_" + alt)
        for cand in candidates:
            if os.path.exists(os.path.join(ROOT, base + cand + ".h")):
                return base + cand + ".h"
        if d.kind == "dictionary":
            # gen_idl_dictionaries.py has not produced this one.  Two reasons
            # are possible and they need opposite treatment:
            #
            #  - ordering: the dictionaries have simply not been generated yet.
            #    The union is emitted and reported as pending.
            #  - the dictionary is one gen_idl_dictionaries.py *skips*, because
            #    a member has a type that cannot be expressed without V8 (an
            #    `any`, an `object`, a script callback).  Then the union cannot
            #    be expressed either, and emitting it produces a .cc that
            #    includes a header nobody will ever write.
            #
            # Distinguishing them from here is not possible, so the emitted
            # union is checked against the tree afterwards; see Union.emit.
            raise Pending(base + candidates[0] + ".h")
        raise Unsupported("%s: generated %s header %s is not in the tree"
                          % (d.name, d.kind, base + candidates[0] + ".h"))

    def build(self, t):
        """A flattened union member type -> Info.  Raises Unsupported."""
        k = t.kind
        ext_keys = set(key for key, _v in self.names.annotations(t))

        if k == "simple":
            return self.simple(t, ext_keys)
        if k == "named":
            return self.named(t)
        if k in ("sequence", "frozen_array"):
            return self.array(t)
        if k == "record":
            return self.record(t)
        if k == "union":
            return self.nested_union(t)
        if k == "observable_array":
            raise Unsupported("ObservableArray needs the V8 backing store")
        if k == "promise":
            raise Unsupported("Promise is a ScriptPromise, which needs V8")
        raise Unsupported("unexpected member kind %s" % k)

    def nested_union(self, t):
        tokens = self.names.member_tokens(t)
        entry = self.union_index.get(tokens)
        if entry is None:
            raise Unsupported("nested union V8Union%s is not produced"
                              % "Or".join(tokens))
        return self.gc(entry[0], entry[1])

    def simple(self, t, ext_keys):
        name = t.name
        if name == "boolean":
            return Info("bool", clear="%s = false", init=" = false")
        if name in NUMERIC_CXX:
            cxx = NUMERIC_CXX[name]
            return Info(cxx, clear="%s = 0", init=" = 0",
                        includes=(("<cstdint>",) if cxx.endswith("_t") else ()))
        if name in STRING_TYPES:
            return Info("String", member_ref_t="const String&",
                        is_move_effective=True, clear="%s = String()",
                        includes=(WTF_STRING_H,))
        if name in ("undefined", "void"):
            # Upstream stores an empty ToV8UndefinedGenerator tag here.  The
            # struct itself has no V8 in it; it was declared in the deleted
            # bindings/core/v8/native_value_traits_impl.h and has been moved
            # into platform/bindings/union_base.h, which every union includes.
            return Info("ToV8UndefinedGenerator",
                        member_ref_t="const ToV8UndefinedGenerator&", clear="")
        if name == "ArrayBuffer":
            if "AllowShared" in ext_keys:
                return self.gc("DOMArrayBufferBase",
                               TYPED_ARRAYS + "dom_array_buffer_base.h")
            return self.gc("DOMArrayBuffer",
                           TYPED_ARRAYS + "dom_array_buffer.h")
        if name in BUFFER_SOURCE_TYPES:
            wrapper = "MaybeShared" if "AllowShared" in ext_keys else "NotShared"
            impl = "DOM" + name
            if name == "ArrayBufferView":
                header = TYPED_ARRAYS + "dom_array_buffer_view.h"
            elif name == "DataView":
                header = TYPED_ARRAYS + "dom_data_view.h"
            else:
                header = TYPED_ARRAYS + "dom_typed_array.h"
            if not os.path.exists(os.path.join(ROOT, header)):
                raise Unsupported("%s: %s is not in the tree" % (name, header))
            return Info("%s<%s>" % (wrapper, impl), is_gc=True,
                        includes=(TYPED_ARRAYS + "array_buffer_view_helpers.h",
                                  header))
        if name == "any":
            raise Unsupported("`any` is a ScriptValue, which needs V8")
        if name == "object":
            raise Unsupported("`object` is a ScriptObject, which needs V8")
        if name == "bigint":
            raise Unsupported("`bigint` is a BigInt, which needs V8")
        if name == "symbol":
            raise Unsupported("blink does not accept the IDL symbol type")
        raise Unsupported("unhandled built-in type %s" % name)

    GENERATED = "third_party/blink/renderer/bindings/"

    def gc(self, cxx, header):
        # A bare `Member<T>` field can be declared with T incomplete, but
        # `HeapVector<Member<T>>` cannot: instantiating the vector instantiates
        # Member<T>'s kAffinity, whose initialiser is
        # `ThreadingTrait<T>::kAffinity`, and choosing between ThreadingTrait's
        # partial specialisations evaluates `std::derived_from<T, blink::Node>`.
        # That needs T complete. This cost a build round: the diagnostic is
        # "incomplete type used in type trait expression" inside libc++'s
        # is_base_of.h, five notes away from the member that caused it.
        #
        # So the header is included, unless T is a *non-generated* blink class.
        # Those are the ones the .h cannot include: the cycle that forced the
        # .h/.cc split runs impl header -> v8_typedefs.h -> union header ->
        # impl header. Another generated bindings class closes no cycle,
        # because generated headers no longer include impl headers at all.
        if header.startswith(self.GENERATED):
            return Info(cxx + "*", member_t="Member<%s>" % cxx,
                        member_ref_t=cxx + "*", is_gc=True, deref=".Get()",
                        includes=(MEMBER_H, header))
        return Info(cxx + "*", member_t="Member<%s>" % cxx,
                    member_ref_t=cxx + "*", is_gc=True, deref=".Get()",
                    includes=(MEMBER_H,), fwd=cxx, impl_includes=(header,))

    def named(self, t):
        d = self.defs.get(t.name)
        if d is None:
            raise Unsupported("%s: no IDL definition in this tree" % t.name)
        if d.kind == "enum":
            # IDL enumerations are value types, and the class has no default
            # constructor -- upstream initialises the member with enum value 0
            # and relies on content_type_ to keep it from being read.
            header = self.header_for(d)
            return Info(d.impl, member_ref_t=d.impl, clear="",
                        init="{static_cast<%s::Enum>(0)}" % d.impl,
                        includes=(header,))
        if d.kind in ("interface", "dictionary", "namespace", "callback"):
            try:
                header = self.header_for(d)
            except Pending as p:
                info = self.gc(d.impl, str(p))
                info.includes = info.includes + ("!pending",)
                return info
            return self.gc(d.impl, header)
        if d.kind == "mixin":
            raise Unsupported("%s: interface mixins are not types" % t.name)
        raise Unsupported("%s: unsupported definition kind %s"
                          % (t.name, d.kind))

    def array(self, t):
        elem = self.build(self.names.unwrap(t.args[0]))
        if self.names.includes_nullable(t.args[0]) and not elem.is_gc:
            raise Unsupported("nullable non-pointer sequence element")
        if elem.is_traceable:
            cxx = "HeapVector<%s>" % elem.member_t
            includes = (HEAP_VECTOR_H,) + elem.includes
        else:
            cxx = "Vector<%s>" % elem.value_t
            includes = (WTF_VECTOR_H,) + elem.includes
        return Info(cxx, member_ref_t="const %s&" % cxx,
                    is_move_effective=True, is_traceable=elem.is_traceable,
                    clear="%s.clear()", includes=includes,
                    fwd=elem.fwd, impl_includes=elem.impl_includes)

    def record(self, t):
        key = self.build(self.names.unwrap(t.args[0]))
        if key.value_t != "String":
            raise Unsupported("record key is not a string type")
        value = self.build(self.names.unwrap(t.args[1]))
        if value.is_traceable:
            cxx = "HeapVector<std::pair<String, %s>>" % value.member_t
            includes = (HEAP_VECTOR_H, WTF_STRING_H, "<utility>")
        else:
            cxx = "Vector<std::pair<String, %s>>" % value.value_t
            includes = (WTF_VECTOR_H, WTF_STRING_H, "<utility>")
        return Info(cxx, member_ref_t="const %s&" % cxx,
                    is_move_effective=True, is_traceable=value.is_traceable,
                    clear="%s.clear()", includes=includes + value.includes,
                    fwd=value.fwd, impl_includes=value.impl_includes)


# ---------------------------------------------------------------------------
# Code generation
# ---------------------------------------------------------------------------

FILE_TMPL = """// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Generated by tools/shot/gen_idl_unions.py from %(idl)s.
// Do not edit; edit the .idl and re-run the generator.

#ifndef %(guard)s
#define %(guard)s

%(includes)s
namespace blink {

%(forwards)s// IDL union %(syntax)s.
class %(cls)s final : public bindings::UnionBase {
 public:
  // The type of the content value of this IDL union.
  enum class ContentType {
%(content_types)s  };

%(ctors)s
  // Returns the type of the content value.
  ContentType GetContentType() const { return content_type_; }

%(accessors)s  // Defined in the .cc: tracing a Member<T> needs T complete, and T's
  // header cannot be included here without a cycle.
  void Trace(Visitor* visitor) const override;

  static constexpr const char* UnionNameInIDL() { return "%(idl_name)s"; }

 private:
  void Clear() {
%(clear)s  }

  ContentType content_type_;

%(members)s};

}  // namespace blink

#endif  // %(guard)s
"""


IMPL_TMPL = """// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Generated by tools/shot/gen_idl_unions.py from %(idl)s.
// Do not edit; edit the .idl and re-run the generator.

#include "%(header)s"

%(includes)s
namespace blink {

void %(cls)s::Trace(Visitor* visitor) const {
%(trace)s}

}  // namespace blink
"""


class Member(object):
    __slots__ = ("token", "info", "is_null", "var", "pred", "get", "content")

    def __init__(self, token, info):
        self.token = token
        self.info = info
        self.is_null = info is None
        self.var = NameStyleConverter("member " + token).to_snake_case() + "_"
        self.pred = "Is" + token
        self.get = "GetAs" + token
        self.content = "ContentType::k" + token


class Union(object):
    def __init__(self, tokens, node, idl, names):
        self.tokens = tokens
        self.node = node
        # Every spelling that maps to this class.  (A or B), (A? or B) and
        # (T or B) where `typedef A T` are one class but three spellings, and
        # bind_gen/typedef.py emits a `using` alias for each one written with a
        # typedef'ed member.
        self.nodes = [node]
        self.idl = idl
        self.cls = "V8Union" + "Or".join(tokens)
        self.basename = ("v8_union_" + "_".join(tokens)).lower()
        self.members = []
        self.pending = []
        self.pending_headers = []
        self.names = names

    def build(self, builder):
        self.members = []
        self.pending = []
        self.pending_headers = []
        flat = self.names.flattened(self.node)
        flat_tokens = [self.names.token(t) for t in flat]
        expected = [t for t in self.tokens if t != "Null"]
        if flat_tokens != expected:
            raise Unsupported("member/token mismatch %s vs %s"
                              % (flat_tokens, expected))
        for token, t in zip(flat_tokens, flat):
            info = builder.build(t)
            if "!pending" in info.includes:
                info.includes = tuple(i for i in info.includes
                                      if i != "!pending")
                self.pending.append(token)
                self.pending_headers.append(info.impl_includes[0]
                                            if info.impl_includes
                                            else info.includes[0])
            self.members.append(Member(token, info))
        if "Null" in self.tokens:
            self.members.append(Member("Null", None))


def component_of(union, defs, names):
    """core unless some member type is defined under renderer/modules."""
    found = ["core"]

    def walk(t, depth=0):
        if depth > 32:
            return
        if t.kind == "named":
            orig = names.original(t)
            if orig is not None:
                walk(orig, depth + 1)
            d = defs.get(t.name)
            if d is not None and "/renderer/modules/" in rel(d.idl):
                found[0] = "modules"
        for a in t.args:
            walk(a, depth + 1)

    walk(union.node)
    return found[0]


def render(union, component):
    includes = set(["third_party/blink/renderer/platform/bindings/union_base.h",
                    "third_party/blink/renderer/platform/heap/garbage_collected.h"])
    system = set()
    # Headers that belong in the .cc, and the classes the .h forward-declares
    # in their place. See Info.fwd for why they cannot go in the header.
    impl_includes = set()
    forwards = set()
    for m in union.members:
        if m.is_null:
            system.add("<cstddef>")
            continue
        for inc in m.info.includes:
            (system if inc.startswith("<") else includes).add(inc)
        impl_includes.update(m.info.impl_includes)
        if m.info.fwd:
            forwards.add(m.info.fwd)
        if m.info.is_move_effective:
            system.add("<utility>")

    inc_lines = []
    if system:
        inc_lines.extend("#include %s" % s for s in sorted(system))
        inc_lines.append("")
    inc_lines.append('#include "base/check_op.h"')
    inc_lines.extend('#include "%s"' % i for i in sorted(includes))

    content_types = "".join("    k%s,\n" % m.token for m in union.members)

    ctors = []
    for m in union.members:
        if m.is_null:
            ctors.append("  explicit %s(std::nullptr_t)\n"
                         "      : content_type_(%s) {}\n"
                         % (union.cls, m.content))
            continue
        check = "\n    DCHECK(value);\n  " if m.info.is_gc else ""
        ctors.append("  explicit %s(%s value)\n"
                     "      : content_type_(%s), %s(value) {%s}\n"
                     % (union.cls, m.info.member_ref_t, m.content, m.var, check))
        if m.info.is_move_effective:
            ctors.append("  explicit %s(%s&& value)\n"
                         "      : content_type_(%s), %s(std::move(value)) {}\n"
                         % (union.cls, m.info.value_t, m.content, m.var))
    ctors = "\n".join(ctors)

    acc = []
    for m in union.members:
        block = ["  bool %s() const { return content_type_ == %s; }\n"
                 % (m.pred, m.content)]
        if m.is_null:
            block.append("  void Set(std::nullptr_t) {\n"
                         "    Clear();\n"
                         "    content_type_ = %s;\n"
                         "  }\n" % m.content)
        else:
            block.append("  %s %s() const {\n"
                         "    DCHECK_EQ(content_type_, %s);\n"
                         "    return %s%s;\n"
                         "  }\n"
                         % (m.info.member_ref_t, m.get, m.content, m.var,
                            m.info.deref))
            check = "    DCHECK(value);\n" if m.info.is_gc else ""
            block.append("  void Set(%s value) {\n"
                         "%s    Clear();\n"
                         "    %s = value;\n"
                         "    content_type_ = %s;\n"
                         "  }\n"
                         % (m.info.member_ref_t, check, m.var, m.content))
            if m.info.is_move_effective:
                block.append("  void Set(%s&& value) {\n"
                             "    Clear();\n"
                             "    %s = std::move(value);\n"
                             "    content_type_ = %s;\n"
                             "  }\n"
                             % (m.info.value_t, m.var, m.content))
        acc.append("".join(block))
    accessors = "\n".join(acc) + "\n"

    trace = "".join("    visitor->Trace(%s);\n" % m.var
                    for m in union.members
                    if not m.is_null and m.info.is_traceable)
    trace += "    bindings::UnionBase::Trace(visitor);\n"

    clear = "".join("    %s;\n" % (m.info.clear % m.var)
                    for m in union.members
                    if not m.is_null and m.info.clear)
    if not clear:
        clear = "    // No member holds a resource that needs releasing.\n"

    members = "".join("  %s %s%s;\n" % (m.info.member_t, m.var, m.info.init)
                      for m in union.members if not m.is_null)

    path = "third_party/blink/renderer/bindings/%s/v8/%s.h" % (component,
                                                               union.basename)
    guard = re.sub(r"[^A-Za-z0-9]", "_", path).upper() + "_"

    syntax = union.names.syntactic_form(union.node)
    idl_name = "(%s)%s" % (
        " or ".join(sorted(union.names.syntactic_form(t)
                           for t in union.names.flattened(union.node))),
        "?" if union.names.includes_nullable(union.node) else "")

    fwd_lines = "".join("class %s;\n" % f for f in sorted(forwards))
    if fwd_lines:
        fwd_lines += "\n"

    header_text = FILE_TMPL % {
        "idl": union.idl,
        "guard": guard,
        "cls": union.cls,
        "syntax": syntax,
        "idl_name": idl_name,
        "includes": "\n".join(inc_lines) + "\n",
        "forwards": fwd_lines,
        "content_types": content_types,
        "ctors": ctors,
        "accessors": accessors,
        "clear": clear,
        "members": members,
    }
    impl_text = IMPL_TMPL % {
        "idl": union.idl,
        "header": path,
        "cls": union.cls,
        "includes": "".join('#include "%s"\n' % i
                            for i in sorted(impl_includes)) + "\n",
        "trace": trace,
    }
    return path, header_text, path[:-2] + ".cc", impl_text


TYPEDEFS_TMPL = """// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Generated by tools/shot/gen_idl_unions.py.
// Do not edit; edit the .idl files and re-run the generator.
//
// Upstream's bind_gen/typedef.py puts the aliases for IDL typedefs of union
// types in one header per component, rather than giving each named typedef a
// class of its own.  `typedef (double or CSSNumericValue) CSSNumberish;` is
// V8CSSNumberish, an alias for the union class the members produce.

#ifndef %(guard)s
#define %(guard)s

%(includes)s
namespace blink {

// Typedefs to IDL unions
%(aliases)s
}  // namespace blink

#endif  // %(guard)s
"""


# ---------------------------------------------------------------------------

def collect_unions(sources, names):
    """Every distinct union type expression in the .idl files, grouped by
    member token tuple the way upstream's Union.IR groups them."""
    unions = {}
    problems = []
    for path, src in sorted(sources.items()):
        toks = lex(src)
        parser = TypeParser(toks)
        for i, tok in enumerate(toks):
            if tok != "(":
                continue
            r = parser.union(i)
            if r is None:
                continue
            node, _j = r
            # [PassAsSpan] unions are IdlType.is_phantom upstream: no class is
            # generated because every variant is coerced to one C++ type.
            if any(k == "PassAsSpan" for k, _v in node.ext):
                continue
            try:
                tokens = names.member_tokens(node)
            except RecursionError as e:
                problems.append("%s: %s" % (rel(path), e))
                continue
            if len(tokens) < 2:
                continue
            if tokens in unions:
                unions[tokens].nodes.append(node)
            else:
                unions[tokens] = Union(tokens, node, rel(path), names)
    return unions, problems


def collect_typedef_aliases(typedefs, names, unions):
    """(alias class name) -> (union class name), for v8_typedefs.h.

    Two sources, both from bind_gen/typedef.py: a typedef whose type is a
    union, and a union written in terms of a typedef'ed member type, which
    gets an alias spelled with the typedef's name in place of the member's."""
    aliases = {}
    for name, (node, _idl) in sorted(typedefs.items()):
        if names.unwrap(node).kind != "union":
            continue
        tokens = names.member_tokens(names.unwrap(node))
        if tokens in unions:
            aliases["V8" + name] = unions[tokens].cls

    for tokens, union in sorted(unions.items()):
        for spelling in union.nodes:
            node = names.unwrap(spelling)
            if node.kind != "union":
                continue
            pieces = []
            used_typedef = False
            for m in node.args:
                # unwrap(nullable=True): the typedef itself is what we want to
                # name, so only the nullable wrapper comes off.
                body = m.args[0] if m.kind == "nullable" else m
                if body.kind == "named" and body.name in typedefs:
                    pieces.append(body.name)
                    used_typedef = True
                else:
                    pieces.append(names.token(body))
            if not used_typedef:
                continue
            pieces = sorted(pieces)
            if names.includes_nullable(spelling):
                pieces.append("Null")
            alias = "V8Union" + "Or".join(pieces)
            if alias != union.cls:
                aliases[alias] = union.cls
    return aliases


def main(argv):
    dry = "-n" in argv
    check = "--check" in argv
    verbose = "-v" in argv

    sources = {}
    for path in idl_files():
        sources[path] = io.open(path, encoding="utf-8", errors="replace").read()

    defs, typedefs = collect_definitions_and_typedefs(sources)
    names = Names(typedefs)
    builder = TypeInfoBuilder(defs, names)
    unions, problems = collect_unions(sources, names)

    components = dict((tokens, component_of(u, defs, names))
                      for tokens, u in unions.items())
    paths = dict(
        (tokens, "third_party/blink/renderer/bindings/%s/v8/%s.h"
         % (components[tokens], u.basename))
        for tokens, u in unions.items())

    # A union that contains another union can only be produced if the inner one
    # is, so shrink the candidate set until it stops shrinking.  Two rounds is
    # the most this tree needs, but the loop is the honest way to say it.
    candidates = set(unions)
    skipped = {}
    while True:
        builder.union_index = dict(
            (t, (unions[t].cls, paths[t])) for t in candidates)
        good = set()
        for tokens in sorted(candidates):
            union = unions[tokens]
            try:
                union.build(builder)
            except (Unsupported, RecursionError) as e:
                skipped.setdefault(union.cls, str(e))
                continue
            good.add(tokens)
        if good == candidates:
            break
        candidates = good
    for tokens in sorted(set(unions) - candidates):
        skipped.setdefault(unions[tokens].cls, "dropped with an inner union")
    for tokens in candidates:
        skipped.pop(unions[tokens].cls, None)

    produced = {}
    pending = {}
    written = 0
    by_component = {}
    for tokens in sorted(candidates):
        union = unions[tokens]
        component = components[tokens]
        by_component.setdefault(component, []).append(union)
        path, text, impl_path, impl_text = render(union, component)
        assert path == paths[tokens], (path, paths[tokens])
        if union.pending:
            # A pending member is one whose generated dictionary header does not
            # exist. If it still does not exist by now, gen_idl_dictionaries.py
            # is not going to write it -- it skips dictionaries with an `any`,
            # an `object` or a script callback -- and emitting this union would
            # produce a .cc including a header nobody will ever write. A union
            # whose member cannot be expressed without V8 cannot be either.
            missing = [h for h in union.pending_headers
                       if not os.path.exists(
                           os.path.join(ROOT, h.replace("/", os.sep)))]
            if missing:
                skipped[union.cls] = ("member dictionary %s is not generated"
                                      % ", ".join(sorted(set(union.pending))))
                for stale in (path, impl_path):
                    full = os.path.join(ROOT, stale.replace("/", os.sep))
                    if os.path.exists(full) and not dry:
                        os.remove(full)
                continue
            pending[union.cls] = sorted(set(union.pending))
        produced[union.cls] = union
        if not dry:
            for where, what in ((path, text), (impl_path, impl_text)):
                full = os.path.join(ROOT, where.replace("/", os.sep))
                os.makedirs(os.path.dirname(full), exist_ok=True)
                io.open(full, "w", encoding="utf-8",
                        newline="\n").write(what)
        written += 1
        if verbose:
            print("  %s -> %s" % (union.cls, path))

    aliases = collect_typedef_aliases(typedefs, names, produced_map(produced))
    alias_count = 0
    for component, members in sorted(by_component.items()):
        comp_aliases = {}
        cls_here = set(u.cls for u in members)
        for alias, target in sorted(aliases.items()):
            if target in cls_here:
                comp_aliases[alias] = target
        if not comp_aliases:
            continue
        path = ("third_party/blink/renderer/bindings/%s/v8/v8_typedefs.h"
                % component)
        guard = re.sub(r"[^A-Za-z0-9]", "_", path).upper() + "_"
        includes = sorted(set(
            "third_party/blink/renderer/bindings/%s/v8/%s.h"
            % (component, produced[t].basename)
            for t in comp_aliases.values()))
        text = TYPEDEFS_TMPL % {
            "guard": guard,
            "includes": "\n".join('#include "%s"' % i for i in includes) + "\n",
            "aliases": "".join("using %s = %s;\n" % (a, t)
                               for a, t in sorted(comp_aliases.items())),
        }
        alias_count += len(comp_aliases)
        if not dry:
            full = os.path.join(ROOT, path.replace("/", os.sep))
            os.makedirs(os.path.dirname(full), exist_ok=True)
            io.open(full, "w", encoding="utf-8", newline="\n").write(text)

    print("%d IDL unions, %d headers written, %d typedef aliases%s"
          % (len(unions), written, alias_count, " (dry run)" if dry else ""))
    for p in problems:
        print("  PROBLEM %s" % p)
    if skipped:
        print("%d unions skipped:" % len(skipped))
        for cls in sorted(skipped):
            print("    SKIP %s: %s" % (cls, skipped[cls]))
    if pending:
        print("%d unions reference a generated dictionary header that "
              "gen_idl_dictionaries.py has not produced:" % len(pending))
        for cls in sorted(pending):
            print("    PENDING %s: %s" % (cls, ", ".join(pending[cls])))

    if check:
        run_check(produced, aliases, skipped)


def produced_map(produced):
    """produced is keyed by class name; collect_typedef_aliases wants it keyed
    by token tuple."""
    return dict((u.tokens, u) for u in produced.values())


def git_grep(pattern, paths=None):
    out = subprocess.run(
        ["git", "grep", "-ohE", pattern, "--"] +
        (paths or ["third_party/blink/renderer/core",
                   "third_party/blink/renderer/platform"]),
        cwd=ROOT, capture_output=True, text=True).stdout
    return set(out.split())


def git_grep_files(pattern):
    out = subprocess.run(
        ["git", "grep", "-lE", pattern, "--",
         "third_party/blink/renderer/core",
         "third_party/blink/renderer/platform"],
        cwd=ROOT, capture_output=True, text=True).stdout
    return sorted(out.split())


def run_check(produced, aliases, skipped):
    known = set(produced) | set(aliases)

    used = git_grep(r"\bV8Union[A-Za-z0-9_]+")
    missing = sorted(used - known)
    print("--- check ---")
    print("%d V8Union names referenced in core+platform, %d produced, "
          "%d missing" % (len(used), len(used & known), len(missing)))
    for m in missing:
        print("    MISSING %s%s"
              % (m, (": " + skipped[m]) if m in skipped else ""))

    all_v8_names = git_grep(r"\bV8[A-Za-z0-9_]+")
    alias_used = sorted(a for a in aliases if a in all_v8_names)
    print("%d typedef aliases produced, %d of them referenced"
          % (len(aliases), len(alias_used)))

    # A typedef of a union whose declaring .idl was deleted is invisible to the
    # generator -- the identifier still appears as a type in .idl files that
    # survived, but nothing says what it means. `V8TrustedType` is the live
    # example. Ask the baseline what those typedefs were, so the report names
    # the missing declaration instead of leaving a bare undefined identifier
    # for the build to find. (One-line typedefs only; a union typedef wrapped
    # across lines would be missed, and there are none in the baseline.)
    out = subprocess.run(
        ["git", "grep", "-hE", r"^\s*typedef\s*\(", PRISTINE, "--",
         "third_party/blink/renderer/*.idl"],
        cwd=ROOT, capture_output=True, text=True).stdout
    baseline = set()
    for line in out.splitlines():
        toks = lex(line)
        if not toks or toks[0] != "typedef":
            continue
        r = TypeParser(toks).type(1)
        if r is None:
            continue
        _node, j = r
        name = TypeParser(toks).at(j)
        if name and IDENT.match(name):
            baseline.add("V8" + name)
    lost = sorted((baseline & all_v8_names) - known)
    print("%d union typedef aliases referenced whose declaring .idl is gone "
          "from this tree" % len(lost))
    for name in lost:
        print("    LOST %s" % name)

    unused = sorted(set(produced) - used)
    print("%d unions produced that no call site names" % len(unused))

    # Secondary check: the member-level API.  Is<Member>()/GetAs<Member>() and
    # ContentType::k<Member> are not qualified at the call sites, and blink has
    # plenty of unrelated classes with such methods (ObjectContentType::kNone,
    # WebMediaPlayerSource::GetAsURL()), so the search is narrowed to the files
    # that name one of our classes. Inside those, every such identifier should
    # be a member of something we produce.
    member_tokens = set()
    for u in produced.values():
        for m in u.members:
            member_tokens.add(m.token)
    ours = r"\b(V8Union[A-Za-z0-9_]+%s)\b" % "".join(
        "|" + a for a in sorted(aliases))
    files = git_grep_files(ours)
    for kind, pattern, fmt in (
            ("ContentType", r"\bContentType::k[A-Za-z0-9_]+", "ContentType::k%s"),
            ("GetAs", r"\bGetAs[A-Z][A-Za-z0-9_]*\(\)", "GetAs%s()")):
        used_ids = git_grep(pattern, files)
        have = set(fmt % t for t in member_tokens)
        unknown = sorted(used_ids - have)
        print("%s: %d referenced in the %d files that name a union, "
              "%d not a member of one" % (kind, len(used_ids), len(files),
                                          len(unknown)))
        for u in unknown:
            print("    UNKNOWN %s" % u)


if __name__ == "__main__":
    main(sys.argv[1:])
