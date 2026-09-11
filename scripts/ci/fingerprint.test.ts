// Two things can go wrong with a content fingerprint, and they fail in
// opposite directions. Counting a documentation file is one avoidable build.
// Leaving out something the build reads is a release cut from stale bytes
// that every check passed, because the checks ran on the previous bytes.
// So the membership rules are pinned here path by path, and the
// paths-ignore list engine.yml carries is checked against them on the real
// tree, in both directions.
import assert from 'node:assert/strict';
import test from 'node:test';

import {execa} from 'execa';

import {resolve} from '../lib/repo.ts';
import {type Entry, fingerprint, globToRegExp, isEngineInput, parseLsTree, PATHS_IGNORE} from './fingerprint.ts';

test('what the build reads is an input; what it never reads is not', () => {
  for (const file of [
    'DEPS', 'BUILD.gn', '.gn', '.gitmodules', 'LICENSE', 'shot/BUILD.gn', 'shot/shot_renderer.cc', 'shot/testdata/demos/a.html',
    'build/args/shot.gn', 'third_party/blink/renderer/core/dom/document.cc', 'third_party/boringssl/src',
    'apps/typescript/native/binding.cc', 'apps/c-abi/README.md', 'apps/demo-card/card.html', 'apps/go/main.go', 'apps/rust/Cargo.lock',
    'scripts/build/shards.ts', 'scripts/ci/select-shards.ts', 'scripts/ci/stamp-mtimes.ts', 'scripts/package/platform.ts',
    'scripts/lib/release-artifacts.ts', 'scripts/verify/delivery.ts', 'scripts/package.json', 'scripts/pnpm-lock.yaml', 'package.json',
    '.github/actions/linux-source/action.yml', '.github/workflows/engine-linux.yml', '.github/workflows/check-ffi.yml',
  ]) assert.equal(isEngineInput(file), true, `${file} should be an input`);

  for (const file of [
    'README.md', 'README.zh.md', 'CLAUDE.md', 'AGENTS.md', 'AUTHORS', '.clang-format',
    'apps/typescript/src/index.ts', 'apps/typescript/test/consumer.ts', 'apps/typescript/package.json', 'apps/typescript/README.md',
    'apps/docs/release.md', 'apps/docs/benchmarks/v0.7.3/x/manifest.json', 'apps/benchmark/src/cli.ts', 'apps/benchmark-site/index.md',
    'apps/demo-card/README.md', 'apps/demo-card/card.mjs', 'apps/demo-express/README.md', 'apps/test/render/cases.json',
    'apps/go/README.md', 'apps/python/README.zh.md',
    '.claude/skills/release/SKILL.md', '.github/.gitignore',
    '.github/workflows/checks.yml', '.github/workflows/engine.yml', '.github/workflows/preview.yml', '.github/workflows/publish.yml',
    '.github/workflows/benchmark.yml', '.github/workflows/perf-gate.yml',
    'scripts/docs/hero.ts', 'scripts/perf/ci.ts', 'scripts/tree/trim-tree.ts', 'scripts/lib/perf-gate.ts',
    'scripts/ci/fingerprint.ts', 'scripts/ci/engine-artifacts.ts', 'scripts/ci/dispatch-engines.ts',
    'scripts/build/shards.test.ts', 'scripts/ci/select-shards.test.ts',
  ]) assert.equal(isEngineInput(file), false, `${file} should be ignored`);
});

const tree: Entry[] = [
  {mode: '100644', type: 'blob', id: 'a'.repeat(40), path: 'DEPS'},
  {mode: '100644', type: 'blob', id: 'b'.repeat(40), path: 'README.md'},
  {mode: '100755', type: 'blob', id: 'c'.repeat(40), path: 'shot/run.sh'},
  {mode: '160000', type: 'commit', id: 'd'.repeat(40), path: 'third_party/boringssl/src'},
];

