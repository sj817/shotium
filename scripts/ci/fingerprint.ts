// The engine fingerprint: a hash of every tracked file the six native builds
// depend on, and of nothing else.
//
// why: the engine artifacts used to be found by the commit they were built
// at, so a README commit after the builds meant six more builds before a
// release could be cut, and the version bump itself was such a commit. The
// binaries do not read the README. Naming the artifacts after the content
// that produced them -- `engine-linux-amd64-<fingerprint>` -- lets a commit
// that changes documentation, the TypeScript package, the benchmark or the
// workflows around the build reuse what is already built, and lets a
// release tag reuse whatever `main` built last.
//
// The hash covers `git ls-tree -r HEAD`: mode, blob id and path of every
// tracked file, plus the DEPS gitlinks. That is content-addressed and
// independent of the commit, the branch, or how a pull request was merged,
// and reading it needs no blobs, so a `--filter=blob:none` checkout is
// enough. Paths are classified by the rules below; a path nobody thought
// about is an input, because the cost of a wrong "input" is one avoidable
// build while the cost of a wrong "ignored" is a release built from stale
// bytes.
//
//   pnpm ci:fingerprint                 # prints the id
//   pnpm ci:fingerprint --explain       # what was counted and what was not
//   pnpm ci:fingerprint --output        # also appends fingerprint=<id> to $GITHUB_OUTPUT
//   pnpm ci:fingerprint --paths-ignore  # the paths-ignore block engine.yml carries

