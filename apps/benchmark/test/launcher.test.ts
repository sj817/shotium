import assert from 'node:assert/strict';
import test from 'node:test';
import {parseArgs, recoverNpmRunValues} from '../src/args.ts';
import {prepareCoreArgs} from '../src/launcher.ts';

test('launcher starts the benchmark core in a fresh process after target resolution', async () => {
  const coreArgs = await prepareCoreArgs([
    '--shotium-version', '0.3.2', '--profile', 'smoke', '--skip-install', 'true',
  ]);
  const parsed = parseArgs(coreArgs);
  recoverNpmRunValues(parsed, ['shotiumVersion', 'skipInstall']);
  assert.equal(parsed.shotiumVersion, '0.3.2');
  assert.equal(parsed.skipInstall, 'true');
  assert.equal(parsed.profile, 'smoke');
  assert.equal(coreArgs.at(-1), '--skip-install=true');
});
