// Evaluate a pinned DEPS file with the pinned depot_tools and emit JSON.
//
// This is the machine-readable half of the bootstrap's input lock. The DEPS
// parsing is done by depot_tools' own gclient_eval rather than a parser of
// this project's: the condition language, deps_os folding and the CIPD/GCS
// schemas are gclient implementation details that a hand-written parser gets
// subtly wrong. depot_tools' python is asked to parse and evaluate; the
// counting and the JSON shape are this file's.
//
// Nothing here touches the network or any checkout. Input is a DEPS blob;
// output is a JSON document on stdout or at --out.
//
//   pnpm deps-lock --deps-file DEPS.pinned --depot-tools D:/Github/depot_tools
//                  [--custom-deps-file bootstrap/custom-deps-batch1.txt]
//                  [--custom-vars '{"checkout_configuration":"small"}']
//                  [--out lock.json] [--baseline-matrix] [--include-deps]

import {createHash} from 'node:crypto';
import {existsSync, readFileSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execaSync} from 'execa';

import {resolve} from './lib/repo.ts';

// Mirrors gclient.GClient.get_builtin_vars() for target_os=['win'],
// target_cpu=['x64'], host win/x64. Hard-coded rather than detected: the lock
// is a statement about the intended build, not about whatever machine runs
// it.
export const BUILTIN_VARS: Record<string, boolean | string> = {
  checkout_android: false, checkout_chromeos: false, checkout_fuchsia: false, checkout_ios: false, checkout_linux: false, checkout_mac: false,
  checkout_win: true, host_os: 'win', checkout_arm: false, checkout_arm64: false, checkout_x86: false, checkout_mips: false, checkout_mips64: false,
  checkout_ppc: false, checkout_riscv64: false, checkout_s390: false, checkout_x64: true, checkout_loong64: false, host_cpu: 'x64',
};

// What depot_tools' python is asked to do: parse DEPS with gclient_eval and
// evaluate every condition -- entry, GCS object and hook -- against the
// merged variables (DEPS < builtin < custom, gclient's precedence).
const BRIDGE = `
import json, sys
sys.path.insert(0, sys.argv[1])
import gclient_eval
content = open(sys.argv[2], encoding='utf-8').read()
builtin = json.loads(sys.argv[3])
rows = []
for cvars in json.loads(sys.argv[4]):
    parsed = gclient_eval.Parse(content, filename='DEPS', vars_override=cvars, builtin_vars=builtin)
    variables = dict(parsed.get('vars', {})); variables.update(builtin); variables.update(cvars)
    def ev(c):
        return True if not c else bool(gclient_eval.EvaluateCondition(c, variables))
    deps = {}
    for p, info in parsed.get('deps', {}).items():
        if info is None:
            deps[p] = None
            continue
        d = {'dep_type': info.get('dep_type', 'git'), 'condition': info.get('condition'), 'selected': ev(info.get('condition'))}
        if d['dep_type'] == 'git':
            d['url'] = info.get('url')
        elif d['dep_type'] == 'cipd':
            d['packages'] = [{'package': x.get('package'), 'version': x.get('version')} for x in info.get('packages', [])]
        elif d['dep_type'] == 'gcs':
            d['bucket'] = info.get('bucket')
            d['objects'] = [{'object_name': o.get('object_name'), 'sha256sum': o.get('sha256sum'), 'size_bytes': o.get('size_bytes'), 'generation': o.get('generation'), 'condition': o.get('condition'), 'selected': ev(o.get('condition'))} for o in info.get('objects', [])]
        deps[p] = d
    hooks = [{'name': h.get('name'), 'pattern': h.get('pattern'), 'condition': h.get('condition'), 'selected': ev(h.get('condition')), 'action': [str(a) for a in h.get('action', [])]} for h in parsed.get('hooks', [])]
    rows.append({'deps': deps, 'hooks': hooks})
sys.stdout.write(json.dumps(rows))
`;

interface GcsObject {
  object_name: string | null;
  sha256sum: string | null;
  size_bytes: number | null;
  generation: number | null;
  condition: string | null;
  selected: boolean;
}
interface DepInfo {
  dep_type: 'git' | 'cipd' | 'gcs';
  condition: string | null;
  selected: boolean;
  url?: string;
  packages?: Array<{package: string; version: string}>;
  bucket?: string;
  objects?: GcsObject[];
}
interface HookInfo {
  name: string | null;
  pattern: string | null;
  condition: string | null;
  selected: boolean;
  action: string[];
}
interface Evaluated {
  deps: Record<string, DepInfo | null>;
  hooks: HookInfo[];
}