test('the id follows the inputs and nothing else', () => {
  const base = fingerprint(tree);
  assert.match(base.id, /^[0-9a-f]{16}$/);
  assert.equal(base.inputs, 3);
  assert.deepEqual(base.ignored, ['README.md']);

  const edit = (index: number, change: Partial<Entry>) => fingerprint(tree.map((e, i) => (i === index ? {...e, ...change} : e)));
  assert.equal(edit(1, {id: 'e'.repeat(40)}).id, base.id, 'a README edit changes nothing');
  assert.notEqual(edit(0, {id: 'e'.repeat(40)}).id, base.id, 'a DEPS edit changes the id');
  assert.notEqual(edit(2, {mode: '100644'}).id, base.id, 'a mode flip changes the id');
  assert.notEqual(edit(3, {id: 'e'.repeat(40)}).id, base.id, 'a gitlink bump changes the id');
  assert.notEqual(fingerprint(tree.slice(1)).id, base.id, 'a removed input changes the id');
});

test('ls-tree records parse, with tabs and spaces in paths left alone', () => {
  const nul = '\0';
  const listing = [`100644 blob ${'1'.repeat(40)}\tDEPS`, `160000 commit ${'2'.repeat(40)}\tthird_party/x/src`,
    `100644 blob ${'3'.repeat(40)}\tshot/testdata/a b.html`].join(nul) + nul;
  assert.deepEqual(parseLsTree(listing).map((e) => [e.type, e.path]), [['blob', 'DEPS'], ['commit', 'third_party/x/src'], ['blob', 'shot/testdata/a b.html']]);
  assert.throws(() => parseLsTree('garbage' + nul), /unexpected ls-tree record/);
});

test('GitHub globs: * stays in its segment, ** does not', () => {
  assert.ok(globToRegExp('apps/docs/**').test('apps/docs/a/b.md'));
  assert.ok(!globToRegExp('apps/docs/**').test('apps/docs2/a.md'));
  assert.ok(globToRegExp('apps/typescript/*.md').test('apps/typescript/README.zh.md'));
  assert.ok(!globToRegExp('apps/typescript/*.md').test('apps/typescript/src/README.md'));
  assert.ok(globToRegExp('scripts/**/*.test.ts').test('scripts/ci/fingerprint.test.ts'));
  assert.ok(!globToRegExp('scripts/**/*.test.ts').test('scripts/ci/fingerprint.ts'));
  assert.ok(globToRegExp('README.md').test('README.md') && !globToRegExp('README.md').test('apps/README.md'));
});

// The real tree, so that a file added under an ignored directory without a
// matching glob, or a glob that has started to cover an input, shows up in
// the checks run rather than in the next release.
test('engine.yml paths-ignore and the deny rules agree on every tracked file', async () => {
  // The index rather than HEAD, so a file staged for this commit counts.
  const {stdout} = await execa('git', ['--no-optional-locks', 'ls-files', '-z', '--full-name'], {cwd: resolve(), stripFinalNewline: false});
  const files = stdout.split('\0').filter(Boolean);
  const globs = PATHS_IGNORE.map((glob) => ({glob, re: globToRegExp(glob)}));
  const coveredInputs = files.filter((file) => isEngineInput(file) && globs.some((g) => g.re.test(file)));
  assert.deepEqual(coveredInputs, [], 'paths-ignore must never match an engine input');
  const uncoveredIgnored = files.filter((file) => !isEngineInput(file) && !globs.some((g) => g.re.test(file)));
  assert.deepEqual(uncoveredIgnored, [], 'every ignored file needs a paths-ignore glob, or engine.yml starts runs for nothing');
  const idle = globs.filter((g) => !files.some((file) => g.re.test(file))).map((g) => g.glob);
  // Globs for files this PR series adds later are fine; a glob that matches
  // nothing at all otherwise is a typo.
  const pending = ['.github/workflows/engine.yml', '.github/workflows/preview.yml', '.github/workflows/refresh.yml', '.agents/**'];
  assert.deepEqual(idle.filter((glob) => !pending.includes(glob)), []);
});
