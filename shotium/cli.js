#!/usr/bin/env node
// The command line, which is a client of the resident daemon.
//
// A command-line screenshot is the case the in-process pool serves worst: the
// process exists for one render, so every invocation would pay for starting
// workers and then throw them away. So this talks to the daemon by default and
// starts one if there is not one, and `--no-daemon` is there for the caller who
// would rather pay the startup than leave a process behind.

import fs from 'node:fs';
import path from 'node:path';

import * as shotium from './index.js';
import {DEFAULT_IDLE_TIMEOUT_MS} from './lib/daemon.js';

const USAGE = `Usage:
  shotium URL_OR_PATH [options]        one screenshot, through the daemon
  shotium daemon start|status|stop [options]

Capture:
  -o, --output PATH     Where to write the image (default: screenshot.png)
  --width N             Viewport width in CSS pixels (default: 1280)
  --height N            Viewport height in CSS pixels (default: 720)
  --scale N             Device scale factor, 0.01-8 (default: 1)
  --full-page           Capture the whole document, not just the viewport
  --selector CSS        Capture the first element matching CSS
  --type TYPE           png, jpeg or webp (default: png)
  --quality N           1-100, jpeg and webp only
  --omit-background     Keep the alpha channel instead of painting white
  --wait-until WHEN     load or networkidle (default: load)
  --timeout MS          Load timeout in milliseconds (default: 30000)
  --retry N             Re-send this many times after a crash or a timeout

Daemon:
  --workers N           Worker processes (default: half the cores, at most 4)
  --binary PATH         Path to shot (default: $SHOTIUM_BINARY, then ./bin)
  --cache-dir PATH      Root of the per-worker HTTP disk caches
  --no-cache            Run the workers without a disk cache
  --idle-timeout MS     Exit after this long with nothing to do (default: ${
    DEFAULT_IDLE_TIMEOUT_MS}, 0 never)
  --name NAME           Address the daemon by name instead of by configuration
  --no-daemon           Render in this process instead of the resident pool
  --json                Print the result as JSON
  -h, --help            Show this help

A daemon is identified by its configuration, so a different binary, worker
count or cache root is a different daemon rather than a silent reuse of one
that renders with something else.`;

function fail(message) {
  process.stderr.write(`shotium: ${message}\n`);
  process.exit(2);
}

function parse(argv) {
  const options = {daemon: {}, capture: {}, json: false, useDaemon: true};
  let target = null;
  let command = 'shot';

  if (argv[0] === 'daemon') {
    command = `daemon:${argv[1] || 'status'}`;
    argv = argv.slice(2);
  }

  const number = (value, flag) => {
    const parsed = Number(value);
    if (!Number.isFinite(parsed)) {
      fail(`${flag} needs a number`);
    }
    return parsed;
  };

  for (let i = 0; i < argv.length; ++i) {
    const arg = argv[i];
    const next = () => {
      if (i + 1 >= argv.length) {
        fail(`${arg} needs a value`);
      }
      return argv[++i];
    };
    switch (arg) {
      case '-h':
      case '--help':
        process.stdout.write(`${USAGE}\n`);
        process.exit(0);
        break;
      case '-o':
      case '--output':
        options.output = next();
        break;
      case '--width':
        options.width = number(next(), arg);
        break;
      case '--height':
        options.height = number(next(), arg);
        break;
      case '--scale':
        options.capture.scale = number(next(), arg);
        break;
      case '--full-page':
        options.capture.fullPage = true;
        break;
      case '--selector':
        options.capture.selector = next();
        break;
      case '--type':
        options.capture.type = next();
        break;
      case '--quality':
        options.capture.quality = number(next(), arg);
        break;
      case '--omit-background':
        options.capture.omitBackground = true;
        break;
      case '--wait-until':
        options.waitUntil = next();
        break;
      case '--timeout':
        options.timeout = number(next(), arg);
        break;
      case '--retry':
        options.capture.retry = number(next(), arg);
        break;
      case '--workers':
        options.daemon.workers = number(next(), arg);
        break;
      case '--binary':
        options.daemon.binary = next();
        break;
      case '--cache-dir':
        options.daemon.cacheDir = next();
        break;
      case '--no-cache':
        options.daemon.cacheDir = null;
        break;
      case '--idle-timeout':
        options.daemon.idleTimeoutMs = number(next(), arg);
        break;
      case '--name':
        options.daemon.name = next();
        break;
      case '--no-daemon':
        options.useDaemon = false;
        break;
      case '--json':
        options.json = true;
        break;
      default:
        if (arg.startsWith('-')) {
          fail(`unknown option "${arg}"`);
        }
        if (target !== null) {
          fail('one target at a time');
        }
        target = arg;
    }
  }
  return {command, target, options};
}

