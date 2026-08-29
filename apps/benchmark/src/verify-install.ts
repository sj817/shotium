import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {currentPlatformId, FIXTURE_ROOT, VIEWPORT} from './constants.ts';
import {inspectPng} from './image.ts';

const require = createRequire(import.meta.url);

function manifest(name) {
  return JSON.parse(fs.readFileSync(require.resolve(`${name}/package.json`), 'utf8'));
}

function packageContentEvidence(manifestFile: string) {
  const root = path.dirname(manifestFile);
  const pending = [root];
  const files: string[] = [];
  while (pending.length) {
    const directory = pending.pop()!;
    for (const entry of fs.readdirSync(directory, {withFileTypes: true})) {
      const candidate = path.join(directory, entry.name);
      if (entry.isDirectory()) pending.push(candidate);
      else if (entry.isFile()) files.push(candidate);
    }
  }
  files.sort((left, right) => left.localeCompare(right));
  const digest = crypto.createHash('sha256');
  let bytes = 0;
  for (const file of files) {
    const content = fs.readFileSync(file);
    const relative = path.relative(root, file).replaceAll('\\', '/');
    const fileHash = crypto.createHash('sha256').update(content).digest('hex');
    digest.update(relative).update('\0').update(fileHash).update('\n');
    bytes += content.length;
  }
  return {content_sha256: digest.digest('hex'), files: files.length, bytes};
}

export async function verifyConsumerInstall(expectedVersion, evidenceFile: string | null = null) {
  const packageName = '@shotkit/shotium';
  const platformName = `@shotkit/shotium-${currentPlatformId()}`;
  const main = manifest(packageName);
  const platform = manifest(platformName);
  if (main.version !== expectedVersion || platform.version !== expectedVersion) {
    throw new Error(`package versions disagree: main=${main.version}, platform=${platform.version}, expected=${expectedVersion}`);
  }
  const esm = await import(packageName);
  const cjs = require(packageName);
  for (const [label, module] of [['ESM', esm], ['CommonJS', cjs]]) {
    if (typeof module.screenshot !== 'function' || typeof module.Runtime !== 'function') {
      throw new Error(`${label} consumer exports are incomplete`);
    }
  }
  let apiSmoke;
  try {
    esm.start({cacheDir: null});
    const result = await esm.screenshot({
      file: path.join(FIXTURE_ROOT, 'simple.html'),
      viewport: VIEWPORT,
      scale: 1,
      type: 'png',
      allowFileAccess: true,
      cache: 'no-store',
      pageGotoParams: {waitUntil: 'load', timeout: 30_000},
    });
    if (!result?.image) throw new Error('top-level screenshot() returned no PNG bytes');
    apiSmoke = inspectPng(result.image, VIEWPORT);
    if (evidenceFile) {
      fs.mkdirSync(path.dirname(evidenceFile), {recursive: true});
      fs.writeFileSync(evidenceFile, result.image);
    }
  } finally {
    await esm.stop().catch(() => {});
  }
  const mainManifestPath = require.resolve(`${packageName}/package.json`);
  const platformManifestPath = require.resolve(`${platformName}/package.json`);
  return {
    main: {name: packageName, version: main.version, ...packageContentEvidence(mainManifestPath)},
    platform: {name: platformName, version: platform.version,
      ...packageContentEvidence(platformManifestPath)},
    main_manifest_sha256: crypto.createHash('sha256')
        .update(fs.readFileSync(mainManifestPath)).digest('hex'),
    platform_manifest_sha256: crypto.createHash('sha256')
        .update(fs.readFileSync(platformManifestPath)).digest('hex'),
    esm: true,
    commonjs: true,
    api_smoke: apiSmoke,
  };
}
