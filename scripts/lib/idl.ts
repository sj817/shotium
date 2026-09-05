// The Web IDL reading the three binding generators share: a lexer, a type
// expression parser, the definition table, upstream's type-naming rules and
// the C++ type information a union member needs.
//
// This is deliberately a type-expression parser and not an IDL parser. Every
// union in Web IDL is written "(" T ("or" T)+ ")", so it is enough to try to
// parse a union at every "(" in a file: an argument list or an
// [Exposed=(Window,Worker)] simply fails to parse and is discarded. That
// catches unions wherever they appear without modelling the rest of the
// grammar.
//
// THE NAMING RULE is the part that has to be exactly right, because the call
// sites in renderer/core were written against upstream's generator. It was
// read out of that generator at the pristine revision:
//
//   bind_gen/blink_v8_bridge.py  blink_class_name()
//       "V8Union" + "Or".join(union.member_tokens)
//   web_idl/union.py             Union.unique_token()
//       sorted(type_name_with_extended_attribute_key_values of each member,
//              after recursively flattening nested unions and unwrapping
//              typedefs and nullables), then "Null" appended -- after the
//              sort -- if the union includes a nullable type.
//   bind_gen/path_manager.py
//       file name = ("v8_union_" + "_".join(member_tokens)).lower()
//
// and the type names themselves from web_idl/idl_type.py: DOMString -> String
// (USVString and ByteString stay), definitions as written, sequence<T> ->
// T + "Sequence", FrozenArray<T> -> T + "Array", record<K, V> -> K + V +
// "Record", T? -> T + "OrNull", a nested union -> "Or".join(sorted members),
// and a type's extended-attribute keys, sorted, appended. The tokenizer is
// Blink's own NameStyleConverter (lib/name-style.ts).

import {existsSync, readFileSync} from 'node:fs';
import path from 'node:path';

import {globSync} from 'tinyglobby';

import {NameStyleConverter} from './name-style.ts';
import {root} from './repo.ts';

export const RENDERER = path.join(root, 'third_party', 'blink', 'renderer');
export const PRISTINE = 'c0bba1026178';  // upstream baseline: the clone root, no cuts.

export const upperCamel = (name: string) => new NameStyleConverter(name).toUpperCamelCase();
export const snake = (name: string) => new NameStyleConverter(name).toSnakeCase();
export const rel = (file: string) => path.relative(root, file).replace(/\\/g, '/');

// Every .idl under renderer/, in a stable order.
export function idlFiles(): string[] {
  return globSync('**/*.idl', {cwd: RENDERER, absolute: true}).sort();
}

export function readSources(): Map<string, string> {
  const sources = new Map<string, string>();
  for (const file of idlFiles()) sources.set(file, readFileSync(file, 'utf8'));
  return sources;
}

// ---------------------------------------------------------------------------
// Lexing and type parsing

const BLOCK_COMMENT = /\/\*[\s\S]*?\*\//g;
const LINE_COMMENT = /\/\/[^\n]*/g;
const TOKEN = /"[^"]*"|[A-Za-z_][0-9A-Za-z_]*|[0-9][0-9A-Za-z_.]*|\.\.\.|\S/g;
const IDENT = /^[A-Za-z_][0-9A-Za-z_]*$/;

// SimpleType._VALID_TYPES from web_idl/idl_type.py: built-in types, never
// resolved against a definition.
const INTEGER_TYPES = ['byte', 'octet', 'short', 'unsigned short', 'long', 'unsigned long', 'long long', 'unsigned long long'];
const FLOAT_TYPES = ['float', 'unrestricted float', 'double', 'unrestricted double'];
export const NUMERIC_TYPES = [...FLOAT_TYPES, ...INTEGER_TYPES];
export const STRING_TYPES = ['DOMString', 'ByteString', 'USVString'];
const TYPED_ARRAY_TYPES = ['Int8Array', 'Int16Array', 'Int32Array', 'BigInt64Array', 'Uint8Array', 'Uint16Array', 'Uint32Array', 'BigUint64Array', 'Uint8ClampedArray', 'Float16Array', 'Float32Array', 'Float64Array'];
export const BUFFER_SOURCE_TYPES = ['ArrayBuffer', 'ArrayBufferView', 'DataView', ...TYPED_ARRAY_TYPES];
const MISC_TYPES = ['any', 'bigint', 'boolean', 'object', 'symbol', 'undefined', 'void'];
const SIMPLE_TYPES = new Set([...NUMERIC_TYPES, ...STRING_TYPES, ...BUFFER_SOURCE_TYPES, ...MISC_TYPES]);

