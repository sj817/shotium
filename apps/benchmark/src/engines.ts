import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {execa} from 'execa';
import {isNativeBinary} from './binary-architecture.ts';
import {VIEWPORT, currentPlatformId} from './constants.ts';

const require = createRequire(import.meta.url);

async function importDefault(name) {
  const module = await import(name);
  return module.default || module;
}

function imageBuffer(value) {
  if (Buffer.isBuffer(value)) return value;
  if (Buffer.isBuffer(value?.image)) return value.image;
  if (value?.image instanceof Uint8Array) return Buffer.from(value.image);
  if (value instanceof Uint8Array) return Buffer.from(value);
  throw new Error('engine returned no image bytes');
}

export function competitorChromiumPolicy(platform = process.platform) {
  if (platform !== 'linux') {
    return {puppeteerArgs: [], playwrightChromiumSandbox: undefined};
  }
  // GitHub's Ubuntu runners block the user-namespace sandbox with AppArmor.
  // The benchmark only opens its own local fixtures and gives every launch an
  // isolated profile, so explicitly match Playwright's sandbox-disabled CI
  // behavior instead of letting Puppeteer fail before a sample can start.
  return {puppeteerArgs: ['--no-sandbox'], playwrightChromiumSandbox: false};
}

export class ShotiumEngine {
  name: string;
  workers: number;
  mode: string;
  daemonName: string;
  module: any;
  runtime: any;
  client: any;

  constructor({workers = 4, mode = 'runtime', daemonName = 'benchmark'} = {}) {
    this.name = mode === 'daemon' ? 'shotium-daemon' : 'shotium';
    this.workers = workers;
    this.mode = mode;
    this.daemonName = daemonName;
    this.module = null;
    this.runtime = null;
    this.client = null;
  }

  get config() {
    return this.mode === 'daemon' ?
      {name: this.daemonName, cacheDir: null, prewarm: true} :
      {cacheDir: null};
  }

  async launch() {
    this.module = await import('@shotkit/shotium');
    if (this.mode === 'daemon') {
      await this.module.daemon.start(this.config);
      this.client = await this.module.daemon.connect(this.config);
      return;
    }
    this.runtime = new this.module.Runtime();
    this.runtime.start(this.config);
  }

  async connect(endpoint) {
    this.module = await import('@shotkit/shotium');
    this.mode = 'daemon';
    this.daemonName = endpoint.daemonName;
    this.client = await this.module.daemon.connect({...this.config, failIfMissing: true});
  }

  async shot(url, {timeoutMs = 30_000, fullPage = false} = {}) {
    const request = {
      file: url,
      viewport: VIEWPORT,
      scale: 1,
      type: 'png',
      allowFileAccess: true,
      cache: 'no-store',
      fullPage,
      pageGotoParams: {waitUntil: 'load', timeout: timeoutMs},
    };
    const result = this.client ? await this.client.screenshot(request) :
      await this.runtime.screenshot(request);
    return {image: imageBuffer(result), stats: result.stats || null};
  }

  async status() {
    if (!this.module || this.mode !== 'daemon') return null;
    return this.module.daemon.status(this.config);
  }

  async close({stopDaemon = false} = {}) {
    if (this.client) {
      this.client.close();
      this.client = null;
    }
    if (this.runtime) {
      await this.runtime.stop();
      this.runtime = null;
    }
    if (stopDaemon && this.module && this.mode === 'daemon') {
      await this.module.daemon.stop(this.config);
    }
  }
}

class ChromeEngine {
  name: string;
  launchHook: (...args: any[]) => Promise<any>;
  connectHook: (...args: any[]) => Promise<any>;
  reusePage: boolean;
  browser: any;
  context: any;
  idlePages: any[];
  attached: boolean;
  profileDir: string | null;

  constructor({name, launch, connect, reusePage = false, profileDir = null}) {
    this.name = name;
    this.launchHook = launch;
    this.connectHook = connect;
    this.reusePage = reusePage;
    this.browser = null;
    this.context = null;
    this.idlePages = [];
    this.attached = false;
    this.profileDir = profileDir;
  }

  async launch() {
    ({browser: this.browser, context: this.context} = await this.launchHook());
  }