export function evaluateDeps(python: string, depotTools: string, depsFile: string, customVarsRows: Array<Record<string, unknown>>): Evaluated[] {
  const r = execaSync(python, ['-c', BRIDGE, depotTools, depsFile, JSON.stringify(BUILTIN_VARS), JSON.stringify(customVarsRows)], {reject: false, maxBuffer: 1 << 28});
  if (r.exitCode !== 0) throw new Error(`gclient_eval failed (${r.exitCode}):\n${r.stderr}`);
  return JSON.parse(r.stdout) as Evaluated[];
}

export interface DepsSummary {
  declared: number;
  selectedBeforeCustomDeps: number;
  customDepsRequested: number;
  customDepsActiveRemovals: number;
  remaining: number;
  git: number;
  cipd: number;
  gcs: number;
  gcsSizeBytes: number;
  gcsDeclaredSizeBytes: number;
}

export function summarize(evaluated: Evaluated, customDeps: string[]): [DepsSummary, Array<Record<string, unknown>>] {
  const removed = new Set(customDeps);
  const entries: Array<Record<string, unknown>> = [];
  const counts = {git: 0, cipd: 0, gcs: 0};
  let gcsBytes = 0, gcsDeclaredBytes = 0, selected = 0, removedActive = 0;
  for (const p of Object.keys(evaluated.deps).sort()) {
    const info = evaluated.deps[p];
    if (info === null) {
      entries.push({path: p, type: 'none', selected: false, condition: null, removedByCustomDeps: removed.has(p)});
      continue;
    }
    const entry: Record<string, unknown> = {path: p, type: info.dep_type, condition: info.condition, selected: info.selected, removedByCustomDeps: removed.has(p)};
    let fetched = 0, declared = 0;
    if (info.dep_type === 'git') entry.url = info.url;
    else if (info.dep_type === 'cipd') entry.packages = info.packages;
    else if (info.dep_type === 'gcs') {
      entry.bucket = info.bucket;
      entry.objects = (info.objects ?? []).map((o) => ({objectName: o.object_name, sha256sum: o.sha256sum, sizeBytes: o.size_bytes, generation: o.generation, condition: o.condition}));
      // Object-level conditions are ANDed with the entry condition, and on
      // Windows x64 they exclude the other-platform Rust/LLVM/libclang
      // objects. Both numbers are emitted so a report cannot silently mix
      // them.
      for (const o of info.objects ?? []) {
        const size = Number(o.size_bytes ?? 0);
        declared += size;
        if (o.selected) fetched += size;
      }
      entry.sizeBytes = fetched;
      entry.declaredSizeBytes = declared;
    }
    if (info.selected) {
      if (removed.has(p)) {
        // custom_deps = None only saves anything when the entry would
        // otherwise have been selected.
        removedActive++;
      } else {
        selected++;
        counts[info.dep_type]++;
        if (info.dep_type === 'gcs') {
          gcsBytes += fetched;
          gcsDeclaredBytes += declared;
        }
      }
    }
    entries.push(entry);
  }
  return [{
    declared: Object.keys(evaluated.deps).length, selectedBeforeCustomDeps: selected + removedActive, customDepsRequested: removed.size,
    customDepsActiveRemovals: removedActive, remaining: selected, git: counts.git, cipd: counts.cipd, gcs: counts.gcs, gcsSizeBytes: gcsBytes, gcsDeclaredSizeBytes: gcsDeclaredBytes,
  }, entries];
}

export function summarizeHooks(evaluated: Evaluated) {
  return evaluated.hooks.map((h, index) => ({
    order: index, name: h.name, pattern: h.pattern, condition: h.condition, selected: h.selected, action: h.action,
    actionSha256: createHash('sha256').update(h.action.join('\n'), 'utf8').digest('hex'),
  }));
}

// Reads the candidate list; '#' comments and blank lines are ignored.
export function readCustomDeps(file: string | undefined): string[] {
  if (!file) return [];
  const out: string[] = [];
  for (const line of readFileSync(file, 'utf8').split(/\r?\n/)) {
    const stripped = line.split('#', 1)[0].trim();
    if (stripped) out.push(stripped);
  }
  if (new Set(out).size !== out.length) throw new Error(`duplicate entries in ${file}`);
  return out;
}

