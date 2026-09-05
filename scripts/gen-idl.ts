// Generate blink's IDL enum, union and dictionary classes from the .idl files,
// without V8.
//
// blink core does not use plain C++ enums, unions or option bags for its IDL
// types. It uses the classes the bindings generator emits --
// V8AnimationPlayState, V8UnionCSSNumericValueOrDouble, FocusOptions -- as its
// own type vocabulary, all through core, css, animation and layout. The
// generator lived in third_party/blink/renderer/bindings/scripts and is gone
// with V8. The .idl files it read are not, and they are the actual source of
// truth, so the classes are regenerated from the same input, minus the parts
// that only exist to talk to a JavaScript engine: Create() from a JS value,
// ToV8(), the NativeValueTraits / ToV8Traits specialisations.
//
// The output is written into the source tree rather than into gen/. It is
// derived from checked-in .idl files by a checked-in script, so it is
// reproducible either way, and putting it in the tree keeps it out of the GN
// action graph -- the target that would run it went away with V8.
//
//   pnpm gen:idl all [-n]                  after any .idl change: the three
//                                          generators, then the prune and
//                                          build.gni rewrite
//   pnpm gen:idl enums [-n] [--check]
//   pnpm gen:idl unions [-n] [-v] [--check]
//   pnpm gen:idl dictionaries [-n] [--only Name,Name]
//   pnpm gen:idl prune [-n]                generated files whose .idl is gone
//   pnpm gen:idl build-gni                 the GN source list for the .cc files
//
// --check is the real test: every V8Foo::Enum::kBar, V8Union... and
// GetAs...() named in renderer/core and renderer/platform must be produced.
// The call sites were written against upstream's generator, so agreeing with
// them is agreeing with it. Inferred naming rules have produced code that
// looked right and broke hundreds of files; run it.

import {existsSync, mkdirSync, openSync, readdirSync, readFileSync, readSync, closeSync, rmSync, statSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execaSync} from 'execa';

import {
  BUFFER_SOURCE_TYPES, collectDefinitionsAndTypedefs, collectUnions, compareTokens, componentOf, idlFiles, Info, lex, Names, NUMERIC_TYPES, PRISTINE,
  readSources, rel, RENDERER, snake, snakeAttached, STRING_TYPES, type Definition, type Type, TypeInfoBuilder, TypeParser, type Typedefs,
  Union, Unsupported,
} from './lib/idl.ts';
import {root} from './lib/repo.ts';

const BINDINGS = path.join(RENDERER, 'bindings');
// What the banner in every generated file names as its producer. The prune
// and the `all` sweep recognise the banner by this prefix.
// What the banner in every generated file names. The subcommand after the
// script is one token so the prune regexes can capture it.
const GENERATOR = {enums: 'scripts/gen-idl.ts enums', unions: 'scripts/gen-idl.ts unions', dictionaries: 'scripts/gen-idl.ts dictionaries', buildGni: 'scripts/gen-idl.ts build-gni'};
const BANNER_PREFIX = 'scripts/gen-idl.ts ';

const write = (file: string, text: string) => {
  mkdirSync(path.dirname(file), {recursive: true});
  writeFileSync(file, text);
};
const head = (file: string, bytes: number): string => {
  const fd = openSync(file, 'r');
  const buf = Buffer.alloc(bytes);
  const n = readSync(fd, buf, 0, bytes, 0);
  closeSync(fd);
  return buf.subarray(0, n).toString('utf8');
};
function gitGrep(pattern: string, paths: string[] = ['third_party/blink/renderer/core', 'third_party/blink/renderer/platform']): Set<string> {
  const out = execaSync('git', ['grep', '-ohE', pattern, '--', ...paths], {cwd: root, reject: false, maxBuffer: 1 << 28}).stdout;
  return new Set(out.split(/\s+/).filter(Boolean));
}

// ===========================================================================
// enums

// `enum Foo { "a", "b" };` -- IDL allows newlines and comments inside the body.
const ENUM = /^\s*enum\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{([\s\S]*?)\}\s*;/gm;
const VALUE = /"([^"]*)"/g;
const LINE_COMMENT = /\/\/[^\n]*/g;

// Tokens the bindings generator's name_style_converter keeps fully upper-case
// instead of title-casing. Only the ones that actually occur in the surviving
// .idl files are listed; --check is what proves the list is complete. The
// call sites say kRgbaFloat16, kRec2100Hlg and kSharedStorageSelectUrl, so
// rgba, hlg and url are title-cased like any other word; srgb is the one
// token that is not. Add to it only when a real call site disagrees.
const ACRONYMS: Record<string, string> = {srgb: 'SRGB'};

// IDL enum string -> C++ enumerator name, matching upstream's spelling. An
// empty value is a bare "k": upstream's own vtt_region.h reads
// `V8ScrollSetting::Enum::k`, and there is not one use of "Enum::kEmptyString"
// in the tree.
function enumEntry(value: string): string {
  if (value === '') return 'k';
  const parts = value.split(/[^A-Za-z0-9]+/).filter(Boolean);
  return 'k' + parts.map((p) => ACRONYMS[p.toLowerCase()] ?? (/^[A-Z]{2}/.test(p) ? p : p[0].toUpperCase() + p.slice(1))).join('');
}

const enumHeader = (o: {idl: string; guard: string; name: string; cls: string; entries: string; strings: string; count: number}) => `// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Generated by ${GENERATOR.enums} from ${o.idl}.
// Do not edit; edit the .idl and re-run the generator.

#ifndef ${o.guard}
#define ${o.guard}

#include <array>

#include "third_party/blink/renderer/platform/bindings/enumeration_base.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

// IDL enumeration ${o.name}.
class ${o.cls} final : public bindings::EnumerationBase {
 public:
  enum class Enum : enum_int_t {
${o.entries}  };

  // std::array rather than a C array: indexing a raw array is
  // -Wunsafe-buffer-usage, which this tree builds with -Werror, and the index
  // below is a hard error in 590 translation units otherwise. std::array's
  // operator[] is a member function, so it is not caught by that warning and
  // is bounds-checked under libc++ hardening.
  static constexpr std::array<const char*, ${o.count}> kEnumStrings = {
${o.strings}  };

  // Not explicit: upstream's generated class is implicitly constructible from
  // its Enum, and call sites rely on that (\`return V8FillMode::Enum::kBoth;\`).
  constexpr ${o.cls}(Enum value)
      : bindings::EnumerationBase(static_cast<enum_int_t>(value),
                                  kEnumStrings[static_cast<enum_int_t>(value)]) {}

  constexpr ${o.cls}(const ${o.cls}&) = default;
  constexpr ${o.cls}(${o.cls}&&) = default;
  ${o.cls}& operator=(const ${o.cls}&) = default;
  ${o.cls}& operator=(${o.cls}&&) = default;
  ~${o.cls}() = default;

  Enum AsEnum() const { return static_cast<Enum>(GetEnumValue()); }

 private:
  // Only the generated Create() produced an empty instance, to represent
  // "an exception was thrown while converting". Nothing here throws.
  constexpr ${o.cls}() = default;
};

}  // namespace blink

#endif  // ${o.guard}
`;

