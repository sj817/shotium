import assert from 'node:assert/strict';
import test from 'node:test';
import {waitForVisualReady} from '../src/engines.ts';
import {readinessDiagnostics} from '../src/settle.ts';

test('browser visual readiness waits for fonts and two animation frames', async () => {
  const events: string[] = [];
  const page = {
    async evaluate(operation) {
      const originalDocument = (globalThis as any).document;
      const originalAnimationFrame = (globalThis as any).requestAnimationFrame;
      (globalThis as any).document = {
        fonts: {ready: Promise.resolve().then(() => events.push('fonts'))},
      };
      (globalThis as any).requestAnimationFrame = (callback) => {
        events.push('frame');
        callback(0);
      };
      try {
        await operation();
      } finally {
        if (originalDocument === undefined) delete (globalThis as any).document;
        else (globalThis as any).document = originalDocument;
        if (originalAnimationFrame === undefined) delete (globalThis as any).requestAnimationFrame;
        else (globalThis as any).requestAnimationFrame = originalAnimationFrame;
      }
    },
  };

  await waitForVisualReady(page, 100);
  assert.deepEqual(events, ['fonts', 'frame', 'frame']);
});

test('warmup variation remains an explicit diagnostic', () => {
  assert.deepEqual(readinessDiagnostics(
      [100, 200, 100], [100, 125, 150]), {
    latency_cv: 0.43301,
    rss_drift: 0.4,
    warmup_latency_stable: false,
    warmup_rss_stable: false,
  });
});
