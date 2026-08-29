import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {pathToFileURL} from 'node:url';
import {execa} from 'execa';
import {parseArgs, booleanArg, recoverNpmRunValues} from './args.ts';
import {APP_ROOT, currentPlatformId} from './constants.ts';

const require = createRequire(import.meta.url);
const sleep = (milliseconds: number) => new Promise<void>((resolve) => setTimeout(resolve, milliseconds));

async function npmView(specifier) {
  const result = await execa('npm', ['view', specifier, 'version', '--json'], {
    cwd: APP_ROOT,
    timeout: 30_000,
    reject: false,
  });
  if (result.exitCode !== 0) throw new Error(result.stderr || `npm view ${specifier} failed`);
  const parsed = JSON.parse(result.stdout);
  return Array.isArray(parsed) ? parsed.at(-1) : parsed;
}

export async function resolveMainShotiumVersion(requested, timeoutMs = 600_000) {
  const deadline = Date.now() + timeoutMs;
  let delay = 5000;
  let lastError;
  while (Date.now() < deadline) {
    try {
      return await npmView(`@shotkit/shotium@${requested}`);
    } catch (error) {
      lastError = error;
      await sleep(Math.min(delay, Math.max(0, deadline - Date.now())));
      delay = Math.min(30_000, delay * 2);
    }
  }
  throw new Error(`npm registry did not resolve @shotkit/shotium@${requested}: ${lastError}`);
}

export async function resolveShotiumVersion(requested, timeoutMs = 600_000) {
  const deadline = Date.now() + timeoutMs;
  let delay = 5000;
  let lastError;
  while (Date.now() < deadline) {
    try {
      const version = await npmView(`@shotkit/shotium@${requested}`);
      const platformPackage = `@shotkit/shotium-${currentPlatformId()}@${version}`;
      const platformVersion = await npmView(platformPackage);
      if (platformVersion !== version) {
        throw new Error(`${platformPackage} resolved as ${platformVersion}`);
      }
      return version;
    } catch (error) {
      lastError = error;
      await sleep(Math.min(delay, Math.max(0, deadline - Date.now())));
      delay = Math.min(30_000, delay * 2);
    }
  }
  throw new Error(`npm registry did not expose @shotkit/shotium@${requested} and its platform package: ${lastError}`);
}

function installedVersion() {
  try {
    const manifest = JSON.parse(fs.readFileSync(require.resolve('@shotkit/shotium/package.json'), 'utf8'));
    return manifest.version;
  } catch {
    return null;
  }
}

export async function ensureShotium(requested, {timeoutMs = 600_000, install = true} = {}) {
  const version = await resolveShotiumVersion(requested, timeoutMs);
  if (!install || installedVersion() === version) return version;
  const result = await execa('npm', [
    'install',
    '--no-save',
    '--package-lock=false',
    '--no-audit',
    '--no-fund',
    `@shotkit/shotium@${version}`,
  ], {
    cwd: APP_ROOT,
    timeout: timeoutMs,
    reject: false,
    stdout: 'inherit',
    stderr: 'inherit',
  });
  if (result.exitCode !== 0) throw new Error(`npm install @shotkit/shotium@${version} failed`);
  if (installedVersion() !== version) {
    throw new Error(`installed Shotium is ${installedVersion() || 'missing'}, expected ${version}`);
  }
  return version;
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  const options = parseArgs(process.argv.slice(2), {shotiumVersion: 'latest', timeout: '600000'});
  recoverNpmRunValues(options, ['shotiumVersion', 'timeout', 'resolveOnly', 'mainOnly']);
  const mainOnly = booleanArg(options.mainOnly, false);
  const operation = mainOnly ?
    resolveMainShotiumVersion(options.shotiumVersion, Number(options.timeout)) :
    ensureShotium(options.shotiumVersion, {
      timeoutMs: Number(options.timeout),
      install: !booleanArg(options.resolveOnly, false),
    });
  operation.then((version) => process.stdout.write(`${version}\n`)).catch((error) => {
    process.stderr.write(`${error?.stack || error}\n`);
    process.exitCode = 1;
  });
}