// (enum name) -> [idl path, values]. Later definitions must agree.
function collectEnums(): Map<string, [string, string[]]> {
  const found = new Map<string, [string, string[]]>();
  for (const file of idlFiles()) {
    const src = readFileSync(file, 'utf8').replace(LINE_COMMENT, '');
    for (const m of src.matchAll(ENUM)) {
      const values = [...m[2].matchAll(VALUE)].map((v) => v[1]);
      if (!values.length) continue;
      const r = rel(file);
      const existing = found.get(m[1]);
      if (existing && existing[1].join('\0') !== values.join('\0')) {
        console.log(`  CONFLICT ${m[1]}: ${existing[0]} vs ${r}`);
        continue;
      }
      if (!existing) found.set(m[1], [r, values]);
    }
  }
  return found;
}

const outDirFor = (idlRel: string) => path.join(RENDERER, 'bindings', idlRel.includes('/renderer/modules/') ? 'modules' : 'core', 'v8');

function enums(dry: boolean, check: boolean): void {
  const found = collectEnums();
  const produced = new Set<string>();
  let written = 0;
  for (const name of [...found.keys()].sort()) {
    const [idlRel, values] = found.get(name)!;
    const cls = 'V8' + name;
    const entries = values.map(enumEntry);
    if (new Set(entries).size !== entries.length) {
      console.log(`  SKIP ${cls}: enumerator collision [${entries.map((e) => `'${e}'`).join(', ')}]`);
      continue;
    }
    for (const e of entries) produced.add(`${cls}::Enum::${e}`);
    const outDir = outDirFor(idlRel);
    const fname = `v8_${snakeAttached(name)}.h`;
    const guard = `THIRD_PARTY_BLINK_RENDERER_BINDINGS_${path.basename(path.dirname(outDir)).toUpperCase()}_V8_V8_${snakeAttached(name).toUpperCase()}_H_`;
    const text = enumHeader({idl: idlRel, guard, name, cls, entries: entries.map((e) => `    ${e},\n`).join(''), strings: values.map((v) => `      "${v}",\n`).join(''), count: values.length});
    if (!dry) write(path.join(outDir, fname), text);
    written++;
  }
  console.log(`${found.size} IDL enums, ${written} headers written${dry ? ' (dry run)' : ''}`);
  if (check) {
    const used = gitGrep(String.raw`\bV8[A-Za-z0-9_]+::Enum::k[A-Za-z0-9_]*`);
    const missing = [...used].filter((u) => !produced.has(u)).sort();
    console.log(`${used.size} distinct enum values referenced, ${missing.length} not produced`);
    for (const m of missing) console.log(`    MISSING ${m}`);
  }
}

// ===========================================================================
// unions

const unionHeader = (o: {idl: string; guard: string; cls: string; syntax: string; idlName: string; includes: string; forwards: string; contentTypes: string; ctors: string; accessors: string; clear: string; members: string}) => `// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Generated by ${GENERATOR.unions} from ${o.idl}.
// Do not edit; edit the .idl and re-run the generator.

#ifndef ${o.guard}
#define ${o.guard}

${o.includes}
namespace blink {

${o.forwards}// IDL union ${o.syntax}.
class ${o.cls} final : public bindings::UnionBase {
 public:
  // The type of the content value of this IDL union.
  enum class ContentType {
${o.contentTypes}  };

${o.ctors}
  // Returns the type of the content value.
  ContentType GetContentType() const { return content_type_; }

${o.accessors}  // Defined in the .cc: tracing a Member<T> needs T complete, and T's
  // header cannot be included here without a cycle.
  void Trace(Visitor* visitor) const override;

  static constexpr const char* UnionNameInIDL() { return "${o.idlName}"; }

 private:
  void Clear() {
${o.clear}  }

  ContentType content_type_;

${o.members}};

}  // namespace blink

#endif  // ${o.guard}
`;

const unionImpl = (o: {idl: string; header: string; cls: string; includes: string; trace: string}) => `// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Generated by ${GENERATOR.unions} from ${o.idl}.
// Do not edit; edit the .idl and re-run the generator.

#include "${o.header}"

${o.includes}
namespace blink {

void ${o.cls}::Trace(Visitor* visitor) const {
${o.trace}}

}  // namespace blink
`;

const typedefsHeader = (o: {guard: string; includes: string; aliases: string}) => `// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Generated by ${GENERATOR.unions}.
// Do not edit; edit the .idl files and re-run the generator.
//
// Upstream's bind_gen/typedef.py puts the aliases for IDL typedefs of union
// types in one header per component, rather than giving each named typedef a
// class of its own.  \`typedef (double or CSSNumericValue) CSSNumberish;\` is
// V8CSSNumberish, an alias for the union class the members produce.

#ifndef ${o.guard}
#define ${o.guard}

${o.includes}
namespace blink {

// Typedefs to IDL unions
${o.aliases}
}  // namespace blink

#endif  // ${o.guard}
`;

const guardOf = (p: string) => p.replace(/[^A-Za-z0-9]/g, '_').toUpperCase() + '_';