// Words that start a declaration or modify one. An identifier used as a type
// can never be one of these, and refusing them keeps the "try to parse a
// union here" scan from mistaking e.g. `(optional X y)` for a type.
const NOT_A_TYPE_NAME = new Set([
  'or', 'optional', 'attribute', 'readonly', 'static', 'const', 'stringifier', 'getter', 'setter', 'deleter', 'inherit', 'required',
  'typedef', 'interface', 'dictionary', 'enum', 'namespace', 'callback', 'partial', 'includes', 'mixin', 'iterable', 'maplike',
  'setlike', 'async', 'constructor',
]);

const CONTAINERS: Record<string, TypeKind> = {sequence: 'sequence', FrozenArray: 'frozen_array', ObservableArray: 'observable_array', Promise: 'promise'};

export type TypeKind = 'simple' | 'named' | 'sequence' | 'frozen_array' | 'observable_array' | 'promise' | 'record' | 'union' | 'nullable';
export type ExtAttr = [string, string[]];

// A parsed IDL type expression. `args` holds the composed types (element,
// key/value, union members, nullable inner) and `ext` the extended attributes
// written on this type.
export class Type {
  kind: TypeKind;
  name: string | null;
  args: Type[];
  ext: ExtAttr[];

  constructor(kind: TypeKind, name: string | null = null, args: Type[] = [], ext: ExtAttr[] = []) {
    this.kind = kind;
    this.name = name;
    this.args = args;
    this.ext = ext;
  }
}

export function lex(src: string): string[] {
  return src.replace(BLOCK_COMMENT, ' ').replace(LINE_COMMENT, ' ').match(TOKEN) ?? [];
}

export class TypeParser {
  readonly t: string[];

  constructor(t: string[]) {
    this.t = t;
  }

  at(i: number): string | null {
    return i >= 0 && i < this.t.length ? this.t[i] : null;
  }

  // `[` ExtAttr ("," ExtAttr)* `]` -> [attrs, next index].
  extAttrs(start: number): [ExtAttr[], number] | null {
    let i = start + 1;
    const out: ExtAttr[] = [];
    for (;;) {
      const key = this.at(i);
      if (key === null || !IDENT.test(key)) return null;
      i++;
      const values: string[] = [];
      if (this.at(i) === '=') {
        i++;
        if (this.at(i) === '(') {
          i++;
          while (this.at(i) !== ')' && this.at(i) !== null) {
            if (this.at(i) !== ',') values.push(this.at(i)!);
            i++;
          }
          if (this.at(i) === null) return null;
          i++;
        } else {
          if (this.at(i) === null) return null;
          values.push(this.at(i)!);
          i++;
        }
      }
      if (this.at(i) === '(') {  // legacy [Constructor(...)]-style argument list
        let depth = 0;
        while (this.at(i) !== null) {
          if (this.at(i) === '(') depth++;
          else if (this.at(i) === ')') {
            depth--;
            if (depth === 0) {
              i++;
              break;
            }
          }
          i++;
        }
      }
      out.push([key, values]);
      if (this.at(i) === ',') {
        i++;
        continue;
      }
      if (this.at(i) === ']') return [out, i + 1];
      return null;
    }
  }

  type(start: number): [Type, number] | null {
    let i = start;
    let ext: ExtAttr[] = [];
    if (this.at(i) === '[') {
      const r = this.extAttrs(i);
      if (r === null) return null;
      [ext, i] = r;
    }
    const r = this.at(i) === '(' ? this.union(i) : this.single(i);
    if (r === null) return null;
    let [node, j] = r;
    node.ext = ext;
    // A nullable wraps the annotated type: `[X] long?` is `([X] long)?`, which
    // is why the extended attributes land on the inner node.
    if (this.at(j) === '?') {
      node = new Type('nullable', null, [node]);
      j++;
    }
    return [node, j];
  }

  union(start: number): [Type, number] | null {
    let i = start + 1;
    const members: Type[] = [];
    let r = this.type(i);
    if (r === null) return null;
    members.push(r[0]);
    i = r[1];
    if (this.at(i) !== 'or') return null;  // an argument list, not a union
    while (this.at(i) === 'or') {
      r = this.type(i + 1);
      if (r === null) return null;
      members.push(r[0]);
      i = r[1];
    }
    if (this.at(i) !== ')') return null;
    return [new Type('union', null, members), i + 1];
  }

