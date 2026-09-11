import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {cpSync, mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync} from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {test} from 'node:test';

import {acceptanceMode, assessAcceptance, snapshotRuntime, type AcceptanceResult, type RuntimeSnapshot} from '../perf/acceptance.ts';
import {report} from '../perf/report.ts';

function installation(directory: string): void {
  const platform = `${process.platform}-${process.arch}`;
  const native = path.join(directory, 'node_modules/@pixel.js', `shotium-${platform}`);
  mkdirSync(path.join(directory, 'dist'), {recursive: true});
  mkdirSync(native, {recursive: true});
  writeFileSync(path.join(directory, 'package.json'), JSON.stringify({
    name: '@pixel.js/shotium', version: '0.7.4', optionalDependencies: {[`@pixel.js/shotium-${platform}`]: '0.7.4'},
  }));
  writeFileSync(path.join(directory, 'dist/index.js'), 'export const version = "0.7.4";');
  writeFileSync(path.join(directory, 'dist/worker.js'), 'process.on("message", () => {});');
  writeFileSync(path.join(native, 'package.json'), JSON.stringify({name: `@pixel.js/shotium-${platform}`, version: '0.7.4'}));
  for (const file of ['shotium.node', 'shotium_data.pak', 'shotium_strings.pak']) writeFileSync(path.join(native, file), file);
}

function result(snapshot: RuntimeSnapshot): AcceptanceResult {
  const file = (name: string) => snapshot.files.find((f) => f.path === name)!.sha256;
  const metadata = {
    packageVersion: snapshot.packageVersion, addon: path.join(snapshot.platformDirectory, 'shotium.node'),
    library: path.join(snapshot.platformDirectory, 'shotium.node'), resourceDirectory: snapshot.platformDirectory,
    addonSha256: file('platform/shotium.node'), librarySha256: file('platform/shotium.node'), bundleSha256: file('package/dist/index.js'),
  };
  return {
    acceptanceMode: 'identical-runtime', platform: process.platform, arch: process.arch, complete: true, shard: 'all',
    requiredCases: ['card', 'startup'], sampling: {minimumPairs: 20},
    cases: ['card', 'startup'].map((name) => ({
      name, class: 'engine', status: 'equivalent', accepted: false, metrics: {wall: {samples: 20}},
      summary: {baseline: {p50: 1, p95: 2, mean: 1.1}, candidate: {p50: 1, p95: 2, mean: 1.1}},
    })),
    metadata: {baseline: metadata, candidate: metadata},
    runtimeIdentity: {baseline: {before: snapshot, after: snapshot}, candidate: {before: snapshot, after: snapshot}},
  };
}

test('release identity acceptance is explicit; the default still requires improvement', () => {
  assert.equal(acceptanceMode(undefined), 'improvement');
  assert.equal(acceptanceMode('identical-runtime'), 'identical-runtime');
  assert.throws(() => acceptanceMode('non-regression'), /acceptance/);
});

test('runtime snapshots include all bundles, resources and manifests, independent of installation path', () => {
  const temporary = mkdtempSync(path.join(os.tmpdir(), 'shot-perf-identity-'));
  try {
    const baseline = path.join(temporary, 'baseline'), candidate = path.join(temporary, 'candidate');
    installation(baseline);
    cpSync(baseline, candidate, {recursive: true});
    const original = snapshotRuntime(baseline);
    assert.equal(snapshotRuntime(candidate).sha256, original.sha256);
    for (const relative of [
      'dist/worker.js', 'package.json',
      `node_modules/@pixel.js/shotium-${process.platform}-${process.arch}/shotium_data.pak`,
      `node_modules/@pixel.js/shotium-${process.platform}-${process.arch}/shotium.node`,
    ]) {
      const file = path.join(candidate, relative), bytes = readFileSync(file);
      writeFileSync(file, Buffer.concat([bytes, Buffer.from(' ')]));
      assert.notEqual(snapshotRuntime(candidate).sha256, original.sha256, relative);
      writeFileSync(file, bytes);
    }
    writeFileSync(path.join(candidate, 'dist/new-chunk.js'), 'new runtime code');
    assert.notEqual(snapshotRuntime(candidate).sha256, original.sha256);
  } finally {
    rmSync(temporary, {recursive: true, force: true});
  }
});

