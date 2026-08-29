import assert from 'node:assert/strict';
import test from 'node:test';
import {packageVersions} from '../src/engines.ts';

test('records every locked benchmark tool version even when package exports hide package.json', async () => {
  const versions = await packageVersions();
  for (const name of [
    'tinybench', 'execa', 'systeminformation', 'ajv', 'pngjs', 'pixelmatch', 'wait-on', 'tsx', 'typescript',
  ]) {
    assert.match(versions[name], /^\d+\.\d+\.\d+/, `${name} version`);
  }
});