  single(i: number): [Type, number] | null {
    const tok = this.at(i);
    if (tok === null) return null;
    if (tok in CONTAINERS) {
      if (this.at(i + 1) !== '<') return null;
      const r = this.type(i + 2);
      if (r === null) return null;
      const [inner, j] = r;
      if (this.at(j) !== '>') return null;
      return [new Type(CONTAINERS[tok], null, [inner]), j + 1];
    }
    if (tok === 'record') {
      if (this.at(i + 1) !== '<') return null;
      const k = this.type(i + 2);
      if (k === null) return null;
      const [key, j] = k;
      if (this.at(j) !== ',') return null;
      const v = this.type(j + 1);
      if (v === null) return null;
      const [value, l] = v;
      if (this.at(l) !== '>') return null;
      return [new Type('record', null, [key, value]), l + 1];
    }
    if (tok === 'unsigned') {
      const nxt = this.at(i + 1);
      if (nxt === 'short') return [new Type('simple', 'unsigned short'), i + 2];
      if (nxt === 'long') {
        if (this.at(i + 2) === 'long') return [new Type('simple', 'unsigned long long'), i + 3];
        return [new Type('simple', 'unsigned long'), i + 2];
      }
      return null;
    }
    if (tok === 'long') {
      if (this.at(i + 1) === 'long') return [new Type('simple', 'long long'), i + 2];
      return [new Type('simple', 'long'), i + 1];
    }
    if (tok === 'unrestricted') {
      const nxt = this.at(i + 1);
      if (nxt === 'float' || nxt === 'double') return [new Type('simple', `unrestricted ${nxt}`), i + 2];
      return null;
    }
    if (SIMPLE_TYPES.has(tok)) return [new Type('simple', tok), i + 1];
    if (IDENT.test(tok) && !NOT_A_TYPE_NAME.has(tok)) return [new Type('named', tok), i + 1];
    return null;
  }
}

// ---------------------------------------------------------------------------
// The IDL definition table: what each identifier names, and where it lives.

