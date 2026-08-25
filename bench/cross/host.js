'use strict';

// The resident half of the `reuse` scenario: an engine that is already up when
// the measured process starts.
//
// This is the comparison the shotium daemon was written for, and it is only
// fair if the other side gets the same thing -- so puppeteer keeps a browser
// alive behind `browser.wsEndpoint()` and playwright behind
// `chromium.launchServer()`, which are those libraries' own answers to the
// same problem. Each host warms its engine with one throwaway screenshot, so
// the process being measured attaches to something that has already rendered.
//
// What each engine leaves resident is written into the endpoint file as a list
// of root pids, because that -- not the number of milliseconds -- is what the
// reuse scenario costs you while nothing is happening.
//
//   node host.js --engine puppeteer-shell --endpoint-file out/host.json
//   node host.js --engine puppeteer-shell --endpoint-file out/host.json --stop

const fs = require('fs');
const path = require('path');

const {VIEWPORT} = require('./lib/engines');

const SHOTIUM = path.join(__dirname, '..', '..', 'shotium');
const WARMUP_URL =
    new URL(`file:///${path.join(__dirname, '..', 'simple.html').replace(/\\/g, '/')}`)
        .href;

function parseArgs(argv) {
  const options = {engine: 'shotium-daemon', endpointFile: null, stop: false,
                   workers: 4, reusePage: false};
  for (let i = 0; i < argv.length; ++i) {
    const key = argv[i].replace(/^--/, '');
    if (key === 'stop') {
      options.stop = true;
      continue;
    }
    const value = argv[++i];
    switch (key) {
      case 'engine': options.engine = value; break;
      case 'endpoint-file': options.endpointFile = value; break;
      case 'workers': options.workers = Number(value); break;
      default: throw new Error(`unknown flag --${key}`);
    }
  }
  return options;
}

function shotiumConfig(options) {
  return {
    binary: process.env.SHOTIUM_BINARY,
    workers: options.workers,
    name: 'bench',
    cacheDir: null,
  };
}

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

async function startShotium(options) {
  const shotium = require(SHOTIUM);
  const config = shotiumConfig(options);
  const started = await shotium.daemon.start(config);
  // start() returns as soon as the daemon is listening, which is before its
  // workers have rendered anything. Waiting for `warm` here is what makes this
  // host comparable to the browser hosts below, which warm up explicitly.
  for (let i = 0; i < 400 && !(await shotium.daemon.status(config)).warm; ++i) {
    await sleep(50);
  }
  return {
    engine: options.engine,
    pids: [started.pid],
    endpoint: {},
    // Nothing to keep alive here: the daemon is its own process and does not
    // need this one, which is the point of it.
    detached: true,
  };
}

async function startPuppeteer(options, launchOptions) {
  const puppeteer = require('puppeteer');
  const browser = await puppeteer.launch({
    defaultViewport: {...VIEWPORT, deviceScaleFactor: 1},
    ...launchOptions,
  });
  const page = await browser.newPage();
  await page.goto(WARMUP_URL, {waitUntil: 'load'});
  await page.screenshot({type: 'png'});
  // The page stays open, and it has to: Chrome exits when its last target
  // closes, so a browser kept for other processes to attach to needs one page
  // held open or the first client to close its own page takes the browser down
  // with it -- which shows up as ECONNREFUSED on the next connect, several
  // seconds later, in a different process. That blank page is part of what a
  // resident puppeteer browser costs, and it is in the resident numbers.
  return {
    engine: options.engine,
    pids: [process.pid],
    endpoint: {wsEndpoint: browser.wsEndpoint()},
    detached: false,
    _browser: browser,
    _page: page,
  };
}

async function startPlaywright(options, launchOptions) {
  const {chromium} = require('playwright');
  const server = await chromium.launchServer({headless: true, ...launchOptions});
  const browser = await chromium.connect(server.wsEndpoint());
  const context = await browser.newContext({viewport: VIEWPORT, deviceScaleFactor: 1});
  const page = await context.newPage();
  await page.goto(WARMUP_URL, {waitUntil: 'load'});
  await page.screenshot({type: 'png'});
  await context.close();
  await browser.close();
  return {
    engine: options.engine,
    pids: [process.pid],
    endpoint: {wsEndpoint: server.wsEndpoint()},
    detached: false,
    _server: server,
  };
}

const HOSTS = {
  'shotium-daemon': (options) => startShotium(options),
  'puppeteer-shell': (options) => startPuppeteer(options, {headless: 'shell'}),
  'puppeteer-chrome': (options) => startPuppeteer(options, {headless: true}),
  'playwright-shell': (options) =>
      startPlaywright(options, {channel: 'chromium-headless-shell'}),
  'playwright-chrome': (options) => startPlaywright(options, {channel: 'chromium'}),
};

async function stop(options) {
  if (options.engine === 'shotium-daemon') {
    const shotium = require(SHOTIUM);
    const result = await shotium.daemon.stop(shotiumConfig(options));
    process.stdout.write(`${JSON.stringify(result)}\n`);
    return;
  }
  // A browser host is killed with its process tree by whoever started it;
  // there is nothing here to ask politely.
  process.stdout.write('{"stopped":false,"reason":"kill the host process"}\n');
}

async function main() {
  const options = parseArgs(process.argv.slice(2));
  if (options.stop) {
    await stop(options);
    return;
  }
  const start = HOSTS[options.engine];
  if (!start) {
    throw new Error(`unknown host engine "${options.engine}"`);
  }
  const info = await start(options);
  const {_browser, _server, _page, ...published} = info;
  if (options.endpointFile) {
    fs.mkdirSync(path.dirname(options.endpointFile), {recursive: true});
    fs.writeFileSync(options.endpointFile, `${JSON.stringify(published)}\n`);
  }
  process.stdout.write(`${JSON.stringify(published)}\n`);
  if (published.detached) {
    return;
  }
  // Stay up. The browser is a child of this process and both libraries kill it
  // when their parent exits, so the host has to outlive the measurement.
  setInterval(() => {}, 1 << 30);
}

main().catch((error) => {
  process.stderr.write(`${error && error.stack || error}\n`);
  process.exit(1);
});
