import assert from 'node:assert/strict';
import test from 'node:test';
import {createEngine} from '../src/engines.ts';

function recordingContext(requests: any[], {cdp = null as any[] | null} = {}) {
  const page: Record<string, any> = {
    async goto() {},
    async evaluate() {},
    async screenshot() {
      return Buffer.from([0x89, 0x50, 0x4e, 0x47]);
    },
    async close() {},
  };
  // A Puppeteer page disables its cache directly; a Playwright page has no such
  // method and the adapter uses a CDP session for it, which is where the window
  // bounds go as well.
  if (!cdp) page.setCacheEnabled = async () => {};
  return {
    newPage(options) {
      requests.push(options);
      return page;
    },
    async newCDPSession() {
      return {
        async send(method, params) {
          cdp!.push({method, params});
          return method === 'Browser.getWindowForTarget' ? {windowId: 7} : {};
        },
        async detach() {},
      };
    },
  };
}

// A Puppeteer page defaults to a background tab in Chrome's own headless mode,
// where it never paints; the option below is what keeps concurrent captures on
// pages that render.
test('Puppeteer captures ask for a page in its own window', async () => {
  const requests: any[] = [];
  const engine: any = await createEngine('puppeteer-chrome');
  engine.context = recordingContext(requests);
  await engine.shot('about:blank');
  assert.deepEqual(requests, [{type: 'window'}]);
});

test('Playwright captures keep the package default page model', async () => {
  const requests: any[] = [];
  const engine: any = await createEngine('playwright-chrome');
  engine.context = recordingContext(requests);
  await engine.shot('about:blank');
  assert.deepEqual(requests, [undefined]);
});

// Playwright sizes a headless window to the viewport; where Chrome's own
// headless mode keeps its tab strip and toolbar (macOS) the tab container is
// shorter, a re-attached page shrinks to it, and screenshots that grow the
// widget while copying come back tiled. The measured inset is added to the
// window on the page's CDP session before the cache is disabled on it.
test('Playwright captures grow the window by the measured headless inset', async () => {
  const requests: any[] = [];
  const cdp: any[] = [];
  const engine: any = await createEngine('playwright-chrome', {windowInset: {width: 0, height: 87}});
  engine.context = recordingContext(requests, {cdp});
  await engine.shot('about:blank');
  assert.deepEqual(requests, [undefined]);
  assert.deepEqual(cdp, [
    {method: 'Browser.getWindowForTarget', params: undefined},
    {method: 'Browser.setWindowBounds', params: {windowId: 7, bounds: {width: 1280, height: 807}}},
    {method: 'Network.enable', params: undefined},
    {method: 'Network.setCacheDisabled', params: {cacheDisabled: true}},
  ]);
});

test('A zero inset leaves the Playwright window at the package bounds', async () => {
  const cdp: any[] = [];
  const engine: any = await createEngine('playwright-chrome', {windowInset: {width: 0, height: 0}});
  engine.context = recordingContext([], {cdp});
  await engine.shot('about:blank');
  assert.deepEqual(cdp.map((call) => call.method), ['Network.enable', 'Network.setCacheDisabled']);
});

test('A reused Playwright page is fitted once, when it is created', async () => {
  const cdp: any[] = [];
  const engine: any = await createEngine('playwright-chrome',
      {reusePage: true, windowInset: {width: 0, height: 87}});
  engine.context = recordingContext([], {cdp});
  await engine.shot('about:blank');
  await engine.shot('about:blank');
  assert.deepEqual(cdp.map((call) => call.method),
      ['Browser.getWindowForTarget', 'Browser.setWindowBounds']);
});