import {createHash} from 'node:crypto';
import {appendFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';

import {resolve} from '../lib/repo.ts';

// Bump when the hashing itself changes meaning; every artifact is rebuilt once.
export const SCHEMA = 1;

// An ALLOW hit is an input even where a DENY rule would exclude it: these
// live under directories that are otherwise documentation and packages, but
// the build reads them.
export const ALLOW: readonly RegExp[] = [
  /^apps\/typescript\/native\//,                    // compiled into shotium.node by shot/BUILD.gn
  /^apps\/c-abi\//,                                 // C_ABI.md, C_ABI.zh.md ride in the C ABI archive
  /^apps\/demo-card\/card\.html$/,                  // the fixture verify:delivery renders
  /^apps\/(go|python|rust|csharp|java)\/(?!.*\.md$)/, // verify:ffi compiles these against the C ABI
];

// Everything the build never reads. Documentation, the TypeScript package,
// benchmarks, demos, the workflows that consume the engine rather than
// produce it, and the scripts that only run outside the engine jobs.
export const DENY: readonly RegExp[] = [
  /^apps\//,
  /^(README(\.zh)?\.md|CLAUDE\.md|AGENTS\.md|AUTHORS|\.clang-format|\.rustfmt\.toml)$/,
  /^\.(claude|agents)\//,
  /^\.github\/\.gitignore$/,
  /^\.github\/workflows\/(checks|engine|preview|publish|refresh|benchmark|benchmark-site|perf-gate)\.yml$/,
  /^scripts\/(docs|perf|tree)\//,
  /^scripts\/ci\/(fingerprint|engine-artifacts|dispatch-engines)\.ts$/,
  /^scripts\/lib\/(perf-gate|docs-workspace)\.ts$/,
  /^scripts\/.*\.test\.ts$/,
];

// The same rules as GitHub `paths-ignore` globs, for engine.yml: a push that
// touches only these paths does not even start a run. GitHub's `*` does not
// cross a slash and `**` does, so directories whose contents are mixed are
// spelled out per file kind. The test checks this list against DENY on the
// real tree in both directions.
export const PATHS_IGNORE: readonly string[] = [
  'apps/benchmark/**', 'apps/benchmark-site/**', 'apps/docs/**', 'apps/test/**',
  'apps/demo-express/**', 'apps/demo-hello/**', 'apps/demo-genshin-card/**',
  'apps/demo-card/*.md', 'apps/demo-card/*.mjs', 'apps/demo-card/*.txt', 'apps/demo-card/*.tape',
  'apps/typescript/src/**', 'apps/typescript/test/**',
  'apps/typescript/*.md', 'apps/typescript/*.json', 'apps/typescript/*.ts',
  'apps/go/*.md', 'apps/python/*.md', 'apps/rust/*.md', 'apps/csharp/*.md', 'apps/java/*.md',
  'README.md', 'README.zh.md', 'CLAUDE.md', 'AGENTS.md', 'AUTHORS', '.clang-format', '.rustfmt.toml',
  '.claude/**', '.agents/**', '.github/.gitignore',
  '.github/workflows/checks.yml', '.github/workflows/engine.yml', '.github/workflows/preview.yml',
  '.github/workflows/publish.yml', '.github/workflows/refresh.yml', '.github/workflows/benchmark.yml',
  '.github/workflows/benchmark-site.yml', '.github/workflows/perf-gate.yml',
  'scripts/docs/**', 'scripts/perf/**', 'scripts/tree/**',
  'scripts/ci/fingerprint.ts', 'scripts/ci/engine-artifacts.ts', 'scripts/ci/dispatch-engines.ts',
  'scripts/lib/perf-gate.ts', 'scripts/lib/docs-workspace.ts',
  'scripts/**/*.test.ts',
];

export function isEngineInput(file: string): boolean {
  if (ALLOW.some((rule) => rule.test(file))) return true;
  return !DENY.some((rule) => rule.test(file));
}

/** GitHub's path-filter dialect: `*` stays inside a segment, `**` does not. */
export function globToRegExp(glob: string): RegExp {
  const source = glob.split('**').map((part) =>
    part.split('*').map((s) => s.replace(/[.+?^${}()|[\]\\]/g, '\\$&')).join('[^/]*')).join('.*');
  return new RegExp(`^${source}$`);
}

export interface Entry {
  mode: string;
  type: string;
  id: string;
  path: string;
}

/** Parse `git ls-tree -r -z` output. */
export function parseLsTree(listing: string): Entry[] {
  return listing.split('\0').filter(Boolean).map((record) => {
    const match = /^(\d{6}) (blob|commit|tree) ([0-9a-f]{40,64})\t(.+)$/s.exec(record);
    if (!match) throw new Error(`unexpected ls-tree record: ${JSON.stringify(record)}`);
    return {mode: match[1], type: match[2], id: match[3], path: match[4]};
  });
}

export interface Fingerprint {
  /** Sixteen hex characters: enough to never collide, short enough for an artifact name. */
  id: string;
  inputs: number;
  ignored: string[];
}

export function fingerprint(entries: Entry[]): Fingerprint {
  const hash = createHash('sha256').update(`schema=${SCHEMA}\n`);
  const ignored: string[] = [];
  let inputs = 0;
  for (const entry of entries) {
    if (!isEngineInput(entry.path)) {
      ignored.push(entry.path);
      continue;
    }
    hash.update(`${entry.mode} ${entry.type} ${entry.id}\t${entry.path}\n`);
    inputs++;
  }
  return {id: hash.digest('hex').slice(0, 16), inputs, ignored};
}

export async function fingerprintOf(ref = 'HEAD'): Promise<Fingerprint> {
  const {stdout} = await execa('git', ['--no-optional-locks', 'ls-tree', '-r', '-z', '--full-tree', ref],
    {cwd: resolve(), stripFinalNewline: false});
  return fingerprint(parseLsTree(stdout));
}

if (process.argv[1] && path.resolve(process.argv[1]) === import.meta.filename) {
  const cli = cac('pnpm ci:fingerprint');
  cli.command('', 'hash the tracked files the engine builds read')
    .option('--ref <rev>', 'tree to hash', {default: 'HEAD'})
    .option('--output', 'append fingerprint=<id> to $GITHUB_OUTPUT')
    .option('--explain', 'list what the hash left out')
    .option('--paths-ignore', 'print the paths-ignore entries for engine.yml and exit')
    .action(async (options: {ref: string; output?: boolean; explain?: boolean; pathsIgnore?: boolean}) => {
      try {
        if (options.pathsIgnore) {
          for (const glob of PATHS_IGNORE) console.log(`      - '${glob}'`);
          return;
        }
        const result = await fingerprintOf(options.ref);
        if (options.explain) {
          console.log(`${result.inputs} inputs, ${result.ignored.length} ignored, schema ${SCHEMA}`);
          const groups = new Map<string, number>();
          for (const file of result.ignored) {
            const key = file.split('/').slice(0, 2).join('/');
            groups.set(key, (groups.get(key) ?? 0) + 1);
          }
          for (const [key, count] of [...groups].sort()) console.log(`  ignored ${String(count).padStart(5)}  ${key}`);
        }
        console.log(result.id);
        if (options.output) {
          if (!process.env.GITHUB_OUTPUT) throw new Error('GITHUB_OUTPUT is not set');
          appendFileSync(process.env.GITHUB_OUTPUT, `fingerprint=${result.id}\n`);
        }
      } catch (error) {
        console.error(error instanceof Error ? error.message : String(error));
        process.exitCode = 1;
      }
    });
  cli.help();
  cli.parse();
}
