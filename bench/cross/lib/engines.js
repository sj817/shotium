'use strict';

const path = require('path');

// One adapter per engine, all four with the same five methods, so that the
// runner never asks what it is driving.
//
// The rules that make the comparison a comparison, applied identically to all
// of them:
//
//   * default launch configuration. No tuning flags on either side -- what a
//     caller gets from `npm install puppeteer` and from `require('@shotkit/shotium')`.
//   * one fresh page per screenshot, closed afterwards. That is the isolation
//     shot gives whether you want it or not (it builds and tears down a Page
//     per request), so holding one page open across ten documents would be
//     comparing different things. `reusePage` measures that other thing on
//     purpose, and the report carries both.
//   * waitUntil: 'load' everywhere. The corpus is static, so networkidle would
//     only add its 500ms quiet window to every engine equally.
//   * PNG, viewport only, 1280x720, deviceScaleFactor 1, returned as bytes in
//     process. Nothing writes to disk on the timed path.
//
// What cannot be equalised, and is stated rather than hidden: these engines do
// not do the same amount of work. Chrome parses and runs the page's script and
// composites it; shot has no script engine at all. On this corpus -- static
// documents, no script -- that difference is small for Chrome, but on a React
// page it is the whole product boundary, and shot photographs it blank.

const VIEWPORT = {width: 1280, height: 720};

class ShotiumEngine {
  // mode: 'pool' renders in this process, 'daemon' talks to a resident one.
  constructor(options = {}) {
    this.name = options.name || 'shotium';
    this._mode = options.mode || 'pool';
    this._workers = options.workers || 4;
    this._binary = options.binary || process.env.SHOTIUM_BINARY;
    this._daemonName = options.daemonName || 'bench';
    // By path, not by name: the package under test is the one in this
    // checkout, not whatever an npm install might have left in node_modules.
    this._shotium = require(
        options.package || path.join(__dirname, '..', '..', '..', 'shotium'));
    this._runtime = null;
    this._client = null;
  }

  get config() {
    return {
      binary: this._binary,
      workers: this._workers,
      name: this._daemonName,
      // The disk cache is off for every engine: a benchmark that measures a
      // warm HTTP cache on one side and a cold one on the other measures the
      // cache.
      cacheDir: null,
    };
  }

  async launch() {
    if (this._mode === 'daemon') {
      this._client = await this._shotium.daemon.connect(this.config);
      return;
    }
    this._runtime = new this._shotium.Runtime();
    this._runtime.start(this.config);
    // start() only spawns; the first render is what proves a worker is up.
    // Every engine here is launched to the same definition -- ready to be
    // asked -- and made warm separately by the scenario's warmup shots.
  }

  async connect() {
    this._mode = 'daemon';
    this._client = await this._shotium.daemon.connect(this.config);
  }

  async shot(url) {
    const request = {
      file: url,
      viewport: VIEWPORT,
      type: 'png',
      allowFileAccess: true,
    };
    return this._mode === 'daemon' ? this._client.screenshot(request) :
                                     this._runtime.screenshot(request);
  }

  async close() {
    if (this._client) {
      this._client.close();
      this._client = null;
    }
    if (this._runtime) {
      await this._runtime.stop();
      this._runtime = null;
    }
  }
}

// puppeteer and playwright differ in three places -- how a browser is
// launched, how a page is made, and how one is connected to -- so they are one
// class with three hooks rather than two classes of mostly the same code.
class ChromeEngine {
  constructor(options) {
    this.name = options.name;
    this._options = options;
    this._browser = null;
    this._context = null;
    this._idle = [];
    this._attached = false;
    this._reusePage = Boolean(options.reusePage);
  }

  async launch() {
    const {browser, context} = await this._options.launch();
    this._browser = browser;
    this._context = context;
  }

  async connect(endpoint) {
    const {browser, context} = await this._options.connect(endpoint);
    // The flag lives on the engine, not on the options object the hooks close
    // over: the definitions below build an engine's options with a spread, so a
    // hook writing to its own object writes to a copy nobody reads -- and the
    // engine then "closes" a browser it only attached to, taking down the
    // resident engine the next sample was going to measure.
    this._attached = true;
    this._browser = browser;
    this._context = context;
  }

