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
  daemon,
  runtime,
  screenshot,
} from '@shotkit/shotium';
import type {
  DaemonStatus,
  ScreenshotOptions,
  StartOptions,
} from '@shotkit/shotium';

// The default export and the named ones are the same values, which is the
// whole reason the default exists.
const _sameRuntime: typeof runtime = shotium.runtime;
const _sameScreenshot: typeof screenshot = shotium.screenshot;

const start: StartOptions = {
  cacheDir: null,
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
  retry: 2,
};

// The lifecycle from the README, which is the shape this package is for: the
// caller decides when Blink starts and when it goes away.
async function lifecycle(): Promise<void> {
  const own = new Runtime();
  own.start(start);
  const image: Buffer|null = await own.screenshot(request);
  const _running: boolean = own.running;
  own.purge({releaseWorkingSet: true});
  await own.stop();
  void image;
}

// The same, on the shared singleton, with no lifecycle at all.
async function implicit(): Promise<void> {
  runtime.start();
  await screenshot(request);
  await runtime.stop();
}

async function resident(): Promise<void> {
  const client = await daemon.connect({...start, name: 'checks'});
  const _endpoint: string = client.endpoint;
  const _closed: boolean = client.closed;
  const status: DaemonStatus = await client.status();
  void status.served;
  await client.screenshot(request);
  client.close();

  const started = await daemon.start(start);
  void started.spawned;
  const seen = await daemon.status(start);
  void seen.running;
  const stopped = await daemon.stop(start);
  void stopped.endpoint;
  await daemon.screenshot({...request, daemon: start});
}

export {_sameRuntime, _sameScreenshot, implicit, lifecycle, resident};
