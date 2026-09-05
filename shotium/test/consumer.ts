// What a consumer sees, compiled the way a consumer compiles it.
//
// It imports by package name rather than by relative path, so it goes through
// the `exports` map in package.json and reads the generated .d.ts out of
// dist/ -- which means it fails if the map is wrong, if an entry point stopped
// being reachable, or if the types no longer describe the code. A relative
// import would check none of those.
//
// Nothing here runs. It exists to be typechecked, so every line is either a
// call whose types must line up or an assertion that a type is what it claims.

import shotium, {
  Runtime,
  cache,
  daemon,
  releaseMemory,
  runtime,
  screenshot,
  screenshotTiles,
  start as startEngine,
  stop as stopEngine,
} from '@shotkit/shotium';
import type {
  CacheEntry,
  CaptureStats,
  DaemonCapability,
  DaemonStatus,
  ScreenshotOptions,
  ScreenshotResult,
  ScreenshotTilesOptions,
  ScreenshotTilesResult,
  StartOptions,
  StartResult,
} from '@shotkit/shotium';

// The default export and the named ones are the same values, which is the
// whole reason the default exists.
const _sameRuntime: typeof runtime = shotium.runtime;
const _sameScreenshot: typeof screenshot = shotium.screenshot;
const _sameScreenshotTiles: typeof screenshotTiles = shotium.screenshotTiles;
const _sameCache: typeof cache = shotium.cache;
const _sameStart: typeof startEngine = shotium.start;

const start: StartOptions = {
  cacheDir: null,
  cacheMaxBytes: 64 * 1024 * 1024,
  userAgent: 'checks/1.0',
  resourceDir: '/opt/shotium',
};

const request: ScreenshotOptions = {
  file: 'https://example.com',
  type: 'webp',
  quality: 85,
  fullPage: true,
  viewport: {width: 1280, height: 720},
  pageGotoParams: {timeout: 15000, waitUntil: 'networkidle'},
  clip: {x: 0, y: 0, width: 100, height: 100},
  allowFileAccess: false,
  cache: 'only-if-cached',
  headers: {Authorization: 'Bearer token'},
};

const tileRequest: ScreenshotTilesOptions = {
  file: 'https://example.com',
  tile: {height: 32000},
};

// retry was a supervisor's knob, and there is no supervisor: the pool that
// re-sent a request to a fresh worker is gone, and an in-process engine has
// nothing to re-send to. It is rejected rather than accepted-and-ignored,
// because a knob that does nothing is worse than no knob -- and this line is
// what says so, since removing a field from an interface is exactly the kind
// of change that compiles everywhere and quietly changes behaviour.
// @ts-expect-error retry is not a screenshot option
const _noRetry: ScreenshotOptions = {file: 'https://example.com', retry: 2};

// `workers` went with the pool in 0.2 and `purge` was renamed in 0.3. Both are
// asserted gone for the same reason as retry: an option that is accepted and
// ignored is worse than one that is refused.
// @ts-expect-error workers is not a start option
const _noWorkers: StartOptions = {workers: 4};

// The lifecycle from the README, which is the shape this package is for: the
// caller decides when Blink starts and when it goes away.
async function lifecycle(): Promise<void> {
  const own = new Runtime();
  const started: StartResult = own.start(start);
  void started.cacheDir;
  // The half worth reading: a directory that could not be opened means this
  // engine is running without a cache, correctly and silently.
  void started.cacheActive;
  void started.running;

  const result: ScreenshotResult = await own.screenshot(request);
  const image: Buffer|null = result.image;
  const stats: CaptureStats = result.stats;
  void stats.fromCache;
  void stats.timing.fetch;
  void stats.timing.setup;
  void stats.timing.wait;
  void stats.timing.lifecycle;
  void stats.timing.paint;
  void stats.timing.raster;
  void stats.finalUrl;

  const tiled: ScreenshotTilesResult =
      await own.screenshotTiles(tileRequest);
  void tiled.stats.timing.total;
  void tiled.tiles[0]?.image;
  void tiled.tiles[0]?.path;
  void tiled.tiles[0]?.x;
  void tiled.tiles[0]?.y;
  void tiled.tiles[0]?.width;
  void tiled.tiles[0]?.height;

  const _running: boolean = own.running;
  own.releaseMemory({releaseWorkingSet: true});

  // @ts-expect-error purge was renamed to releaseMemory in 0.3
  own.purge({releaseWorkingSet: true});

  await own.stop();

  // Stopping is not final. The engine comes back, and so does its cache.
  const again: StartResult = own.start();
  void again.running;
  await own.stop();
  void image;
}

// The same, on the shared singleton, with no lifecycle at all.
async function implicit(): Promise<void> {
  startEngine();
  const {image, stats} = await screenshot(request);
  void image;
  void stats.requests;
  releaseMemory();
  await stopEngine();
}

// The cache, which is reachable with no engine running at all -- that is the
// reason it is at the top level rather than under the runtime.
async function caching(): Promise<void> {
  const dir: string = cache.getDir();
  const everywhere: string[] = cache.getDirs({target: 'all'});
  const named: string = cache.getDir({target: '0123456789abcdef'});

  const files: CacheEntry[] = await cache.getFiles();
  void files[0]?.url;
  void files[0]?.lastUsedMs;

  await cache.clear();
  await cache.clear({glob: ['https://example.com/**'], target: 'all'});
  const [outcome] = await cache.clear({maxAge: 3600, maxSize: 1024 * 1024});
  void outcome?.removed;
  void outcome?.bytesBefore;

  void dir;
  void everywhere;
  void named;
}

async function resident(): Promise<void> {
  const client = await daemon.connect({...start, name: 'checks'});
  const _endpoint: string = client.endpoint;
  const _closed: boolean = client.closed;
  const status: DaemonStatus = await client.status();
  const capability: DaemonCapability|undefined = status.capabilities[0];
  void status.served;
  void status.protocolVersion;
  void status.capabilities;
  void capability;
  // The daemon returns the same shape the in-process engine does, so moving a
  // program between them is an import change and nothing else.
  const shot: ScreenshotResult = await client.screenshot(request);
  void shot.stats.timing.total;
  const tiled: ScreenshotTilesResult =
      await client.screenshotTiles(tileRequest);
  void tiled.tiles[0]?.height;
  client.close();

  const started = await daemon.start(start);
  void started.spawned;
  const seen = await daemon.status(start);
  void seen.running;
  const stopped = await daemon.stop(start);
  void stopped.endpoint;
  await daemon.screenshot({...request, daemon: start});
}

export {
  _sameCache,
  _sameRuntime,
  _sameScreenshot,
  _sameScreenshotTiles,
  _sameStart,
  caching,
  implicit,
  lifecycle,
  resident,
};