  async connect(endpoint) {
    ({browser: this.browser, context: this.context} = await this.connectHook(endpoint));
    this.attached = true;
  }

  async shot(url, {timeoutMs = 30_000, fullPage = false} = {}) {
    const page = this.reusePage && this.idlePages.length ? this.idlePages.pop() :
      await this.context.newPage();
    let cacheSession = null;
    try {
      if (!this.reusePage) {
        if (typeof page.setCacheEnabled === 'function') {
          await page.setCacheEnabled(false);
        } else if (typeof this.context.newCDPSession === 'function') {
          cacheSession = await this.context.newCDPSession(page);
          await cacheSession.send('Network.enable');
          await cacheSession.send('Network.setCacheDisabled', {cacheDisabled: true});
        }
      }
      await page.goto(url, {waitUntil: 'load', timeout: timeoutMs});
      return {image: Buffer.from(await page.screenshot({type: 'png', fullPage})), stats: null};
    } finally {
      if (cacheSession) await cacheSession.detach().catch(() => {});
      if (this.reusePage) this.idlePages.push(page);
      else await page.close().catch(() => {});
    }
  }

  async close() {
    await Promise.all(this.idlePages.splice(0).map((page) => page.close().catch(() => {})));
    if (this.context && this.context !== this.browser) {
      await this.context.close().catch(() => {});
      this.context = null;
    }
    if (this.browser) {
      if (this.attached && this.browser.disconnect) await this.browser.disconnect();
      else await this.browser.close().catch(() => {});
      this.browser = null;
    }
    if (this.profileDir) fs.rmSync(this.profileDir, {recursive: true, force: true});
  }
}

async function puppeteerDefinition(name, headless, options) {
  const puppeteer = await importDefault('puppeteer');
  const policy = competitorChromiumPolicy();
  return new ChromeEngine({
    name,
    reusePage: options.reusePage,
    profileDir: options.profileDir || null,
    launch: async () => {
      const browser = await puppeteer.launch({
        headless,
        args: policy.puppeteerArgs,
        defaultViewport: {...VIEWPORT, deviceScaleFactor: 1},
        userDataDir: options.profileDir,
      });
      return {browser, context: browser};
    },
    connect: async (endpoint) => {
      const browser = await puppeteer.connect({
        browserWSEndpoint: endpoint.wsEndpoint,
        defaultViewport: {...VIEWPORT, deviceScaleFactor: 1},
      });
      return {browser, context: browser};
    },
  });
}

async function playwrightDefinition(name, channel, options) {
  const {chromium} = await import('playwright');
  const policy = competitorChromiumPolicy();
  const newContext = (browser) => browser.newContext({viewport: VIEWPORT, deviceScaleFactor: 1});
  return new ChromeEngine({
    name,
    reusePage: options.reusePage,
    profileDir: options.profileDir || null,
    launch: async () => {
      if (options.profileDir) {
        const context = await chromium.launchPersistentContext(options.profileDir, {
          headless: true,
          channel,
          chromiumSandbox: policy.playwrightChromiumSandbox,
          viewport: VIEWPORT,
          deviceScaleFactor: 1,
        });
        return {browser: context.browser(), context};
      }
      const browser = await chromium.launch({
        headless: true, channel, chromiumSandbox: policy.playwrightChromiumSandbox,
      });
      return {browser, context: await newContext(browser)};
    },
    connect: async (endpoint) => {
      const browser = await chromium.connect(endpoint.wsEndpoint);
      return {browser, context: await newContext(browser)};
    },
  });
}

export async function createEngine(name, options = {}) {
  switch (name) {
    case 'shotium': return new ShotiumEngine(options);
    case 'shotium-daemon': return new ShotiumEngine({...options, mode: 'daemon'});
    case 'puppeteer-shell': return puppeteerDefinition(name, 'shell', options);
    case 'puppeteer-chrome': return puppeteerDefinition(name, true, options);
    case 'playwright-shell': return playwrightDefinition(name, 'chromium-headless-shell', options);
    case 'playwright-chrome': return playwrightDefinition(name, 'chromium', options);
    default: throw new Error(`unknown engine ${name}`);
  }
}

function sha256(file) {
  return crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
}

