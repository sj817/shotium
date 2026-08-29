import fs from 'node:fs';
import path from 'node:path';
import {booleanArg, parseArgs} from './args.ts';
import {createEngine} from './engines.ts';
import {inspectPng} from './image.ts';

const options = parseArgs(process.argv.slice(2));
const endpoint = options.endpoint ? JSON.parse(Buffer.from(options.endpoint, 'base64url').toString('utf8')) : null;

async function awaitStartGate(): Promise<void> {
  if (!booleanArg(options.startGated, false)) return;
  const token = await new Promise<string>((resolve, reject) => {
    let input = '';
    process.stdin.setEncoding('utf8');
    process.stdin.on('data', (chunk) => {
      input += chunk;
      const newline = input.indexOf('\n');
      if (newline >= 0) resolve(input.slice(0, newline).trim());
    });
    process.stdin.once('end', () => reject(new Error('start gate closed before release')));
    process.stdin.once('error', reject);
    process.stdin.resume();
  });
  if (token !== 'go') throw new Error(`invalid start gate token ${JSON.stringify(token)}`);
  process.stdin.pause();
}

await awaitStartGate();
const engine = await createEngine(options.engine, {
  workers: Number(options.workers) || 4,
  daemonName: options.daemonName || 'benchmark',
});
const timings: Record<string, number> = {};

async function closeWithinDeadline(): Promise<void> {
  let timer: NodeJS.Timeout | undefined;
  try {
    await Promise.race([
      engine.close(),
      new Promise((_, reject) => {
        timer = setTimeout(() => reject(new Error('engine close exceeded 10 seconds')), 10_000);
      }),
    ]);
  } finally {
    if (timer) clearTimeout(timer);
  }
}
let response: Record<string, any>;
try {
  let started = performance.now();
  if (endpoint) await engine.connect(endpoint);
  else await engine.launch();
  timings.connect_or_launch_ms = performance.now() - started;
  started = performance.now();
  const result = await engine.shot(options.url, {timeoutMs: Number(options.timeoutMs) || 30_000});
  timings.shot_ms = performance.now() - started;
  const shotCompletedEpochMs = Date.now();
  const image = inspectPng(result.image, {width: 1280, height: 720});
  if (options.evidenceFile) {
    fs.mkdirSync(path.dirname(String(options.evidenceFile)), {recursive: true});
    fs.writeFileSync(String(options.evidenceFile), result.image);
  }
  response = {ok: true, timings, image, shot_completed_epoch_ms: shotCompletedEpochMs};
} catch (error) {
  response = {ok: false, timings, error: String(error?.stack || error)};
  process.exitCode = 1;
} finally {
  const closeStarted = performance.now();
  await closeWithinDeadline().catch((error) => {
    response.ok = false;
    response.error = `${response.error || ''}\n${String(error)}`.trim();
    process.exitCode = 1;
  });
  timings.close_ms = performance.now() - closeStarted;
}
await new Promise<void>((resolve) =>
  process.stdout.write(`${JSON.stringify(response)}\n`, () => resolve()));
process.exit(response.ok ? 0 : 1);