  // With `reusePage`, a page goes back on the free list instead of being
  // closed, so a sequential run keeps navigating one page and a run with four
  // in flight keeps four. Sharing a single page across concurrent shots is not
  // the faster variant of this -- it is `net::ERR_ABORTED`, because the second
  // goto() cancels the first.
  async shot(url) {
    const page = this._reusePage && this._idle.length ?
        this._idle.pop() :
        await this._context.newPage();
    try {
      await page.goto(url, {waitUntil: 'load'});
      return await page.screenshot({type: 'png'});
    } finally {
      if (this._reusePage) {
        this._idle.push(page);
      } else {
        await page.close();
      }
    }
  }

  async close() {
    for (const page of this._idle.splice(0)) {
      await page.close().catch(() => {});
    }
    if (this._browser) {
      // disconnect() when this process attached to someone else's browser:
      // closing it would take the resident engine down, which is the opposite
      // of what the reuse scenario is measuring.
      if (this._attached && this._browser.disconnect) {
        await this._browser.disconnect();
      } else {
        await this._browser.close();
      }
      this._browser = null;
    }
  }
}

function puppeteerEngine(name, launchOptions) {
  const puppeteer = require('puppeteer');
  const options = {
    name,
    launch: async () => {
      const browser = await puppeteer.launch({
        defaultViewport: {...VIEWPORT, deviceScaleFactor: 1},
        ...launchOptions,
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
  };
  return options;
}

function playwrightEngine(name, launchOptions) {
  const {chromium} = require('playwright');
  const newContext = (browser) => browser.newContext({
    viewport: VIEWPORT,
    deviceScaleFactor: 1,
  });
  const options = {
    name,
    launch: async () => {
      const browser = await chromium.launch({headless: true, ...launchOptions});
      return {browser, context: await newContext(browser)};
    },
    connect: async (endpoint) => {
      // connect(), not connectOverCDP(): the host runs launchServer(), so the
      // endpoint speaks playwright's own protocol. Attaching over CDP would
      // measure a translation layer that a playwright user never pays for.
      const browser = await chromium.connect(endpoint.wsEndpoint);
      return {browser, context: await newContext(browser)};
    },
  };
  return options;
}

// Playwright's own browser server, which is how playwright keeps a browser
// alive for other processes: launchServer() returns a ws endpoint that
// chromium.connect() attaches to, with no CDP translation in between.
function playwrightServerLauncher(launchOptions) {
  const {chromium} = require('playwright');
  return chromium.launchServer({headless: true, ...launchOptions});
}

const DEFINITIONS = {
  // Four processes' worth of shotium.exe, which is what a caller asking for four
  // concurrent screenshots gets.
  'shotium': (options) => new ShotiumEngine({...options, name: 'shotium'}),
  'shotium-daemon': (options) =>
      new ShotiumEngine({...options, name: 'shotium-daemon', mode: 'daemon'}),
  // chrome-headless-shell: the small, old headless binary, and the fastest
  // thing puppeteer can be asked for.
  'puppeteer-shell': (options) => new ChromeEngine(
      {...puppeteerEngine('puppeteer-shell', {headless: 'shell'}), ...options}),
  // The default since Puppeteer 22: real Chrome in headless mode.
  'puppeteer-chrome': (options) => new ChromeEngine(
      {...puppeteerEngine('puppeteer-chrome', {headless: true}), ...options}),
  'playwright-shell': (options) => new ChromeEngine({
    ...playwrightEngine('playwright-shell', {channel: 'chromium-headless-shell'}),
    ...options,
  }),
  // channel: 'chromium' is what selects full chrome.exe in playwright. Its
  // default -- and its 'chromium-headless-shell' channel -- both run
  // chrome-headless-shell.exe, checked by looking at the processes rather than
  // by reading a changelog.
  'playwright-chrome': (options) => new ChromeEngine(
      {...playwrightEngine('playwright-chrome', {channel: 'chromium'}), ...options}),
};

function createEngine(name, options = {}) {
  const factory = DEFINITIONS[name];
  if (!factory) {
    throw new Error(`unknown engine "${name}"`);
  }
  return factory(options);
}

function engineNames() {
  return Object.keys(DEFINITIONS);
}

module.exports = {
  VIEWPORT,
  createEngine,
  engineNames,
  playwrightServerLauncher,
};