function findFile(root, predicate) {
  const queue = [root];
  while (queue.length) {
    const candidate = queue.shift();
    for (const entry of fs.readdirSync(candidate, {withFileTypes: true})) {
      const fullPath = path.join(candidate, entry.name);
      if (entry.isDirectory()) queue.push(fullPath);
      else if (predicate(fullPath)) return fullPath;
    }
  }
  return null;
}

function findFiles(root, predicate) {
  const matches = [];
  const queue = [root];
  while (queue.length) {
    const candidate = queue.shift();
    for (const entry of fs.readdirSync(candidate, {withFileTypes: true})) {
      const fullPath = path.join(candidate, entry.name);
      if (entry.isDirectory()) queue.push(fullPath);
      else if (predicate(fullPath)) matches.push(fullPath);
    }
  }
  return matches.sort();
}

function shotiumPlatformRoot() {
  const platformPackage = `@shotkit/shotium-${currentPlatformId()}`;
  return path.dirname(require.resolve(`${platformPackage}/package.json`));
}

async function executableFor(name) {
  if (name === 'shotium') {
    return findFile(shotiumPlatformRoot(), (file) => file.endsWith('.node'));
  }
  if (name.startsWith('puppeteer-')) {
    const puppeteer = await importDefault('puppeteer');
    return puppeteer.executablePath({headless: name === 'puppeteer-shell' ? 'shell' : true});
  }
  const {chromium} = await import('playwright');
  const chromiumExecutable = chromium.executablePath();
  if (name === 'playwright-chrome') return chromiumExecutable;
  const browserCache = path.dirname(path.dirname(path.dirname(chromiumExecutable)));
  return findFile(browserCache, (file) => {
    const normalized = file.replaceAll('\\', '/').toLowerCase();
    return normalized.includes('chromium_headless_shell-') &&
      (normalized.endsWith('/headless_shell') || normalized.endsWith('/headless_shell.exe'));
  });
}

function historicallyUnsupported(name, platformId) {
  if (name.startsWith('puppeteer-') && ['linux-arm64', 'win32-arm64'].includes(platformId)) {
    return 'the package has no native browser for this platform architecture';
  }
  if (name.startsWith('playwright-') && platformId === 'win32-arm64') {
    return 'the package currently supplies an x64 browser on Windows arm64';
  }
  return null;
}

function normalizedPathSegments(file) {
  return path.resolve(file).replaceAll('\\', '/').toLowerCase().split('/').filter(Boolean);
}

export function browserVersionFromLockedMetadata(name, executable, {
  puppeteerRevisions = {},
  playwrightBrowsers = [],
} = {}) {
  const segments = normalizedPathSegments(executable);
  if (name.startsWith('puppeteer-')) {
    const product = name === 'puppeteer-shell' ? 'chrome-headless-shell' : 'chrome';
    const revision = String(puppeteerRevisions[product] || '').trim();
    if (!revision || !segments.includes(product)) return null;
    return segments.some((segment) => segment.endsWith(`-${revision.toLowerCase()}`)) ? revision : null;
  }
  if (!name.startsWith('playwright-')) return null;
  const product = name === 'playwright-shell' ? 'chromium-headless-shell' : 'chromium';
  const metadata = playwrightBrowsers.find((entry) => entry?.name === product);
  const browserVersion = String(metadata?.browserVersion || '').trim();
  if (!metadata || !browserVersion) return null;
  const revisions = new Set([
    metadata.revision,
    ...Object.values(metadata.revisionOverrides || {}),
  ].map((revision) => String(revision || '').trim()).filter(Boolean));
  const directoryPrefix = product === 'chromium-headless-shell' ?
    'chromium_headless_shell-' : 'chromium-';
  const matchesCacheDirectory = segments.some((segment) =>
    [...revisions].some((revision) => segment === `${directoryPrefix}${revision.toLowerCase()}`));
  return matchesCacheDirectory ? browserVersion : null;
}

async function lockedBrowserVersion(name, executable) {
  if (name.startsWith('puppeteer-')) {
    const puppeteer: any = await import('puppeteer');
    return browserVersionFromLockedMetadata(name, executable, {
      puppeteerRevisions: puppeteer.PUPPETEER_REVISIONS,
    });
  }
  if (name.startsWith('playwright-')) {
    const playwrightRoot = path.dirname(require.resolve('playwright-core/package.json'));
    const metadata = JSON.parse(fs.readFileSync(path.join(playwrightRoot, 'browsers.json'), 'utf8'));
    return browserVersionFromLockedMetadata(name, executable, {
      playwrightBrowsers: metadata.browsers,
    });
  }
  return null;
}