function renderUnion(union: Union, component: string): [string, string, string, string] {
  const includes = new Set(['third_party/blink/renderer/platform/bindings/union_base.h', 'third_party/blink/renderer/platform/heap/garbage_collected.h']);
  const system = new Set<string>();
  const implIncludes = new Set<string>();
  const forwards = new Set<string>();
  for (const m of union.members) {
    if (m.is_null) {
      system.add('<cstddef>');
      continue;
    }
    for (const inc of m.info!.includes) (inc.startsWith('<') ? system : includes).add(inc);
    for (const inc of m.info!.impl_includes) implIncludes.add(inc);
    if (m.info!.fwd) forwards.add(m.info!.fwd);
    if (m.info!.is_move_effective) system.add('<utility>');
  }
  const incLines: string[] = [];
  if (system.size) {
    incLines.push(...[...system].sort().map((s) => `#include ${s}`), '');
  }
  incLines.push('#include "base/check_op.h"', ...[...includes].sort().map((i) => `#include "${i}"`));

  const contentTypes = union.members.map((m) => `    k${m.token},\n`).join('');
  const ctors: string[] = [];
  for (const m of union.members) {
    if (m.is_null) {
      ctors.push(`  explicit ${union.cls}(std::nullptr_t)\n      : content_type_(${m.content}) {}\n`);
      continue;
    }
    const check = m.info!.is_gc ? '\n    DCHECK(value);\n  ' : '';
    ctors.push(`  explicit ${union.cls}(${m.info!.member_ref_t} value)\n      : content_type_(${m.content}), ${m.var}(value) {${check}}\n`);
    if (m.info!.is_move_effective) ctors.push(`  explicit ${union.cls}(${m.info!.value_t}&& value)\n      : content_type_(${m.content}), ${m.var}(std::move(value)) {}\n`);
  }
  const acc: string[] = [];
  for (const m of union.members) {
    const block = [`  bool ${m.pred}() const { return content_type_ == ${m.content}; }\n`];
    if (m.is_null) {
      block.push(`  void Set(std::nullptr_t) {\n    Clear();\n    content_type_ = ${m.content};\n  }\n`);
    } else {
      block.push(`  ${m.info!.member_ref_t} ${m.get}() const {\n    DCHECK_EQ(content_type_, ${m.content});\n    return ${m.var}${m.info!.deref};\n  }\n`);
      const check = m.info!.is_gc ? '    DCHECK(value);\n' : '';
      block.push(`  void Set(${m.info!.member_ref_t} value) {\n${check}    Clear();\n    ${m.var} = value;\n    content_type_ = ${m.content};\n  }\n`);
      if (m.info!.is_move_effective) block.push(`  void Set(${m.info!.value_t}&& value) {\n    Clear();\n    ${m.var} = std::move(value);\n    content_type_ = ${m.content};\n  }\n`);
    }
    acc.push(block.join(''));
  }
  const accessors = acc.join('\n') + '\n';
  let trace = union.members.filter((m) => !m.is_null && m.info!.is_traceable).map((m) => `    visitor->Trace(${m.var});\n`).join('');
  trace += '    bindings::UnionBase::Trace(visitor);\n';
  let clear = union.members.filter((m) => !m.is_null && m.info!.clear).map((m) => `    ${m.info!.clear.replace('%s', m.var)};\n`).join('');
  if (!clear) clear = '    // No member holds a resource that needs releasing.\n';
  const members = union.members.filter((m) => !m.is_null).map((m) => `  ${m.info!.member_t} ${m.var}${m.info!.init};\n`).join('');
  const p = `third_party/blink/renderer/bindings/${component}/v8/${union.basename}.h`;
  const syntax = union.names.syntacticForm(union.node);
  const idlName = `(${union.names.flattened(union.node).map((t) => union.names.syntacticForm(t)).sort().join(' or ')})${union.names.includesNullable(union.node) ? '?' : ''}`;
  let fwdLines = [...forwards].sort().map((f) => `class ${f};\n`).join('');
  if (fwdLines) fwdLines += '\n';
  const headerText = unionHeader({
    idl: union.idl, guard: guardOf(p), cls: union.cls, syntax, idlName, includes: incLines.join('\n') + '\n', forwards: fwdLines,
    contentTypes, ctors: ctors.join('\n'), accessors, clear, members,
  });
  const implText = unionImpl({idl: union.idl, header: p, cls: union.cls, includes: [...implIncludes].sort().map((i) => `#include "${i}"\n`).join('') + '\n', trace});
  return [p, headerText, p.slice(0, -2) + '.cc', implText];
}

// (alias class name) -> (union class name), for v8_typedefs.h. Two sources,
// both from bind_gen/typedef.py: a typedef whose type is a union, and a union
// written in terms of a typedef'ed member type, which gets an alias spelled
// with the typedef's name in place of the member's.
function collectTypedefAliases(typedefs: Typedefs, names: Names, unions: Map<string, Union>): Map<string, string> {
  const aliases = new Map<string, string>();
  for (const name of [...typedefs.keys()].sort()) {
    const node = typedefs.get(name)![0];
    if (names.unwrap(node).kind !== 'union') continue;
    const key = names.memberTokens(names.unwrap(node)).join('Or');
    if (unions.has(key)) aliases.set('V8' + name, unions.get(key)!.cls);
  }
  for (const key of [...unions.keys()].sort((a, b) => compareTokens(unions.get(a)!.tokens, unions.get(b)!.tokens))) {
    const union = unions.get(key)!;
    for (const spelling of union.nodes) {
      const node = names.unwrap(spelling);
      if (node.kind !== 'union') continue;
      const pieces: string[] = [];
      let usedTypedef = false;
      for (const m of node.args) {
        // Only the nullable wrapper comes off: the typedef itself is what we
        // want to name.
        const body = m.kind === 'nullable' ? m.args[0] : m;
        if (body.kind === 'named' && typedefs.has(body.name!)) {
          pieces.push(body.name!);
          usedTypedef = true;
        } else {
          pieces.push(names.token(body));
        }
      }
      if (!usedTypedef) continue;
      pieces.sort();
      if (names.includesNullable(spelling)) pieces.push('Null');
      const alias = 'V8Union' + pieces.join('Or');
      if (alias !== union.cls) aliases.set(alias, union.cls);
    }
  }
  return aliases;
}

function unions(dry: boolean, verbose: boolean, check: boolean): void {
  const sources = readSources();
  const [defs, typedefs] = collectDefinitionsAndTypedefs(sources);
  const names = new Names(typedefs);
  const builder = new TypeInfoBuilder(defs, names);
  const [all, problems] = collectUnions(sources, names);
  const components = new Map<string, string>();
  const paths = new Map<string, string>();
  for (const [key, u] of all) {
    components.set(key, componentOf(u, defs, names));
    paths.set(key, `third_party/blink/renderer/bindings/${components.get(key)}/v8/${u.basename}.h`);
  }
  // A union that contains another union can only be produced if the inner
  // one is, so shrink the candidate set until it stops shrinking.
  const byTokens = (a: string, b: string) => compareTokens(all.get(a)!.tokens, all.get(b)!.tokens);
  let candidates = new Set(all.keys());
  const skipped = new Map<string, string>();
  for (;;) {
    builder.unionIndex = new Map([...candidates].map((k) => [k, [all.get(k)!.cls, paths.get(k)!]]));
    const good = new Set<string>();
    for (const key of [...candidates].sort(byTokens)) {
      const union = all.get(key)!;
      try {
        union.build(builder);
      } catch (e) {
        if (e instanceof Unsupported || e instanceof RangeError) {
          if (!skipped.has(union.cls)) skipped.set(union.cls, e.message);
          continue;
        }
        throw e;
      }
      good.add(key);
    }
    if (good.size === candidates.size) break;
    candidates = good;
  }
  for (const key of [...all.keys()].filter((k) => !candidates.has(k)).sort(byTokens)) {
    if (!skipped.has(all.get(key)!.cls)) skipped.set(all.get(key)!.cls, 'dropped with an inner union');
  }
  for (const key of candidates) skipped.delete(all.get(key)!.cls);

  const produced = new Map<string, Union>();
  const pending = new Map<string, string[]>();
  let written = 0;
  const byComponent = new Map<string, Union[]>();
  for (const key of [...candidates].sort(byTokens)) {
    const union = all.get(key)!;
    const component = components.get(key)!;
    byComponent.set(component, [...(byComponent.get(component) ?? []), union]);
    const [p, text, implPath, implText] = renderUnion(union, component);
    if (p !== paths.get(key)) throw new Error(`${p} vs ${paths.get(key)}`);
    if (union.pending.length) {
      // A pending member is one whose generated dictionary header does not
      // exist. If it still does not exist by now, the dictionaries generator
      // is not going to write it, and emitting this union would produce a .cc
      // including a header nobody will ever write.
      const missing = union.pendingHeaders.filter((h) => !existsSync(path.join(root, h)));
      if (missing.length) {
        skipped.set(union.cls, `member dictionary ${[...new Set(union.pending)].sort().join(', ')} is not generated`);
        for (const stale of [p, implPath]) {
          const full = path.join(root, stale);
          if (existsSync(full) && !dry) rmSync(full);
        }
        continue;
      }
      pending.set(union.cls, [...new Set(union.pending)].sort());
    }
    produced.set(union.cls, union);
    if (!dry) {
      write(path.join(root, p), text);
      write(path.join(root, implPath), implText);
    }
    written++;
    if (verbose) console.log(`  ${union.cls} -> ${p}`);
  }

  const producedByTokens = new Map([...produced.values()].map((u) => [u.tokens.join('Or'), u]));
  const aliases = collectTypedefAliases(typedefs, names, producedByTokens);
  let aliasCount = 0;
  for (const component of [...byComponent.keys()].sort()) {
    const clsHere = new Set(byComponent.get(component)!.map((u) => u.cls));
    const compAliases = new Map<string, string>();
    for (const [alias, target] of [...aliases.entries()].sort()) if (clsHere.has(target)) compAliases.set(alias, target);
    if (!compAliases.size) continue;
    const p = `third_party/blink/renderer/bindings/${component}/v8/v8_typedefs.h`;
    const includes = [...new Set([...compAliases.values()].map((t) => `third_party/blink/renderer/bindings/${component}/v8/${produced.get(t)!.basename}.h`))].sort();
    const text = typedefsHeader({
      guard: guardOf(p),
      includes: includes.map((i) => `#include "${i}"`).join('\n') + '\n',
      aliases: [...compAliases.entries()].sort().map(([a, t]) => `using ${a} = ${t};\n`).join(''),
    });
    aliasCount += compAliases.size;
    if (!dry) write(path.join(root, p), text);
  }

  console.log(`${all.size} IDL unions, ${written} headers written, ${aliasCount} typedef aliases${dry ? ' (dry run)' : ''}`);
  for (const p of problems) console.log(`  PROBLEM ${p}`);
  if (skipped.size) {
    console.log(`${skipped.size} unions skipped:`);
    for (const cls of [...skipped.keys()].sort()) console.log(`    SKIP ${cls}: ${skipped.get(cls)}`);
  }
  if (pending.size) {
    console.log(`${pending.size} unions reference a generated dictionary header that ${GENERATOR.dictionaries} has not produced:`);
    for (const cls of [...pending.keys()].sort()) console.log(`    PENDING ${cls}: ${pending.get(cls)!.join(', ')}`);
  }
  if (check) checkUnions(produced, aliases, skipped);
}

