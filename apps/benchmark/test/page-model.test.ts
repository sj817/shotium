import assert from 'node:assert/strict';
import test from 'node:test';
import {createEngine} from '../src/engines.ts';

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