export async function probeEngine(name) {
  const platform = currentPlatformId();
  let executable;
  try {
    executable = await executableFor(name);
  } catch (error) {
    const reason = historicallyUnsupported(name, platform);
    return reason ? {engine: name, status: 'n/a', reason, executable: null, architectures: []} :
      {engine: name, status: 'infra-error', reason: String(error), executable: null, architectures: []};
  }
  if (!executable || !fs.existsSync(executable)) {
    const reason = historicallyUnsupported(name, platform);
    return reason ? {engine: name, status: 'n/a', reason, executable, architectures: []} :
      {engine: name, status: name === 'shotium' ? 'fail' : 'infra-error',
        reason: 'package browser or addon is not installed', executable, architectures: []};
  }
  const binaryFiles = name === 'shotium' ? findFiles(shotiumPlatformRoot(), (file) => {
    const extension = path.extname(file).toLowerCase();
    return ['.node', '.dll', '.so', '.dylib'].includes(extension);
  }) : [executable];
  const binaries = binaryFiles.map((file) => ({
    file,
    ...isNativeBinary(file),
    sha256: sha256(file),
  }));
  const architecture = binaries.find((entry) => entry.file === executable) || binaries[0];
  if (!architecture?.architectures.length) {
    return {
      engine: name,
      status: 'fail',
      reason: 'binary format or architecture could not be identified',
      executable,
      architectures: [],
      binaries,
    };
  }
  if (binaries.some((entry) => !entry.native)) {
    const unsupported = historicallyUnsupported(name, platform);
    return {
      engine: name,
      status: unsupported ? 'n/a' : 'fail',
      reason: unsupported ||
        `binary architecture ${architecture.architectures.join(',') || 'unknown'} is not ${process.arch}`,
      executable,
      architectures: architecture.architectures,
      binaries,
    };
  }
  let binaryVersion = null;
  if (name === 'shotium') {
    binaryVersion = JSON.parse(fs.readFileSync(require.resolve('@shotkit/shotium/package.json'), 'utf8')).version;
  } else {
    const version = await execa(executable, ['--version'], {timeout: 10_000, reject: false});
    if (version.exitCode === 0) binaryVersion = (version.stdout || version.stderr).trim() || null;
    if (!binaryVersion) binaryVersion = await lockedBrowserVersion(name, executable);
  }
  if (!binaryVersion) {
    return {
      engine: name,
      status: 'infra-error',
      reason: 'native binary version could not be read',
      executable,
      architectures: architecture.architectures,
      sha256: sha256(executable),
      binary_version: null,
      binaries,
    };
  }
  return {
    engine: name,
    status: 'pass',
    reason: null,
    executable,
    architectures: architecture.architectures,
    sha256: sha256(executable),
    binary_version: binaryVersion,
    binaries,
  };
}

export async function packageVersions() {
  const names = [
    '@shotkit/shotium', 'puppeteer', 'playwright', 'tinybench', 'execa', 'systeminformation',
    'ajv', 'pngjs', 'pixelmatch', 'wait-on', 'tsx', 'typescript',
  ];
  const versions = {};
  for (const name of names) {
    try {
      let manifestFile;
      try {
        manifestFile = require.resolve(`${name}/package.json`);
      } catch {
        let directory = path.dirname(require.resolve(name));
        while (path.dirname(directory) !== directory) {
          const candidate = path.join(directory, 'package.json');
          if (fs.existsSync(candidate)) {
            const candidateManifest = JSON.parse(fs.readFileSync(candidate, 'utf8'));
            if (candidateManifest.name === name) {
              manifestFile = candidate;
              break;
            }
          }
          directory = path.dirname(directory);
        }
      }
      if (!manifestFile) throw new Error(`package manifest is unavailable for ${name}`);
      const manifest = JSON.parse(fs.readFileSync(manifestFile, 'utf8'));
      versions[name] = manifest.version;
    } catch {
      versions[name] = null;
    }
  }
  return versions;
}