// json.dumps(indent=2, sort_keys=True), as the Python emitted it.
export function sortedJson(value: unknown): string {
  const sort = (v: unknown): unknown => {
    if (Array.isArray(v)) return v.map(sort);
    if (v && typeof v === 'object') return Object.fromEntries(Object.keys(v as object).sort().map((k) => [k, sort((v as Record<string, unknown>)[k])]));
    return v;
  };
  return JSON.stringify(sort(value), null, 2);
}

export interface LockOptions {
  depsFile: string;
  depotTools: string;
  customDepsFile?: string;
  customVars: Record<string, unknown>;
  baselineMatrix: boolean;
  includeDeps: boolean;
  python: string;
}

export function depsLock(o: LockOptions): Record<string, unknown> {
  if (!existsSync(o.depotTools)) throw new Error(`depot_tools not found: ${o.depotTools}`);
  const raw = readFileSync(o.depsFile);
  const customDeps = readCustomDeps(o.customDepsFile);
  const rows: Array<[string, Record<string, unknown>, string[]]> = [['main', o.customVars, customDeps]];
  if (o.baselineMatrix) {
    rows.push(['default', {}, []], ['small', {checkout_configuration: 'small'}, []], ['small+null', {checkout_configuration: 'small'}, customDeps]);
  }
  const evaluated = evaluateDeps(o.python, o.depotTools, o.depsFile, rows.map((r) => r[1]));
  const [summary, entries] = summarize(evaluated[0], customDeps);
  const hooks = summarizeHooks(evaluated[0]);
  const result: Record<string, unknown> = {
    schema: 'shot-deps-lock/1',
    depsSha256: createHash('sha256').update(raw).digest('hex'),
    depsBytes: raw.length,
    customVars: o.customVars,
    customDeps,
    builtinVars: BUILTIN_VARS,
    summary,
    hooks,
    hookCount: hooks.length,
    selectedHookCount: hooks.filter((h) => h.selected).length,
  };
  if (o.includeDeps) result.deps = entries;
  if (o.baselineMatrix) {
    const matrix: Record<string, DepsSummary> = {};
    rows.slice(1).forEach(([label, , cdeps], i) => {
      matrix[label] = summarize(evaluated[i + 1], cdeps)[0];
    });
    result.baselineMatrix = matrix;
  }
  return result;
}

if (process.argv[1] && path.resolve(process.argv[1]) === import.meta.filename) {
  const argv = process.argv.slice(2);
  const baselineMatrix = argv.includes('--baseline-matrix'), includeDeps = argv.includes('--include-deps');
  const cli = cac('deps-lock');
  cli.command('', 'evaluate a DEPS blob with the pinned depot_tools and emit JSON')
      .option('--deps-file <file>', 'path to the DEPS blob to evaluate')
      .option('--depot-tools <dir>', 'pinned depot_tools checkout')
      .option('--custom-deps-file <file>', 'newline separated dep paths to set to None')
      .option('--custom-vars <json>', 'JSON object of .gclient custom_vars', {default: '{}'})
      .option('--python <exe>', 'the python to run gclient_eval with', {default: 'python3'})
      .option('--out <file>', 'write JSON here instead of stdout')
      .option('--baseline-matrix', 'also emit the default/small/small+null rows')
      .option('--include-deps', 'include the full per-dependency list')
      .action((options: {depsFile?: string; depotTools?: string; customDepsFile?: string; customVars: string; python: string; out?: string}) => {
        try {
          if (!options.depsFile || !options.depotTools) throw new Error('--deps-file and --depot-tools are required');
          const text = sortedJson(depsLock({
            depsFile: resolve(options.depsFile), depotTools: resolve(options.depotTools), customDepsFile: options.customDepsFile ? resolve(options.customDepsFile) : undefined,
            customVars: JSON.parse(options.customVars) as Record<string, unknown>, baselineMatrix, includeDeps, python: options.python,
          }));
          if (options.out) writeFileSync(resolve(options.out), text + '\n');
          else process.stdout.write(text + '\n');
        } catch (error) {
          console.error(error instanceof Error ? error.message : String(error));
          process.exitCode = 1;
        }
      });
  cli.help();
  cli.parse([...process.argv.slice(0, 2), ...argv.filter((a) => a !== '--baseline-matrix' && a !== '--include-deps')]);
}
