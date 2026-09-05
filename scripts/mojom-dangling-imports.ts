// Find `import "..."` lines in .mojom files whose target no longer exists.
//
// mojom_parser resolves every import before generating anything, and it
// reports the *first* unresolvable one and stops. A target holding thirty
// mojoms that each import a different deleted component therefore costs
// thirty build rounds to discover, one name per round -- the same trap as
// ninja's missing-input check, one layer up.
//
// Imports are always root-relative in chromium, so no include-path guessing is
// needed. But some mojoms *are* generated -- blink emits
// runtime_feature_state/runtime_feature.mojom and
// origin_trials/origin_trial_feature.mojom from its feature lists -- and those
// resolve against <out>/gen, not the source root. So the gen directory is
// searched too.
//
//   pnpm mojom:dangling-imports <out-dir> <dir> [<dir> ...]

import {existsSync, readFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {globSync} from 'tinyglobby';

import {resolve, root} from './lib/repo.ts';

const IMPORT = /^import "([^"]+)";/gm;
// An import can be guarded, and a guarded one is never resolved unless the
// feature is on. Only is_win is passed to mojom_parser here, so anything
// guarded by another feature is left out rather than reported as a blocker.
const GUARDED_IMPORT = /^\[EnableIf(?:Not)?=(\w+)\]\s*\n\s*import "([^"]+)";/gm;
const ENABLED_FEATURES = new Set(['is_win']);

function main(outArg: string, dirs: string[]): number {
  const gen = path.join(resolve(outArg), 'gen');
  const missing = new Map<string, string[]>();
  for (const d of dirs) {
    for (const fp of globSync('**/*.mojom', {cwd: resolve(d), absolute: true})) {
      let src: string;
      try {
        src = readFileSync(fp, 'utf8');
      } catch {
        continue;
      }
      const guarded = new Map<string, string>();
      for (const m of src.matchAll(GUARDED_IMPORT)) if (!ENABLED_FEATURES.has(m[1])) guarded.set(m[2], m[1]);
      for (const m of src.matchAll(IMPORT)) {
        const imp = m[1];
        if (guarded.has(imp)) continue;
        if (existsSync(path.join(root, imp))) continue;
        if (existsSync(path.join(gen, imp))) continue;  // Generated into <out>/gen.
        const rel = path.relative(root, fp).replace(/\\/g, '/');
        missing.set(imp, [...(missing.get(imp) ?? []), rel]);
      }
    }
  }
  for (const [imp, who] of [...missing.entries()].sort((a, b) => b[1].length - a[1].length)) {
    console.log(`${String(who.length).padStart(4)}  ${imp}`);
    for (const w of [...who].sort().slice(0, 6)) console.log(`        ${w}`);
    if (who.length > 6) console.log(`        ... ${who.length - 6} more`);
  }
  const total = [...missing.values()].reduce((s, v) => s + v.length, 0);
  console.log(`---- ${total} dangling import(s), ${missing.size} distinct target(s)`);
  return 0;
}

const cli = cac('mojom-dangling-imports');
cli.command('<out> [...dirs]', 'mojom imports whose target is gone from the tree and <out>/gen')
    .action((out: string, dirs: string[]) => {
      if (dirs.length === 0) throw new Error('at least one directory is required');
      process.exitCode = main(out, dirs);
    });
cli.help();
cli.parse();