function checkUnions(produced: Map<string, Union>, aliases: Map<string, string>, skipped: Map<string, string>): void {
  const known = new Set([...produced.keys(), ...aliases.keys()]);
  const used = gitGrep(String.raw`\bV8Union[A-Za-z0-9_]+`);
  const missing = [...used].filter((u) => !known.has(u)).sort();
  console.log('--- check ---');
  console.log(`${used.size} V8Union names referenced in core+platform, ${[...used].filter((u) => known.has(u)).length} produced, ${missing.length} missing`);
  for (const m of missing) console.log(`    MISSING ${m}${skipped.has(m) ? ': ' + skipped.get(m) : ''}`);

  const allV8Names = gitGrep(String.raw`\bV8[A-Za-z0-9_]+`);
  const aliasUsed = [...aliases.keys()].filter((a) => allV8Names.has(a)).sort();
  console.log(`${aliases.size} typedef aliases produced, ${aliasUsed.length} of them referenced`);

  // A typedef of a union whose declaring .idl was deleted is invisible to the
  // generator -- the identifier still appears as a type in .idl files that
  // survived, but nothing says what it means. Ask the baseline what those
  // typedefs were, so the report names the missing declaration.
  const out = execaSync('git', ['grep', '-hE', String.raw`^\s*typedef\s*\(`, PRISTINE, '--', 'third_party/blink/renderer/*.idl'], {cwd: root, reject: false, maxBuffer: 1 << 28}).stdout;
  const baseline = new Set<string>();
  for (const line of out.split(/\r?\n/)) {
    const toks = lex(line);
    if (!toks.length || toks[0] !== 'typedef') continue;
    const parser = new TypeParser(toks);
    const r = parser.type(1);
    if (r === null) continue;
    const name = parser.at(r[1]);
    if (name && /^[A-Za-z_][0-9A-Za-z_]*$/.test(name)) baseline.add('V8' + name);
  }
  const lost = [...baseline].filter((n) => allV8Names.has(n) && !known.has(n)).sort();
  console.log(`${lost.length} union typedef aliases referenced whose declaring .idl is gone from this tree`);
  for (const name of lost) console.log(`    LOST ${name}`);

  const unused = [...produced.keys()].filter((p) => !used.has(p));
  console.log(`${unused.length} unions produced that no call site names`);

  // Secondary check: the member-level API, narrowed to the files that name
  // one of our classes, because blink has plenty of unrelated
  // ContentType::k... and GetAs...() members.
  const memberTokens = new Set<string>();
  for (const u of produced.values()) for (const m of u.members) memberTokens.add(m.token);
  const ours = String.raw`\b(V8Union[A-Za-z0-9_]+${[...aliases.keys()].sort().map((a) => '|' + a).join('')})\b`;
  const files = execaSync('git', ['grep', '-lE', ours, '--', 'third_party/blink/renderer/core', 'third_party/blink/renderer/platform'], {cwd: root, reject: false, maxBuffer: 1 << 28}).stdout.split(/\s+/).filter(Boolean).sort();
  for (const [kind, pattern, fmt] of [
    ['ContentType', String.raw`\bContentType::k[A-Za-z0-9_]+`, (t: string) => `ContentType::k${t}`],
    ['GetAs', String.raw`\bGetAs[A-Z][A-Za-z0-9_]*\(\)`, (t: string) => `GetAs${t}()`],
  ] as Array<[string, string, (t: string) => string]>) {
    const usedIds = gitGrep(pattern, files);
    const have = new Set([...memberTokens].map(fmt));
    const unknown = [...usedIds].filter((u) => !have.has(u)).sort();
    console.log(`${kind}: ${usedIds.size} referenced in the ${files.length} files that name a union, ${unknown.length} not a member of one`);
    for (const u of unknown) console.log(`    UNKNOWN ${u}`);
  }
}

// ===========================================================================
// dictionaries

