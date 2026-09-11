import assert from 'node:assert/strict';
import test from 'node:test';
import {PLAYWRIGHT_CONTEXT_OPTIONS, createEngine, playwrightLaunchArgs} from '../src/engines.ts';

function recordingContext(requests: any[]) {
  return {
    newPage(options) {
      requests.push(options);
      return {
        async setCacheEnabled() {},
        async goto() {},
        async evaluate() {},
        async screenshot() {
          return Buffer.from([0x89, 0x50, 0x4e, 0x47]);
        },
        async close() {},
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

// Playwright pages take the viewport from their window: a context viewport
// makes Playwright size every page's headless window to the viewport as if it
// had no chrome, the tab container comes out smaller, and on macOS a page
// re-attached after a sibling tab closes shrinks to it, so the next capture
// resizes the widget while copying and can come back tiled. The window is
// opened at the viewport plus the measured inset instead, and no page is
// emulated or resized.
test('Playwright windows are opened at the viewport plus the measured inset', () => {
  assert.deepEqual(playwrightLaunchArgs({width: 0, height: 87}),
      ['--window-size=1280,807', '--force-device-scale-factor=1']);
  assert.deepEqual(playwrightLaunchArgs({width: 16, height: 95}),
      ['--window-size=1296,815', '--force-device-scale-factor=1']);
  assert.deepEqual(playwrightLaunchArgs(null),
      ['--window-size=1280,720', '--force-device-scale-factor=1']);
  assert.deepEqual(PLAYWRIGHT_CONTEXT_OPTIONS, {viewport: null});
});
