import assert from 'node:assert/strict';
import test from 'node:test';
import {balancedOrder, distribution, relativeDrift} from '../src/statistics.ts';
import {booleanArg, parseArgs, recoverNpmRunValues} from '../src/args.ts';

test('balanced order is seeded, deterministic and rotates every engine', () => {
  const engines = ['shotium', 'puppeteer', 'playwright'];
  const first = balancedOrder(engines, 0, 'release-seed');
  const second = balancedOrder(engines, 1, 'release-seed');
  assert.deepEqual(first, balancedOrder(engines, 0, 'release-seed'));
  assert.deepEqual([...first].sort(), [...engines].sort());
  assert.deepEqual(second, [...first.slice(1), first[0]]);
});

test('distribution keeps the worst value and robust statistics', () => {
  assert.deepEqual(distribution([1, 2, 3, 100]), {
    n: 4, min: 1, p50: 2.5, p95: 100, max: 100, mean: 26.5, mad: 1,
  });
  assert.equal(relativeDrift([100, 101, 99]), 0.02);
});

test('argument parser accepts inline, separate and boolean values', () => {
  assert.deepEqual(parseArgs(['--profile=smoke', '--seed', 'abc', '--commit-results']), {
    profile: 'smoke', seed: 'abc', commitResults: true,
  });
  assert.equal(booleanArg('false'), false);
  assert.throws(() => booleanArg('maybe'), /expected boolean/);
  const npmReordered = parseArgs(['--shotium-version', '0.3.2']);
  recoverNpmRunValues(npmReordered, ['shotiumVersion']);
  assert.equal(npmReordered.shotiumVersion, '0.3.2');
  process.env.npm_config_shotium_version = '0.3.3';
  try {
    const npmEquals = parseArgs([], {shotiumVersion: 'latest'});
    recoverNpmRunValues(npmEquals, ['shotiumVersion']);
    assert.equal(npmEquals.shotiumVersion, '0.3.3');
  } finally {
    delete process.env.npm_config_shotium_version;
  }
  process.env.npm_config_skip_install = 'true';
  try {
    const npmBoolean = parseArgs([]);
    recoverNpmRunValues(npmBoolean, ['skipInstall']);
    assert.equal(npmBoolean.skipInstall, true);
  } finally {
    delete process.env.npm_config_skip_install;
  }
});