const DEF_RE = /(?:(\[[^\]]*\])\s*)?(partial\s+)?(callback\s+interface|interface\s+mixin|interface|dictionary|namespace|enum)\s+([A-Za-z_][0-9A-Za-z_]*)(?:\s*:\s*[A-Za-z_][0-9A-Za-z_]*)?\s*\{/g;
const CALLBACK_FN_RE = /\bcallback\s+([A-Za-z_][0-9A-Za-z_]*)\s*=/g;
const IMPLEMENTED_AS_RE = /\bImplementedAs\s*=\s*([A-Za-z_][0-9A-Za-z_]*)/;

export type DefKind = 'interface' | 'dictionary' | 'enum' | 'callback' | 'namespace' | 'mixin';
const DEF_KINDS: Record<string, DefKind> = {
  'interface': 'interface', 'namespace': 'namespace', 'interface mixin': 'mixin', 'callback interface': 'callback', 'dictionary': 'dictionary', 'enum': 'enum',
};

export interface Definition {
  name: string;
  kind: DefKind;
  idl: string;   // absolute path
  impl: string;  // blink implementation class name
}

export type Typedefs = Map<string, [Type, string]>;

export function collectDefinitionsAndTypedefs(sources: Map<string, string>): [Map<string, Definition>, Typedefs] {
  const defs = new Map<string, Definition>();
  const typedefs: Typedefs = new Map();
  for (const [file, src] of sources) {
    const text = src.replace(BLOCK_COMMENT, ' ').replace(LINE_COMMENT, ' ');
    for (const m of text.matchAll(DEF_RE)) {
      const kind = DEF_KINDS[m[3].replace(/\s+/g, ' ')];
      const name = m[4];
      let impl = name;
      if (kind === 'enum' || kind === 'callback') impl = 'V8' + name;
      else if (m[1]) {
        const ia = IMPLEMENTED_AS_RE.exec(m[1]);
        if (ia) impl = ia[1];
      }
      // A partial definition contributes members to a name; it does not
      // establish what that name *is*, and its [ImplementedAs] is the C++
      // class implementing those members, not the interface's own class. If
      // a name has no non-partial definition in this tree, it is genuinely
      // undefined and must be reported, not guessed.
      if (m[2]) continue;
      const existing = defs.get(name);
      if (existing && existing.kind === kind) continue;
      defs.set(name, {name, kind, idl: file, impl});
    }
    for (const m of text.matchAll(CALLBACK_FN_RE)) {
      if (!defs.has(m[1])) defs.set(m[1], {name: m[1], kind: 'callback', idl: file, impl: 'V8' + m[1]});
    }
  }
  for (const [file, src] of sources) {
    const toks = lex(src);
    const parser = new TypeParser(toks);
    for (let i = 0; i < toks.length; i++) {
      if (toks[i] !== 'typedef') continue;
      const r = parser.type(i + 1);
      if (r === null) continue;
      const [node, j] = r;
      const name = parser.at(j);
      if (name === null || !IDENT.test(name) || parser.at(j + 1) !== ';') continue;
      if (!typedefs.has(name)) typedefs.set(name, [node, file]);
    }
  }
  return [defs, typedefs];
}

// ---------------------------------------------------------------------------
// type_name, per web_idl/idl_type.py

export class Names {
  readonly typedefs: Typedefs;

  constructor(typedefs: Typedefs) {
    this.typedefs = typedefs;
  }

  // The type a `named` node refers to, if it is a typedef.
  original(t: Type): Type | null {
    if (t.kind === 'named' && t.name !== null && this.typedefs.has(t.name)) return this.typedefs.get(t.name)![0];
    return null;
  }

  // effective_annotations: own extended attributes, plus, through a typedef,
  // the ones written on the typedef'ed type.
  annotations(t: Type, depth = 0): ExtAttr[] {
    if (depth > 16) return [];
    if (t.kind === 'nullable') return this.annotations(t.args[0], depth + 1);
    const orig = this.original(t);
    if (orig !== null) return [...this.annotations(orig, depth + 1), ...t.ext];
    return [...t.ext];
  }

  // type_name_without_extended_attributes.
  bare(t: Type, depth = 0): string {
    if (depth > 32) throw new RangeError('cyclic typedef');
    switch (t.kind) {
      case 'simple': return upperCamel(t.name === 'DOMString' ? 'String' : t.name!);
      case 'named': {
        const orig = this.original(t);
        return orig !== null ? this.bare(orig, depth + 1) : t.name!;
      }
      case 'sequence': return this.full(t.args[0], depth + 1) + 'Sequence';
      case 'frozen_array': return this.full(t.args[0], depth + 1) + 'Array';
      case 'observable_array': return this.full(t.args[0], depth + 1) + 'ObservableArray';
      case 'promise': return this.full(t.args[0], depth + 1) + 'Promise';
      case 'record': return this.full(t.args[0], depth + 1) + this.full(t.args[1], depth + 1) + 'Record';
      case 'nullable': return this.bare(t.args[0], depth + 1) + 'OrNull';
      case 'union': return t.args.map((m) => this.full(m, depth + 1)).sort().join('Or');
    }
  }

  // type_name: the bare name plus the sorted annotation keys.
  full(t: Type, depth = 0): string {
    const keys = [...new Set(this.annotations(t).map(([k]) => k))].sort();
    return this.bare(t, depth) + keys.join('');
  }

  // type_name_with_extended_attribute_key_values, the spelling that actually
  // goes into a union class name.
  token(t: Type, depth = 0): string {
    const ann = this.annotations(t);
    const pieces = [this.bare(t, depth)];
    for (const key of [...new Set(ann.map(([k]) => k))].sort()) {
      pieces.push(key);
      for (const [k, values] of ann) if (k === key) pieces.push(...values);
    }
    return pieces.join('');
  }

  // IdlType.unwrap(): strip nullable and typedef wrappers.
  unwrap(t: Type, depth = 0): Type {
    while (depth < 32) {
      if (t.kind === 'nullable') {
        t = t.args[0];
      } else {
        const orig = this.original(t);
        if (orig === null) return t;
        t = orig;
      }
      depth++;
    }
    return t;
  }

  includesNullable(t: Type, depth = 0): boolean {
    if (depth > 32) return false;
    if (t.kind === 'nullable') return true;
    if (t.kind === 'union') return t.args.some((m) => this.includesNullable(m, depth + 1));
    const orig = this.original(t);
    if (orig !== null) return this.includesNullable(orig, depth + 1);
    return false;
  }

  // Union.unique_token(): the sorted member tokens, plus a trailing "Null" --
  // after the sort -- if the union includes a nullable type.
  memberTokens(union: Type): string[] {
    const pieces: string[] = [];
    const collect = (t: Type, depth = 0) => {
      if (depth > 32) throw new RangeError('cyclic union');
      const u = this.unwrap(t);
      if (u.kind === 'union') for (const m of u.args) collect(m, depth + 1);
      else pieces.push(this.token(u));
    };
    collect(union);
    const out = [...new Set(pieces)].sort();
    if (this.includesNullable(union)) out.push('Null');
    return out;
  }

  // UnionType.flattened_member_types, ordered the way upstream orders them
  // (by token), so members line up with memberTokens().
  flattened(union: Type): Type[] {
    const out = new Map<string, Type>();
    const flatten = (t: Type, depth = 0) => {
      if (depth > 32) throw new RangeError('cyclic union');
      if (t.kind === 'union') {
        for (const m of t.args) flatten(m, depth + 1);
        return;
      }
      const orig = this.original(t);
      if (orig !== null) {
        flatten(orig, depth + 1);
        return;
      }
      if (t.kind === 'nullable') {
        flatten(t.args[0], depth + 1);
        return;
      }
      const tok = this.token(t);
      if (!out.has(tok)) out.set(tok, t);
    };
    flatten(union);
    return [...out.keys()].sort().map((k) => out.get(k)!);
  }

  // IdlType.syntactic_form, for UnionNameInIDL().
  syntacticForm(t: Type, depth = 0): string {
    if (depth > 32) return '?';
    let body: string;
    switch (t.kind) {
      case 'simple': case 'named': body = t.name!; break;
      case 'sequence': body = `sequence<${this.syntacticForm(t.args[0], depth + 1)}>`; break;
      case 'frozen_array': body = `FrozenArray<${this.syntacticForm(t.args[0], depth + 1)}>`; break;
      case 'observable_array': body = `ObservableArray<${this.syntacticForm(t.args[0], depth + 1)}>`; break;
      case 'promise': body = `Promise<${this.syntacticForm(t.args[0], depth + 1)}>`; break;
      case 'record': body = `record<${this.syntacticForm(t.args[0], depth + 1)}, ${this.syntacticForm(t.args[1], depth + 1)}>`; break;
      case 'union': return `(${t.args.map((m) => this.syntacticForm(m, depth + 1)).join(' or ')})`;
      case 'nullable': return `${this.syntacticForm(t.args[0], depth + 1)}?`;
    }
    if (t.ext.length) body = `[${t.ext.map(([k]) => k).join(', ')}] ${body}`;
    return body;
  }
}

// ---------------------------------------------------------------------------
// blink_type_info, per bind_gen/blink_v8_bridge.py, minus the V8-only cases.

const NUMERIC_CXX: Record<string, string> = {
  'byte': 'int8_t', 'octet': 'uint8_t', 'short': 'int16_t', 'unsigned short': 'uint16_t', 'long': 'int32_t', 'unsigned long': 'uint32_t',
  'long long': 'int64_t', 'unsigned long long': 'uint64_t', 'float': 'float', 'unrestricted float': 'float', 'double': 'double', 'unrestricted double': 'double',
};
const WTF_STRING_H = 'third_party/blink/renderer/platform/wtf/text/wtf_string.h';
const WTF_VECTOR_H = 'third_party/blink/renderer/platform/wtf/vector.h';
const HEAP_VECTOR_H = 'third_party/blink/renderer/platform/heap/collection_support/heap_vector.h';
const MEMBER_H = 'third_party/blink/renderer/platform/heap/member.h';
const TYPED_ARRAYS = 'third_party/blink/renderer/core/typed_arrays/';
const GENERATED = 'third_party/blink/renderer/bindings/';

// The member's header has a known generated path that nothing has written yet
// (dictionaries). Carried as the path itself.
export class Pending extends Error {
  override name = 'Pending';
}

// A member type that cannot be expressed without V8, or whose header is not
// in this tree. The message is what --check and the run log print.
export class Unsupported extends Error {
  override name = 'Unsupported';
}

// The subset of upstream's TypeInfo the generated code actually uses.
export class Info {
  value_t: string;
  member_t: string;
  member_ref_t: string;
  is_gc: boolean;
  is_traceable: boolean;
  is_move_effective: boolean;
  clear: string;
  deref: string;
  init: string;
  includes: string[];
  // A blink class that is only pointed at. Its header goes in the .cc and the
  // .h forward-declares it instead, because including it from the .h is a
  // cycle: css_numeric_value.h includes v8_typedefs.h, which includes every
  // union header, one of which would include css_numeric_value.h again.
  fwd: string | null;
  impl_includes: string[];

  constructor(o: {value_t: string; member_t?: string; member_ref_t?: string; is_gc?: boolean; is_traceable?: boolean; is_move_effective?: boolean; clear?: string; deref?: string; init?: string; includes?: string[]; fwd?: string | null; impl_includes?: string[]}) {
    this.value_t = o.value_t;
    this.member_t = o.member_t ?? o.value_t;
    this.member_ref_t = o.member_ref_t ?? o.value_t;
    this.is_gc = o.is_gc ?? false;
    this.is_traceable = (o.is_traceable ?? false) || this.is_gc;
    this.is_move_effective = o.is_move_effective ?? false;
    this.clear = o.clear ?? '%s.Clear()';
    this.deref = o.deref ?? '';
    this.init = o.init ?? '';
    this.includes = [...(o.includes ?? [])];
    this.fwd = o.fwd ?? null;
    this.impl_includes = [...(o.impl_includes ?? [])];
  }
}

// gen-idl enums keeps digit runs attached (v8_rgba_float16.h, not
// v8_rgba_float_16.h) where NameStyleConverter splits them.
export function snakeAttached(name: string): string {
  return name.replace(/(?<=[a-z0-9])(?=[A-Z])/g, '_').replace(/(?<=[A-Z])(?=[A-Z][a-z])/g, '_').toLowerCase();
}

export class TypeInfoBuilder {
  // tokens (joined by "Or") -> [class name, header path] for unions that are
  // themselves producible. A union can appear inside another one, most often
  // as sequence<(A or B)>, and is then stored as Member<V8UnionAOrB>.
  unionIndex = new Map<string, [string, string]>();
  readonly defs: Map<string, Definition>;
  readonly names: Names;

  constructor(defs: Map<string, Definition>, names: Names) {
    this.defs = defs;
    this.names = names;
  }

  // Where the class implementing definition |d| is declared. Mirrors
  // PathManager: colocated with the .idl for interfaces, in the generated
  // bindings directory for enums, dictionaries and callbacks.
  headerFor(d: Definition): string {
    const idlRel = rel(d.idl);
    if (d.kind === 'interface' || d.kind === 'namespace') {
      const p = `${path.posix.dirname(idlRel)}/${snake(d.impl)}.h`;
      if (!existsSync(path.join(root, p))) throw new Unsupported(`${d.name}: no implementation header at ${p}`);
      return p;
    }
    const component = idlRel.includes('/renderer/modules/') ? 'modules' : 'core';
    const base = `third_party/blink/renderer/bindings/${component}/v8/`;
    const candidates = ['v8_' + snake(d.name)];
    const alt = 'v8_' + snakeAttached(d.name);
    if (!candidates.includes(alt)) candidates.push(alt);
    for (const cand of candidates) if (existsSync(path.join(root, base + cand + '.h'))) return base + cand + '.h';
    if (d.kind === 'dictionary') {
      // gen-idl dictionaries has not produced this one: either it has simply
      // not run yet, or it skips the dictionary because a member cannot be
      // expressed without V8. The emitted union is checked against the tree
      // afterwards; see the unions generator.
      throw new Pending(base + candidates[0] + '.h');
    }
    throw new Unsupported(`${d.name}: generated ${d.kind} header ${base + candidates[0]}.h is not in the tree`);
  }

  // A flattened union member type -> Info. Throws Unsupported.
  build(t: Type): Info {
    const extKeys = new Set(this.names.annotations(t).map(([k]) => k));
    switch (t.kind) {
      case 'simple': return this.simple(t, extKeys);
      case 'named': return this.named(t);
      case 'sequence': case 'frozen_array': return this.array(t);
      case 'record': return this.record(t);
      case 'union': return this.nestedUnion(t);
      case 'observable_array': throw new Unsupported('ObservableArray needs the V8 backing store');
      case 'promise': throw new Unsupported('Promise is a ScriptPromise, which needs V8');
      default: throw new Unsupported(`unexpected member kind ${t.kind}`);
    }
  }

  nestedUnion(t: Type): Info {
    const tokens = this.names.memberTokens(t);
    const entry = this.unionIndex.get(tokens.join('Or'));
    if (!entry) throw new Unsupported(`nested union V8Union${tokens.join('Or')} is not produced`);
    return this.gc(entry[0], entry[1]);
  }

  simple(t: Type, extKeys: Set<string>): Info {
    const name = t.name!;
    if (name === 'boolean') return new Info({value_t: 'bool', clear: '%s = false', init: ' = false'});
    if (name in NUMERIC_CXX) {
      const cxx = NUMERIC_CXX[name];
      return new Info({value_t: cxx, clear: '%s = 0', init: ' = 0', includes: cxx.endsWith('_t') ? ['<cstdint>'] : []});
    }
    if (STRING_TYPES.includes(name)) return new Info({value_t: 'String', member_ref_t: 'const String&', is_move_effective: true, clear: '%s = String()', includes: [WTF_STRING_H]});
    if (name === 'undefined' || name === 'void') {
      // Upstream stores an empty ToV8UndefinedGenerator tag here, now declared
      // in platform/bindings/union_base.h, which every union includes.
      return new Info({value_t: 'ToV8UndefinedGenerator', member_ref_t: 'const ToV8UndefinedGenerator&', clear: ''});
    }
    if (name === 'ArrayBuffer') {
      if (extKeys.has('AllowShared')) return this.gc('DOMArrayBufferBase', TYPED_ARRAYS + 'dom_array_buffer_base.h');
      return this.gc('DOMArrayBuffer', TYPED_ARRAYS + 'dom_array_buffer.h');
    }
    if (BUFFER_SOURCE_TYPES.includes(name)) {
      const wrapper = extKeys.has('AllowShared') ? 'MaybeShared' : 'NotShared';
      const impl = 'DOM' + name;
      const header = name === 'ArrayBufferView' ? TYPED_ARRAYS + 'dom_array_buffer_view.h' : name === 'DataView' ? TYPED_ARRAYS + 'dom_data_view.h' : TYPED_ARRAYS + 'dom_typed_array.h';
      if (!existsSync(path.join(root, header))) throw new Unsupported(`${name}: ${header} is not in the tree`);
      return new Info({value_t: `${wrapper}<${impl}>`, is_gc: true, includes: [TYPED_ARRAYS + 'array_buffer_view_helpers.h', header]});
    }
    if (name === 'any') throw new Unsupported('`any` is a ScriptValue, which needs V8');
    if (name === 'object') throw new Unsupported('`object` is a ScriptObject, which needs V8');
    if (name === 'bigint') throw new Unsupported('`bigint` is a BigInt, which needs V8');
    if (name === 'symbol') throw new Unsupported('blink does not accept the IDL symbol type');
    throw new Unsupported(`unhandled built-in type ${name}`);
  }

  // A bare `Member<T>` field can be declared with T incomplete, but
  // `HeapVector<Member<T>>` cannot: instantiating the vector evaluates
  // `std::derived_from<T, blink::Node>`, which needs T complete. So the
  // header is included, unless T is a *non-generated* blink class -- those
  // are the ones the .h cannot include, because the cycle that forced the
  // .h/.cc split runs impl header -> v8_typedefs.h -> union header -> impl
  // header. Another generated bindings class closes no cycle.
  gc(cxx: string, header: string): Info {
    if (header.startsWith(GENERATED)) {
      return new Info({value_t: cxx + '*', member_t: `Member<${cxx}>`, member_ref_t: cxx + '*', is_gc: true, deref: '.Get()', includes: [MEMBER_H, header]});
    }
    return new Info({value_t: cxx + '*', member_t: `Member<${cxx}>`, member_ref_t: cxx + '*', is_gc: true, deref: '.Get()', includes: [MEMBER_H], fwd: cxx, impl_includes: [header]});
  }

  named(t: Type): Info {
    const d = this.defs.get(t.name!);
    if (!d) throw new Unsupported(`${t.name}: no IDL definition in this tree`);
    if (d.kind === 'enum') {
      // IDL enumerations are value types, and the class has no default
      // constructor -- upstream initialises the member with enum value 0 and
      // relies on content_type_ to keep it from being read.
      const header = this.headerFor(d);
      return new Info({value_t: d.impl, member_ref_t: d.impl, clear: '', init: `{static_cast<${d.impl}::Enum>(0)}`, includes: [header]});
    }
    if (d.kind === 'interface' || d.kind === 'dictionary' || d.kind === 'namespace' || d.kind === 'callback') {
      try {
        return this.gc(d.impl, this.headerFor(d));
      } catch (e) {
        if (e instanceof Pending) {
          const info = this.gc(d.impl, e.message);
          info.includes.push('!pending');
          return info;
        }
        throw e;
      }
    }
    if (d.kind === 'mixin') throw new Unsupported(`${t.name}: interface mixins are not types`);
    throw new Unsupported(`${t.name}: unsupported definition kind ${d.kind}`);
  }

  array(t: Type): Info {
    const elem = this.build(this.names.unwrap(t.args[0]));
    if (this.names.includesNullable(t.args[0]) && !elem.is_gc) throw new Unsupported('nullable non-pointer sequence element');
    const cxx = elem.is_traceable ? `HeapVector<${elem.member_t}>` : `Vector<${elem.value_t}>`;
    const includes = [elem.is_traceable ? HEAP_VECTOR_H : WTF_VECTOR_H, ...elem.includes];
    return new Info({value_t: cxx, member_ref_t: `const ${cxx}&`, is_move_effective: true, is_traceable: elem.is_traceable, clear: '%s.clear()', includes, fwd: elem.fwd, impl_includes: elem.impl_includes});
  }

  record(t: Type): Info {
    const key = this.build(this.names.unwrap(t.args[0]));
    if (key.value_t !== 'String') throw new Unsupported('record key is not a string type');
    const value = this.build(this.names.unwrap(t.args[1]));
    const cxx = value.is_traceable ? `HeapVector<std::pair<String, ${value.member_t}>>` : `Vector<std::pair<String, ${value.value_t}>>`;
    const includes = [value.is_traceable ? HEAP_VECTOR_H : WTF_VECTOR_H, WTF_STRING_H, '<utility>', ...value.includes];
    return new Info({value_t: cxx, member_ref_t: `const ${cxx}&`, is_move_effective: true, is_traceable: value.is_traceable, clear: '%s.clear()', includes, fwd: value.fwd, impl_includes: value.impl_includes});
  }
}

// ---------------------------------------------------------------------------
// Unions as collected from the tree

export class Member {
  token: string;
  info: Info | null;
  is_null: boolean;
  var: string;
  pred: string;
  get: string;
  content: string;

  constructor(token: string, info: Info | null) {
    this.token = token;
    this.info = info;
    this.is_null = info === null;
    this.var = new NameStyleConverter('member ' + token).toSnakeCase() + '_';
    this.pred = 'Is' + token;
    this.get = 'GetAs' + token;
    this.content = 'ContentType::k' + token;
  }
}

export class Union {
  tokens: string[];
  node: Type;
  // Every spelling that maps to this class. (A or B), (A? or B) and (T or B)
  // where `typedef A T` are one class but three spellings, and
  // bind_gen/typedef.py emits a `using` alias for each one written with a
  // typedef'ed member.
  nodes: Type[];
  idl: string;
  cls: string;
  basename: string;
  members: Member[] = [];
  pending: string[] = [];
  pendingHeaders: string[] = [];
  names: Names;

  constructor(tokens: string[], node: Type, idl: string, names: Names) {
    this.tokens = tokens;
    this.node = node;
    this.nodes = [node];
    this.idl = idl;
    this.cls = 'V8Union' + tokens.join('Or');
    this.basename = ('v8_union_' + tokens.join('_')).toLowerCase();
    this.names = names;
  }

  build(builder: TypeInfoBuilder): void {
    this.members = [];
    this.pending = [];
    this.pendingHeaders = [];
    const flat = this.names.flattened(this.node);
    const flatTokens = flat.map((t) => this.names.token(t));
    const expected = this.tokens.filter((t) => t !== 'Null');
    if (flatTokens.join('\0') !== expected.join('\0')) throw new Unsupported(`member/token mismatch [${flatTokens.map((s) => `'${s}'`).join(', ')}] vs [${expected.map((s) => `'${s}'`).join(', ')}]`);
    for (const [i, t] of flat.entries()) {
      const info = builder.build(t);
      if (info.includes.includes('!pending')) {
        info.includes = info.includes.filter((x) => x !== '!pending');
        this.pending.push(flatTokens[i]);
        this.pendingHeaders.push(info.impl_includes.length ? info.impl_includes[0] : info.includes[0]);
      }
      this.members.push(new Member(flatTokens[i], info));
    }
    if (this.tokens.includes('Null')) this.members.push(new Member('Null', null));
  }
}

// Every distinct union type expression in the .idl files, grouped by member
// token tuple the way upstream's Union.IR groups them.
export function collectUnions(sources: Map<string, string>, names: Names): [Map<string, Union>, string[]] {
  const unions = new Map<string, Union>();
  const problems: string[] = [];
  for (const file of [...sources.keys()].sort()) {
    const toks = lex(sources.get(file)!);
    const parser = new TypeParser(toks);
    for (let i = 0; i < toks.length; i++) {
      if (toks[i] !== '(') continue;
      const r = parser.union(i);
      if (r === null) continue;
      const node = r[0];
      // [PassAsSpan] unions are IdlType.is_phantom upstream: no class is
      // generated because every variant is coerced to one C++ type.
      if (node.ext.some(([k]) => k === 'PassAsSpan')) continue;
      let tokens: string[];
      try {
        tokens = names.memberTokens(node);
      } catch (e) {
        problems.push(`${rel(file)}: ${e instanceof Error ? e.message : String(e)}`);
        continue;
      }
      if (tokens.length < 2) continue;
      const key = tokens.join('Or');
      const existing = unions.get(key);
      if (existing) existing.nodes.push(node);
      else unions.set(key, new Union(tokens, node, rel(file), names));
    }
  }
  return [unions, problems];
}

// Python sorted the unions by their token tuples, element by element; a
// joined string sorts differently when one token is a prefix of another.
export function compareTokens(a: string[], b: string[]): number {
  for (let i = 0; i < Math.min(a.length, b.length); i++) {
    if (a[i] < b[i]) return -1;
    if (a[i] > b[i]) return 1;
  }
  return a.length - b.length;
}

// core unless some member type is defined under renderer/modules.
export function componentOf(union: Union, defs: Map<string, Definition>, names: Names): 'core' | 'modules' {
  let found: 'core' | 'modules' = 'core';
  const walk = (t: Type, depth = 0) => {
    if (depth > 32) return;
    if (t.kind === 'named') {
      const orig = names.original(t);
      if (orig !== null) walk(orig, depth + 1);
      const d = defs.get(t.name!);
      if (d && rel(d.idl).includes('/renderer/modules/')) found = 'modules';
    }
    for (const a of t.args) walk(a, depth + 1);
  };
  walk(union.node);
  return found;
}