// A local path is not a URL, and the difference decides whether the document
// may read the filesystem: pointing this command at a file is the operator
// saying it may, exactly as shot's own command line does. A URL is not that
// statement, so it does not get the permission.
//
// A Windows path has to be recognised before a scheme is looked for, because
// "D:\page.html" is a syntactically valid URL with the scheme "d" -- and
// reading it as one would quietly render the page with its own images and
// fonts locked out.
function toTarget(value) {
  const windowsDrive = /^[a-z]:[\\/]/i.test(value);
  if (!windowsDrive && /^[a-z][a-z0-9+.-]+:/i.test(value)) {
    return {file: value, allowFileAccess: value.startsWith('file:')};
  }
  return {file: path.resolve(value), allowFileAccess: true};
}

function captureOptions(target, options) {
  const request = {...toTarget(target), ...options.capture};
  if (options.width !== undefined || options.height !== undefined) {
    request.viewport = {};
    if (options.width !== undefined) {
      request.viewport.width = options.width;
    }
    if (options.height !== undefined) {
      request.viewport.height = options.height;
    }
  }
  if (options.timeout !== undefined || options.waitUntil !== undefined) {
    request.pageGotoParams = {};
    if (options.timeout !== undefined) {
      request.pageGotoParams.timeout = options.timeout;
    }
    if (options.waitUntil !== undefined) {
      request.pageGotoParams.waitUntil = options.waitUntil;
    }
  }
  return request;
}

async function runShot(target, options) {
  if (!target) {
    fail(`nothing to photograph\n\n${USAGE}`);
  }
  const output = path.resolve(options.output || 'screenshot.png');
  const request = captureOptions(target, options);
  const started = Date.now();

  let image;
  if (options.useDaemon) {
    image = await shotium.daemon.screenshot({...request, daemon: options.daemon});
  } else {
    const runtime = new shotium.Runtime();
    runtime.start(options.daemon);
    try {
      image = await runtime.screenshot(request);
    } finally {
      await runtime.stop();
    }
  }

  fs.writeFileSync(output, image);
  const result = {
    ok: true,
    output,
    bytes: image.length,
    elapsedMs: Date.now() - started,
    daemon: options.useDaemon,
  };
  process.stdout.write(
      options.json ? `${JSON.stringify(result)}\n` :
                     `${output} (${image.length} bytes, ${result.elapsedMs} ms)\n`);
}

async function runDaemon(action, options) {
  const write = (label, payload) => {
    process.stdout.write(
        options.json ? `${JSON.stringify(payload)}\n` : `${label}\n`);
  };
  switch (action) {
    case 'start': {
      const status = await shotium.daemon.start(options.daemon);
      write(
          `${status.spawned ? 'started' : 'already running'}: pid ${
              status.pid}, ${status.workers} workers, ${status.endpoint}`,
          status);
      return;
    }
    case 'status': {
      const status = await shotium.daemon.status(options.daemon);
      write(
          status.running ?
              `running: pid ${status.pid}, ${status.workers} workers, ${
                  status.served} served, up ${Math.round(status.uptimeMs / 1000)}s` :
              `not running: ${status.endpoint}`,
          status);
      return;
    }
    case 'stop': {
      const result = await shotium.daemon.stop(options.daemon);
      write(result.stopped ? 'stopped' : 'not running', result);
      return;
    }
    default:
      fail(`unknown daemon action "${action}"\n\n${USAGE}`);
  }
}

async function main() {
  const {command, target, options} = parse(process.argv.slice(2));
  if (command.startsWith('daemon:')) {
    await runDaemon(command.slice('daemon:'.length), options);
    return;
  }
  await runShot(target, options);
}

// No exit(0) on the way out: every socket is closed by here, so the process
// ends when the event loop drains, and exiting sooner can truncate a write to
// a pipe on Windows.
main().then(
    () => {},
    (error) => {
      process.stderr.write(`shotium: ${error.message || error}\n`);
      process.exit(1);
    });
