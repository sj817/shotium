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
import {NativeRuntime, native} from '@shotkit/shotium/native';

// The default export and the named ones are the same values, which is the
// whole reason the default exists.
const _sameRuntime: typeof runtime = shotium.runtime;
const _sameScreenshot: typeof screenshot = shotium.screenshot;

const start: StartOptions = {
  binary: '/opt/shotium/shotium',
  workers: 4,
  cacheDir: null,
  args: ['--verbose'],
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

async function pool(): Promise<void> {
  const own = new Runtime();
  own.start(start);
  own.on('stderr', ({worker, line}) => void `${worker}${line}`);
  own.on('worker-error', ({error}) => void error.message);
  const image: Buffer|null = await own.screenshot(request);
  const _running: boolean = own.running;
  await own.stop();
  void image;
}

async function resident(): Promise<void> {
  const client = await daemon.connect({...start, name: 'checks', workers: 2});
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

async function inProcess(): Promise<void> {
  const engine = new NativeRuntime();
  engine.start({cacheDir: null, resourceDir: '/opt/shotium'});
  await engine.screenshot(request);
  engine.purge({releaseWorkingSet: true});
  await engine.stop();
  await native.screenshot(request);
}

export {_sameRuntime, _sameScreenshot, inProcess, pool, resident};