test('same runtime accepts a complete tie or uncertainty without relabelling timing verdicts', () => {
  const temporary = mkdtempSync(path.join(os.tmpdir(), 'shot-perf-policy-'));
  try {
    installation(temporary);
    const data = result(snapshotRuntime(temporary));
    data.cases[1].status = 'unproven';
    const before = structuredClone(data.cases);
    assert.deepEqual(assessAcceptance(data, 'identical-runtime'), []);
    assert.deepEqual(data.cases, before);
    assert.ok(assessAcceptance(data, 'improvement').length);
    data.acceptanceMode = 'improvement';
    assert.ok(assessAcceptance(data, 'improvement').length);
    for (const item of data.cases) { item.status = 'faster'; item.accepted = true; }
    assert.deepEqual(assessAcceptance(data, 'improvement'), []);
  } finally {
    rmSync(temporary, {recursive: true, force: true});
  }
});

test('release acceptance rejects changed runtime, missing evidence, incomplete sampling and slower cases', () => {
  const temporary = mkdtempSync(path.join(os.tmpdir(), 'shot-perf-rejections-'));
  try {
    installation(temporary);
    const valid = result(snapshotRuntime(temporary));
    const changes: Array<[string, (data: AcceptanceResult) => void]> = [
      ['missing identity', (d) => { delete d.runtimeIdentity; }],
      ['missing final snapshot', (d) => { delete d.runtimeIdentity!.candidate!.after; }],
      ['changed bytes', (d) => { d.runtimeIdentity!.candidate!.after!.files[0].sha256 = '0'.repeat(64); }],
      ['missing resource', (d) => { d.runtimeIdentity!.candidate!.before!.files = d.runtimeIdentity!.candidate!.before!.files.filter((f) => !f.path.endsWith('.pak')); }],
      ['unbound native addon', (d) => { d.metadata!.candidate!.addon = path.join(temporary, 'other.node'); }],
      ['unbound resources', (d) => { d.metadata!.candidate!.resourceDirectory = path.join(temporary, 'other'); }],
      ['unbound bundle', (d) => { d.metadata!.candidate!.bundleSha256 = '0'.repeat(64); }],
      ['wrong platform', (d) => { d.arch = 'unexpected'; }],
      ['missing metadata', (d) => { delete d.metadata!.candidate; }],
      ['missing case', (d) => { d.cases.pop(); }],
      ['duplicate case', (d) => { d.cases[1] = d.cases[0]; }],
      ['empty matrix', (d) => { d.requiredCases = []; d.cases = []; }],
      ['shard', (d) => { d.shard = 'render'; }],
      ['filtered run', (d) => { d.complete = false; }],
      ['too few pairs', (d) => { d.cases[0].metrics!.wall!.samples = 19; }],
      ['missing measurements', (d) => { delete d.cases[0].metrics; }],
      ['missing timings', (d) => { delete d.cases[0].summary; }],
      ['runtime error', (d) => { d.cases[0].status = 'error'; }],
      ['measured regression', (d) => { d.cases[0].status = 'slower'; }],
    ];
    for (const [name, change] of changes) {
      const data = structuredClone(valid);
      change(data);
      assert.ok(assessAcceptance(data, 'identical-runtime').length, name);
    }
  } finally {
    rmSync(temporary, {recursive: true, force: true});
  }
});

test('the final report requires bound pixel evidence, provenance and every requested platform', () => {
  const temporary = mkdtempSync(path.join(os.tmpdir(), 'shot-perf-report-'));
  try {
    installation(temporary);
    const data = {
      ...result(snapshotRuntime(temporary)), revision: 'a'.repeat(40), sourceDiffSha256: 'b'.repeat(64),
      harnessSha256: 'c'.repeat(64), fixtureManifestSha256: 'd'.repeat(64),
    };
    const file = path.join(temporary, 'result.json'), pixels = path.join(temporary, 'result.pixels.json');
    const output = path.join(temporary, 'report.md'), platforms = new Set([`${process.platform}-${process.arch}`]);
    const save = () => {
      writeFileSync(file, JSON.stringify(data));
      writeFileSync(pixels, JSON.stringify({status: 'pass', result_sha256: createHash('sha256').update(readFileSync(file)).digest('hex')}));
    };
    save();
    assert.equal(report([file], output, platforms, 'identical-runtime'), true);
    assert.match(readFileSync(output, 'utf8'), /通过此验收不表示性能提升/);
    assert.equal(report([file], output, platforms), false);
    assert.equal(report([file], output, new Set([...platforms, 'missing-platform']), 'identical-runtime'), false);
    assert.throws(() => report([file, file], output, platforms, 'identical-runtime'), /Duplicate/);
    writeFileSync(pixels, JSON.stringify({status: 'pass', result_sha256: '0'.repeat(64)}));
    assert.equal(report([file], output, platforms, 'identical-runtime'), false);
    rmSync(pixels);
    assert.equal(report([file], output, platforms, 'identical-runtime'), false);
    data.revision = '';
    save();
    assert.equal(report([file], output, platforms, 'identical-runtime'), false);
  } finally {
    rmSync(temporary, {recursive: true, force: true});
  }
});