// Matches a double-quoted string OR a // comment, so the substitution can
// keep the string and drop only the comment.
const COMMENT_OR_STRING = /("(?:[^"\\]|\\.)*")|\/\/[^\n]*/g;
const EXT_ATTR = /\[[^\]]*\]/g;
// The *head* of a dictionary; the body is taken by counting braces, because
// a member's default value can contain braces (`sanitizer = {};`).
const DICT = /(?:^|\]\s*)dictionary\s+([A-Za-z_]\w*)\s*(?::\s*([A-Za-z_]\w*)\s*)?\{/gm;
const IFACE = /(?:^|\]\s*)(?:partial\s+)?interface\s+(?:mixin\s+)?([A-Za-z_]\w*)/gm;
const DENUM = /^\s*enum\s+([A-Za-z_]\w*)\s*\{/gm;

const stripComments = (text: string) => text.replace(COMMENT_OR_STRING, (m, s: string | undefined) => s ?? '');
const upperFirst = (s: string) => s.slice(0, 1).toUpperCase() + s.slice(1);

// Text between the dictionary's `{` and its matching `}`, or null if the
// braces do not balance.
function dictBody(src: string, openBrace: number): string | null {
  let depth = 1;
  for (let i = openBrace; i < src.length; i++) {
    const c = src[i];
    if (c === '"') {
      i++;
      while (i < src.length && src[i] !== '"') i += src[i] === '\\' ? 2 : 1;
    } else if (c === '{') {
      depth++;
    } else if (c === '}') {
      depth--;
      if (depth === 0) return src.slice(openBrace, i);
    }
  }
  return null;
}

// IDL primitive -> [C++ type, needs Member<>, default initialiser]
const PRIMITIVES: Record<string, [string, boolean, string | null]> = {
  'boolean': ['bool', false, 'false'], 'byte': ['int8_t', false, '0'], 'octet': ['uint8_t', false, '0'], 'short': ['int16_t', false, '0'],
  'unsigned short': ['uint16_t', false, '0'], 'long': ['int32_t', false, '0'], 'unsigned long': ['uint32_t', false, '0'], 'long long': ['int64_t', false, '0'],
  'unsigned long long': ['uint64_t', false, '0'], 'float': ['float', false, '0'], 'unrestricted float': ['float', false, '0'], 'double': ['double', false, '0'],
  'unrestricted double': ['double', false, '0'], 'DOMHighResTimeStamp': ['double', false, '0'], 'DOMString': ['String', false, null], 'USVString': ['String', false, null],
  'ByteString': ['String', false, null], 'CSSOMString': ['String', false, null],
};
// No representation without a script engine.
const DICT_UNSUPPORTED = ['any', 'object', 'record<', 'Promise<', 'ArrayBuffer', 'ArrayBufferView', 'BufferSource', 'Function'];

const dictHeader = (o: {idl: string; guard: string; name: string; cls: string; base: string; export: string; includes: string; forwards: string; accessors: string; fields: string}) => `// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Generated by ${GENERATOR.dictionaries} from ${o.idl}.
// Do not edit; edit the .idl and re-run the generator.

#ifndef ${o.guard}
#define ${o.guard}

${o.includes}
namespace blink {

${o.forwards}// IDL dictionary ${o.name}.
class ${o.export}${o.cls} : public ${o.base} {
 public:
  static ${o.cls}* Create() { return MakeGarbageCollected<${o.cls}>(); }

  ${o.cls}() = default;
  ~${o.cls}() override = default;

${o.accessors}
  // Defined in the .cc: tracing a Member<T> needs T complete, and including
  // T's header here would close a cycle back through v8_typedefs.h.
  void Trace(Visitor* visitor) const override;

 private:
${o.fields}};

}  // namespace blink

#endif  // ${o.guard}
`;

const dictImpl = (o: {idl: string; header: string; cls: string; includes: string; traces: string; base: string}) => `// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Generated by ${GENERATOR.dictionaries} from ${o.idl}.
// Do not edit; edit the .idl and re-run the generator.

#include "${o.header}"

${o.includes}
namespace blink {

void ${o.cls}::Trace(Visitor* visitor) const {
${o.traces}  ${o.base}::Trace(visitor);
}

}  // namespace blink
`;

interface DictScan {
  dicts: Map<string, [string, string | undefined, string]>;  // name -> [idl rel, parent, body]
  ifaces: Set<string>;
  enums: Set<string>;
  idlOf: Map<string, string>;
}

// Collect every dictionary, interface and enum declared in the tree.
function scanDictionaries(): DictScan {
  const s: DictScan = {dicts: new Map(), ifaces: new Set(), enums: new Set(), idlOf: new Map()};
  for (const file of idlFiles()) {
    const src = stripComments(readFileSync(file, 'utf8'));
    const r = rel(file);
    for (const m of src.matchAll(DICT)) {
      const body = dictBody(src, m.index! + m[0].length);
      if (body === null) throw new Unsupported(`${r}: unbalanced braces in dictionary ${m[1]}`);
      if (!s.dicts.has(m[1])) s.dicts.set(m[1], [r, m[2], body]);
      if (!s.idlOf.has(m[1])) s.idlOf.set(m[1], r);
    }
    for (const m of src.matchAll(IFACE)) {
      s.ifaces.add(m[1]);
      if (!s.idlOf.has(m[1])) s.idlOf.set(m[1], r);
    }
    for (const m of src.matchAll(DENUM)) {
      s.enums.add(m[1]);
      if (!s.idlOf.has(m[1])) s.idlOf.set(m[1], r);
    }
  }
  return s;
}

interface UnionIndex {
  names: Names;
  index: Map<string, [string, string]>;  // tokens key -> [class, header]
  typedefs: Typedefs;
  defs: Map<string, Definition>;
}
let unionIndexCache: UnionIndex | null = null;

// (member token tuple) -> (class name, header path), built from the unions
// generator's own reading so the naming rule lives in exactly one place.
// Keying on tokens rather than on the written spelling is what makes
// `(double or CSSNumericValue)` and `(double or CSSNumberish)` -- one class,
// two spellings -- both resolve.
function unionIndex(): UnionIndex {
  if (unionIndexCache) return unionIndexCache;
  const sources = readSources();
  const [defs, typedefs] = collectDefinitionsAndTypedefs(sources);
  const names = new Names(typedefs);
  const [all] = collectUnions(sources, names);
  const index = new Map<string, [string, string]>();
  for (const [key, union] of all) {
    const header = `third_party/blink/renderer/bindings/${componentOf(union, defs, names)}/v8/${union.basename}.h`;
    if (!existsSync(path.join(root, header))) continue;  // the union generator skipped this one
    index.set(key, [union.cls, header]);
  }
  unionIndexCache = {names, index, typedefs, defs};
  return unionIndexCache;
}

// A typedef name whose target is a union -> [alias class, header]. The
// union's own class name and header, not the alias and v8_typedefs.h: a
// generated file may not use the alias, because reaching it means including
// v8_typedefs.h, which includes the union headers, which include the
// generated dictionaries they hold -- and the second visit to v8_typedefs.h
// is a no-op on the include guard. Naming the union directly keeps generated
// headers free of v8_typedefs.h entirely.
function resolveTypedefUnion(name: string): [string, string] | [null, null] {
  const {names, index, typedefs} = unionIndex();
  if (!typedefs.has(name)) return [null, null];
  let node = typedefs.get(name)![0];
  while (node.kind === 'nullable') node = node.args[0];
  if (node.kind !== 'union') return [null, null];
  let key: string;
  try {
    key = names.memberTokens(node).join('Or');
  } catch {
    return [null, null];
  }
  return index.get(key) ?? [null, null];
}

// Written union type -> [class name, header path], or [null, null].
function resolveUnion(text: string): [string, string] | [null, null] {
  const {names, index} = unionIndex();
  const parsed = new TypeParser(lex(text)).union(0);
  if (parsed === null) return [null, null];
  try {
    return index.get(names.memberTokens(parsed[0]).join('Or')) ?? [null, null];
  } catch {
    return [null, null];
  }
}

type Includes = {h: Set<string>; cc: Set<string>; fwd: Set<string>};

// The C++ class name an interface is [ImplementedAs=...] declared with.
function implementedAs(idlRel: string, name: string): string | null {
  const src = readFileSync(path.join(root, idlRel), 'utf8');
  const m = new RegExp(String.raw`\[([^\]]*)\]\s*(?:partial\s+)?interface\s+(?:mixin\s+)?${name}\b`, 's').exec(src);
  if (!m) return null;
  const a = /ImplementedAs\s*=\s*(\w+)/.exec(m[1]);
  return a ? a[1] : null;
}

// Where the C++ declaration of an IDL-declared type lives.
function headerFor(name: string, s: DictScan): string | null {
  const d = s.dicts.get(name);
  if (d) return `third_party/blink/renderer/bindings/${d[0].includes('/renderer/modules/') ? 'modules' : 'core'}/v8/v8_${snakeAttached(name)}.h`;
  const r = s.idlOf.get(name);
  if (!r) return null;
  const candidate = r.slice(0, -4) + '.h';  // sibling header of the .idl
  if (existsSync(path.join(root, candidate))) return candidate;
  // [ImplementedAs=X] renames the C++ class, and the header follows the class
  // rather than the .idl: window.idl declares `interface Window` but the
  // header next to it is dom_window.h.
  const impl = implementedAs(r, name);
  if (impl) {
    const sibling = `${path.posix.dirname(r)}/${snakeAttached(impl)}.h`;
    if (existsSync(path.join(root, sibling))) return sibling;
  }
  return null;
}

// IDL type -> [C++ type, is_garbage_collected, default initialiser].
function cppType(idlType: string, s: DictScan, includes: Includes): [string, boolean, string | null] {
  let t = idlType.trim();
  const nullable = t.endsWith('?');
  if (nullable) t = t.slice(0, -1).trim();
  if (DICT_UNSUPPORTED.some((u) => t.includes(u))) throw new Unsupported(t);
  if (t.startsWith('(')) {
    // A union member: the unions generator's naming rule, reused rather than
    // restated.
    const [cls, header] = resolveUnion(t);
    if (cls === null) throw new Unsupported(`union ${t}`);
    includes.h.add(header);
    return [cls, true, null];
  }
  const seq = /^sequence<(.+)>$/.exec(t);
  if (seq) {
    const [inner, gc] = cppType(seq[1], s, includes);
    if (gc) {
      includes.h.add('third_party/blink/renderer/platform/heap/collection_support/heap_vector.h');
      return [`HeapVector<Member<${inner}>>`, true, null];
    }
    includes.h.add('third_party/blink/renderer/platform/wtf/vector.h');
    return [`Vector<${inner}>`, false, null];
  }
  if (t in PRIMITIVES) {
    const [cpp, gc, def] = PRIMITIVES[t];
    if (nullable && cpp !== 'String') return [`std::optional<${cpp}>`, false, null];
    return [cpp, gc, def];
  }
  if (s.enums.has(t)) {
    includes.h.add(`third_party/blink/renderer/bindings/core/v8/v8_${snakeAttached(t)}.h`);
    return [`V8${t}`, false, null];
  }
  if (s.dicts.has(t)) {
    const header = headerFor(t, s);
    if (header === null) throw new Unsupported(`no header for ${t}`);
    includes.h.add(header);
    return [t, true, null];
  }
  if (s.ifaces.has(t)) {
    const header = headerFor(t, s);
    if (header === null) throw new Unsupported(`no header for ${t}`);
    // The IDL interface name is not always the C++ class name: `interface
    // Window` is [ImplementedAs=DOMWindow]. Read the same table the unions
    // generator honours. An interface header is the cycle (element.h reaches
    // back here through v8_typedefs.h): forward-declare, include in the .cc.
    const d = unionIndex().defs.get(t);
    const cpp = d ? d.impl : t;
    includes.cc.add(header);
    includes.fwd.add(cpp);
    return [cpp, true, null];
  }
  const [cls, header] = resolveTypedefUnion(t);
  if (cls !== null) {
    includes.h.add(header);
    return [cls, true, null];
  }
  throw new Unsupported(`unknown type ${t}`);
}

// A dictionary body -> [required, idl type, name, cpp name, default].
function membersOf(body: string): Array<[boolean, string, string, string, string | null]> {
  const out: Array<[boolean, string, string, string, string | null]> = [];
  for (const raw of body.split(';')) {
    const impl = /ImplementedAs\s*=\s*(\w+)/.exec(raw);
    let decl = raw.replace(EXT_ATTR, '').trim();
    if (!decl) continue;
    let def: string | null = null;
    if (decl.includes('=')) {
      const eq = decl.indexOf('=');
      def = decl.slice(eq + 1).trim();
      decl = decl.slice(0, eq).trim();
    }
    const required = decl.startsWith('required ');
    if (required) decl = decl.slice('required '.length).trim();
    const split = decl.search(/\s\S+$/);
    if (split < 0) throw new Unsupported(`cannot parse member '${raw.trim()}'`);
    const name = decl.slice(split + 1).trim();
    out.push([required, decl.slice(0, split).trim(), name, impl ? impl[1] : name, def]);
  }
  return out;
}

// The dictionaries generator's own enumerator spelling: an empty value is
// kEmptyString here, where the enums generator says a bare k.
function dictEnumEntry(value: string): string {
  if (value === '') return 'kEmptyString';
  const parts = value.split(/[^A-Za-z0-9]+/).filter(Boolean);
  return 'k' + parts.map((p) => p.toLowerCase() === 'srgb' ? 'SRGB' : /^[A-Z]{2}/.test(p) ? p : upperFirst(p)).join('');
}

// The first enumerator of an IDL enum, for zero-initialising a member.
function firstEnumEntry(name: string, s: DictScan): string | null {
  const r = s.idlOf.get(name);
  if (!r) return null;
  const src = stripComments(readFileSync(path.join(root, r), 'utf8'));
  const m = new RegExp(String.raw`enum\s+${name}\s*\{([\s\S]*?)\}`).exec(src);
  if (!m) return null;
  const values = [...m[1].matchAll(/"([^"]*)"/g)].map((v) => v[1]);
  return values.length ? dictEnumEntry(values[0]) : null;
}

// IDL default value -> C++ initialiser.
function literal(def: string, enumType: boolean): string | null {
  if (def === 'true' || def === 'false') return def;
  if (def === 'null') return null;
  if (def.startsWith('"')) return enumType ? null : def;  // an enum default needs Enum::k, handled by the caller
  if (def === '[]' || def === '{}') return null;
  return def;  // numeric
}

function generateDictionary(name: string, s: DictScan, dry: boolean): void {
  const [idlRel, parent, body] = s.dicts.get(name)!;
  // Three buckets: what the header includes, what the .cc includes, and what
  // the header forward-declares in place of a .cc include.
  const includes: Includes = {h: new Set(), cc: new Set(), fwd: new Set()};
  const accessors: string[] = [], fields: string[] = [], traces: string[] = [];
  let base: string;
  if (parent) {
    if (!s.dicts.has(parent)) throw new Unsupported(`parent ${parent} is not a dictionary`);
    base = parent;
    includes.h.add(headerFor(parent, s)!);
  } else {
    base = 'bindings::DictionaryBase';
    includes.h.add('third_party/blink/renderer/platform/bindings/dictionary_base.h');
  }
  for (const [required, idlType, member, cppName, def] of membersOf(body)) {
    const [cpp, gc, zero] = cppType(idlType, s, includes);
    const field = `member_${snakeAttached(member)}_`;
    const has = `has_${snakeAttached(member)}_`;
    const cap = upperFirst(cppName);
    let init = '';
    let present = 'false';
    const bareType = idlType.trim().replace(/\?$/, '');
    if (def !== null) {
      const enumDefault = s.enums.has(bareType);
      if (enumDefault && def.startsWith('"')) {
        init = ` = V8${bareType}(V8${bareType}::Enum::${dictEnumEntry(def.replace(/^"+|"+$/g, ''))})`;
        present = 'true';
      } else {
        const lit = literal(def, enumDefault);
        if (lit !== null) {
          init = ` = ${lit}`;
          present = 'true';
        }
      }
    } else if (zero !== null) {
      init = ` = ${zero}`;
    } else if (cpp.startsWith('V8') && !gc) {
      // An IDL enum: no default constructor, so pick value 0. has_..._ stays
      // false, so nothing reads it until it is set.
      const first = firstEnumEntry(bareType, s);
      if (first === null) throw new Unsupported(`no enumerator for ${bareType}`);
      init = ` = ${cpp}(${cpp}::Enum::${first})`;
    }
    let ret: string, arg: string, get: string, setv: string;
    if (gc) {
      // A HeapVector is already a heap collection: it is traced directly and
      // is not itself a GarbageCollected type, so it is not wrapped in
      // Member<>.
      const decl = cpp.startsWith('HeapVector<') ? cpp : `Member<${cpp}>`;
      fields.push(`  ${decl} ${field};\n`);
      traces.push(`  visitor->Trace(${field});\n`);
      if (cpp.startsWith('HeapVector<')) {
        ret = `const ${cpp}&`;
        arg = `const ${cpp}& value`;
        get = field;
        setv = `${field} = value`;
      } else {
        ret = `${cpp}*`;
        arg = `${cpp}* value`;
        get = `${field}.Get()`;
        setv = `${field} = value`;
      }
    } else {
      fields.push(`  ${cpp} ${field}${init};\n`);
      const ref = cpp === 'bool' || cpp === 'double' || cpp === 'float' || cpp.startsWith('int') || cpp.startsWith('uint') ? '' : 'const ';
      ret = cpp;
      arg = `${ref}${cpp}${ref ? '&' : ''} value`;
      get = field;
      setv = `${field} = value`;
    }
    fields.push(`  bool ${has} = ${present};\n`);
    accessors.push(`  bool has${cap}() const { return ${has}; }\n  ${ret} ${cppName}() const { return ${get}; }\n  void set${cap}(${arg}) { ${setv}; ${has} = true; }\n`);
    // getFooOr(fallback): upstream emits one for every member, and call sites
    // use it wherever "absent" and "present with this value" mean the same
    // thing.
    accessors.push(`  ${ret} get${cap}Or(${arg}) const { return ${has} ? ${get} : value; }\n\n`);
    // Upstream enforces required-ness while converting from a script value,
    // which is the code path that is gone. Record it and move on.
    if (required) accessors.push(`  // ${member} is \`required\` in ${path.posix.basename(idlRel)}.\n\n`);
  }
  const part = idlRel.includes('/renderer/modules/') ? 'modules' : 'core';
  const outDir = path.join(RENDERER, 'bindings', part, 'v8');
  const guard = `THIRD_PARTY_BLINK_RENDERER_BINDINGS_${part.toUpperCase()}_V8_V8_${snakeAttached(name).toUpperCase()}_H_`;
  let incLines = '#include "third_party/blink/renderer/platform/heap/garbage_collected.h"\n#include "third_party/blink/renderer/platform/heap/member.h"\n#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"\n';
  incLines += [...includes.h].filter(Boolean).sort().map((i) => `#include "${i}"\n`).join('');
  incLines += `#include "third_party/blink/renderer/${part}/${part}_export.h"\n\n`;
  let fwdLines = [...includes.fwd].sort().map((f) => `class ${f};\n`).join('');
  if (fwdLines) fwdLines += '\n';
  const header = `third_party/blink/renderer/bindings/${part}/v8/v8_${snakeAttached(name)}.h`;
  const headerText = dictHeader({idl: idlRel, guard, name, cls: name, base, export: part === 'core' ? 'CORE_EXPORT ' : 'MODULES_EXPORT ', includes: incLines, forwards: fwdLines, accessors: accessors.join(''), fields: fields.join('')});
  const implText = dictImpl({idl: idlRel, header, cls: name, includes: [...includes.cc].filter(Boolean).sort().map((i) => `#include "${i}"\n`).join(''), traces: traces.join(''), base});
  if (!dry) {
    write(path.join(outDir, `v8_${snakeAttached(name)}.h`), headerText);
    write(path.join(outDir, `v8_${snakeAttached(name)}.cc`), implText);
  }
}

function dictionaries(dry: boolean, only: Set<string> | null): void {
  const s = scanDictionaries();
  const names = only ? [...only].sort() : [...s.dicts.keys()].sort();
  let done = 0, skipped = 0;
  const reasons = new Map<string, string>();
  const queue = [...names];
  const seen = new Set<string>();
  while (queue.length) {
    const name = queue.shift()!;
    if (seen.has(name)) continue;
    seen.add(name);
    if (!s.dicts.has(name)) {
      reasons.set(name, 'not a dictionary in any .idl');
      skipped++;
      continue;
    }
    const parent = s.dicts.get(name)![1];
    if (parent && !seen.has(parent)) queue.push(parent);
    try {
      generateDictionary(name, s, dry);
      done++;
    } catch (e) {
      if (!(e instanceof Unsupported)) throw e;
      reasons.set(name, e.message);
      skipped++;
    }
  }
  console.log(`${done} dictionaries generated, ${skipped} skipped${dry ? ' (dry run)' : ''}`);
  for (const name of [...reasons.keys()].sort()) console.log(`  skip   ${name.padEnd(42)} ${reasons.get(name)}`);
}

// ===========================================================================
// prune, build-gni, all

// Hand-written files that live in the generated directories, with the note
// build.gni carries above them. The generator lists every source it finds,
// so the note has to be emitted here or a regeneration silently drops it.
const SOURCE_NOTES: Record<string, string> = {
  'active_script_wrappable.cc': `  # Restored: despite the directory, this holds no V8. It is one function --
  # "is this ExecutionContext destroyed?" -- kept out of line so the header does
  # not have to include execution_context.h. ActiveScriptWrappable is still how
  # HTMLMediaElement and friends decide whether they are still alive, so the
  # templates in the header still call it.
`,
};

const FROM = new RegExp(String.raw`^// Generated by ${BANNER_PREFIX.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}(\S+) from (\S+?)\.$`, 'm');
const BANNER = new RegExp(String.raw`^// Generated by ${BANNER_PREFIX.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}(\S+) from `, 'm');

function bindingDirs(): string[] {
  return readdirSync(BINDINGS).sort().map((c) => path.join(BINDINGS, c, 'v8')).filter((d) => existsSync(d) && statSync(d).isDirectory());
}

// Delete generated binding files whose source .idl is gone. Every generated
// file carries the line `// Generated by ... from <path>.idl.` so the
// question "does this file still have an input?" is answerable from the file
// itself. This runs *after* the generators: anything still wanted has just
// been written, and only genuine orphans are left pointing at a path that no
// longer exists.
function prune(dry: boolean): void {
  const orphans: Array<[string, string]> = [];
  let scanned = 0;
  for (const d of bindingDirs()) {
    for (const name of readdirSync(d).sort()) {
      if (!name.endsWith('.h')) continue;
      const file = path.join(d, name);
      const m = FROM.exec(head(file, 2048));
      if (!m) continue;  // hand-written, or v8_typedefs.h
      scanned++;
      if (existsSync(path.join(root, m[2]))) continue;
      orphans.push([file, m[2]]);
    }
  }
  for (const [file, src] of orphans) {
    console.log(`  orphan ${rel(file).padEnd(72)} (from ${src})`);
    if (dry) continue;
    rmSync(file);
    const impl = file.slice(0, -2) + '.cc';
    if (existsSync(impl)) rmSync(impl);
  }
  console.log(`${scanned} generated headers scanned, ${orphans.length} orphaned${dry ? ' (dry run)' : ' and deleted'}`);
  console.log('Re-run build-gni to drop them from build.gni.');
}

// Write the GN source list for the regenerated bindings. The unions and
// dictionaries emit a .cc as well as a .h, and those have to be compiled, so
// they need to be listed. Scanning the directory rather than having each
// generator maintain its share of the list is what keeps the three of them
// from disagreeing about it.
function buildGni(): void {
  const gni = (names: string[]) => `# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Generated by ${GENERATOR.buildGni}. Do not edit.
#
# The IDL enum, union and dictionary classes, regenerated without V8 by
# scripts/gen-idl.ts. Upstream produced these from the same .idl files
# through third_party/blink/renderer/bindings/scripts, which went with V8.
#
# They are part of the core component rather than a target of their own: the
# .cc files include core headers (a union of (double or CSSNumericValue) traces
# a Member<CSSNumericValue>) and core includes these, so any other arrangement
# is a dependency cycle.
blink_core_sources_bindings_core_v8 = [
${names.map((n) => `${SOURCE_NOTES[n] ?? ''}  "${n}",\n`).join('')}]
`;
  const build = `# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import("//third_party/blink/renderer/core/core.gni")
import("//third_party/blink/renderer/bindings/core/v8/build.gni")

blink_core_sources("bindings_core_v8") {
  sources = blink_core_sources_bindings_core_v8

  deps = [ "//third_party/blink/renderer/core:core_generated" ]
}
`;
  for (const component of ['core', 'modules']) {
    const d = path.join(BINDINGS, component, 'v8');
    if (!existsSync(d)) continue;
    const names = readdirSync(d).filter((n) => n.endsWith('.cc') || n.endsWith('.h')).sort();
    if (!names.some((n) => n.endsWith('.cc'))) continue;
    writeFileSync(path.join(d, 'build.gni'), gni(names));
    writeFileSync(path.join(d, 'BUILD.gn'), build.replaceAll('core/v8', `${component}/v8`).replaceAll('bindings_core_v8', `bindings_${component}_v8`));
    console.log(`  ${component}/v8: ${names.length} sources`);
  }
}

// Every .h under bindings/*/v8 carrying a generator banner.
function generatedFiles(): string[] {
  const out: string[] = [];
  for (const d of bindingDirs()) {
    for (const name of readdirSync(d).sort()) {
      if (!name.endsWith('.h')) continue;
      const file = path.join(d, name);
      if (BANNER.test(head(file, 2048))) out.push(file);
    }
  }
  return out;
}

// The three generators, then delete what they no longer produce, then
// rewrite build.gni. The criterion is exact: every generator rewrites all of
// its outputs on every run, so a generated file not touched during this run
// is not produced any more. A whole second of slack covers the clock, not
// the work.
function all(dry: boolean): void {
  const start = Date.now() - 1000;
  console.log(`== ${GENERATOR.enums}`);
  enums(dry, false);
  console.log(`== ${GENERATOR.unions}`);
  unions(dry, false, false);
  console.log(`== ${GENERATOR.dictionaries}`);
  unionIndexCache = null;
  dictionaries(dry, null);
  if (dry) {
    console.log('== dry run: not pruning, not rewriting build.gni');
    return;
  }
  console.log('== prune');
  const stale = generatedFiles().filter((p) => statSync(p).mtimeMs < start);
  for (const file of stale) {
    console.log(`  stale  ${rel(file)}`);
    rmSync(file);
    const impl = file.slice(0, -2) + '.cc';
    if (existsSync(impl)) rmSync(impl);
  }
  console.log(`  ${stale.length} generated header(s) no longer produced`);
  console.log('== build.gni');
  buildGni();
}

// ===========================================================================

// -n, -v and --check are flags without values; cac 7 would hand the next
// argument to them, so they are read off argv directly.
const argv = process.argv.slice(2);
const dry = argv.includes('-n'), verbose = argv.includes('-v'), check = argv.includes('--check');
const rest = argv.filter((a) => a !== '-n' && a !== '-v' && a !== '--check');
const cli = cac('gen-idl');
cli.command('enums', 'the V8<Enum> classes').option('-n', 'dry run').option('--check', 'verify every V8Foo::Enum::kBar named in core+platform is produced')
    .action(() => enums(dry, check));
cli.command('unions', 'the V8Union<...> classes and v8_typedefs.h').option('-n', 'dry run').option('-v', 'list every union produced').option('--check', 'cross-check against the call sites')
    .action(() => unions(dry, verbose, check));
cli.command('dictionaries', 'the IDL dictionary classes').option('-n', 'dry run').option('--only <names>', 'generate just these dictionaries (comma-separated) and their parents')
    .action((options: {only?: string}) => dictionaries(dry, options.only ? new Set(options.only.split(',')) : null));
cli.command('prune', 'delete generated files whose source .idl is gone').option('-n', 'dry run')
    .action(() => prune(dry));
cli.command('build-gni', 'rewrite the GN source lists for the generated .cc files')
    .action(() => buildGni());
cli.command('all', 'the three generators, the prune and the build.gni rewrite').option('-n', 'dry run')
    .action(() => all(dry));
cli.help();
try {
  cli.parse([...process.argv.slice(0, 2), ...rest], {run: false});
  if (!cli.matchedCommand && !cli.options.help) {
    cli.outputHelp();
    process.exitCode = 2;
  } else {
    await cli.runMatchedCommand();
  }
} catch (error) {
  console.error(error instanceof Error ? error.stack ?? error.message : String(error));
  process.exitCode = 1;
}
