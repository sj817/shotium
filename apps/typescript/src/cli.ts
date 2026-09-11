#!/usr/bin/env node
// The command line, through the Node addon: `npx @pixel.js/shotium`.
//
// This is the fallback, not the CLI. The CLI is the standalone executable in
// the shotium-cli-<platform> release archive; it is one process with no
// runtime in front of it, and it is what to install for anything that runs
// often. What this file gives a machine that has Node and this package and
// nothing else is the same flags -- the parser mirrors shot/shot_options.cc,
// name for name and default for default -- routed through `screenshot()`,
// which costs a Node start-up and an addon load per invocation that the
// executable does not pay. `--serve` is the one thing left out: the resident
// worker is the executable's job, and the Node side has `daemon` for the same
// need.
//
// Nothing here validates a value beyond turning it into a number. The engine
// rejects an out-of-range option before it initialises, with the same message
// the API gives, and repeating those bounds here would be a second copy of
// them.

import {mkdtempSync, readFileSync, rmSync, writeFileSync} from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {parseArgs} from 'node:util';

import {screenshot, screenshotTiles, start, stop} from './index.js';
import type {ScreenshotOptions, StartOptions} from './types.js';

const COMMAND = 'npx @pixel.js/shotium';

const USAGE = `Usage:
  ${COMMAND} URL_OR_PATH [options]
  ${COMMAND} --file PATH [options]
  ${COMMAND} --stdin [options]

Options:
  --width N             Viewport width in CSS pixels (default: 1280)
  --height N            Viewport height in CSS pixels (default: 720)
  --scale N             Device scale factor, 0.01-8 (default: 1)
  --full-page           Capture the whole document, not just the viewport
  --selector CSS        Capture only the first element matching CSS
  --tile-height N       Write the capture as tiles of at most N CSS pixels
                        each, numbered into --output: page-{n}.png, or
                        page-1.png, page-2.png ... when {n} is not given
  --type TYPE           png, jpeg or webp (default: png)
  --quality N           1-100, jpeg and webp only (default: 90)
  --omit-background     Keep the alpha channel instead of painting white
  --wait-until WHEN     load or networkidle (default: load)
  --output PATH, -o PATH
                        Output path (default: screenshot.png)
  --timeout-ms N        Load timeout in milliseconds (default: 30000)
  --timeout N           Alias for --timeout-ms
  --cache-dir PATH      HTTP disk cache directory; without it nothing is cached
  --cache-max-bytes N   Ceiling on that directory; 0 (the default) lets the
                        backend size itself from the volume's free space
  --user-agent STRING   Override the User-Agent sent and reported
  --allow-file-access   Accepted for parity with the executable; the command
                        line already lets the document read file: subresources
  --verbose             Accepted for parity; per-request logging is not
                        available through the addon
  --help, -h            Show this help

http, https and file URLs are accepted, as are local paths.

This is the fallback for machines that have Node and this package and nothing
else. It runs the same engine through the Node addon, which costs a Node
start-up and an addon load on every invocation. For anything that runs often,
or for --serve, use the standalone executable from the release archive
shotium-cli-<platform>.7z: https://github.com/sj817/shotium/releases
`;

// A usage mistake, as opposed to a capture that failed.
class UsageError extends Error {}

function integer(name: string, value: string|undefined): number|undefined {
  if (value === undefined) return undefined;
  if (!/^-?\d+$/.test(value)) throw new UsageError(`--${name} expects an integer, got '${value}'`);
  return Number(value);
}

function decimal(name: string, value: string|undefined): number|undefined {
  if (value === undefined) return undefined;
  const parsed = Number(value);
  if (value.trim() === '' || Number.isNaN(parsed)) throw new UsageError(`--${name} expects a number, got '${value}'`);
  return parsed;
}

interface Parsed {
  input: {kind: 'file'; file: string}|{kind: 'stdin'};
  output: string;
  tileHeight: number|undefined;
  options: Omit<ScreenshotOptions, 'file'|'path'>;
  engine: StartOptions;
}

// Every flag the executable takes, with the same name. Values are strings
// here and numbers below, because parseArgs knows only the two kinds.
const OPTIONS = {
  'help': {type: 'boolean', short: 'h'},
  'stdin': {type: 'boolean'},
  'serve': {type: 'boolean'},
  'allow-file-access': {type: 'boolean'},
  'full-page': {type: 'boolean'},
  'omit-background': {type: 'boolean'},
  'verbose': {type: 'boolean'},
  'file': {type: 'string'},
  'width': {type: 'string'},
  'height': {type: 'string'},
  'scale': {type: 'string'},
  'selector': {type: 'string'},
  'tile-height': {type: 'string'},
  'type': {type: 'string'},
  'quality': {type: 'string'},
  'wait-until': {type: 'string'},
  'output': {type: 'string', short: 'o'},
  'timeout-ms': {type: 'string'},
  'timeout': {type: 'string'},
  'cache-dir': {type: 'string'},
  'cache-max-bytes': {type: 'string'},
  'user-agent': {type: 'string'},
} as const;

function tokens(argv: string[]) {
  return parseArgs({args: argv, allowPositionals: true, options: OPTIONS});
}

function parse(argv: string[]): Parsed|'help' {
  let parsed: ReturnType<typeof tokens>;
  try {
    parsed = tokens(argv);
  } catch (error) {
    throw new UsageError(error instanceof Error ? error.message : String(error));
  }
  const {values, positionals} = parsed;
  if (values.help) return 'help';
  if (values.serve) {
    throw new UsageError(
        '--serve is not available through the addon: the resident worker is the standalone executable, ' +
        'shotium-cli-<platform>.7z on https://github.com/sj817/shotium/releases; from Node, use the daemon API instead');
  }

  const sources = [positionals.length > 0, values.file !== undefined, values.stdin === true].filter(Boolean).length;
  if (positionals.length > 1) throw new UsageError(`one input at a time; got ${positionals.length} positional arguments`);
  if (sources > 1) throw new UsageError('give the input once: a positional URL_OR_PATH, --file, or --stdin');
  if (sources === 0) throw new UsageError('no input: give a URL_OR_PATH, --file PATH, or --stdin');

  const type = values.type as ScreenshotOptions['type'];
  const waitUntil = values['wait-until'] as NonNullable<ScreenshotOptions['pageGotoParams']>['waitUntil'];
  const timeout = integer('timeout-ms', values['timeout-ms']) ?? integer('timeout', values.timeout);

  const options: Parsed['options'] = {
    // The command line is the caller, and the caller decided: a document it
    // named may read the files beside it. Same as the executable.
    allowFileAccess: true,
  };
  if (type !== undefined) options.type = type;
  if (values['full-page']) options.fullPage = true;
  if (values.selector !== undefined) options.selector = values.selector;
  const quality = integer('quality', values.quality);
  if (quality !== undefined) options.quality = quality;
  const scale = decimal('scale', values.scale);
  if (scale !== undefined) options.scale = scale;
  if (values['omit-background']) options.omitBackground = true;
  const width = integer('width', values.width);
  const height = integer('height', values.height);
  if (width !== undefined || height !== undefined) {
    options.viewport = {};
    if (width !== undefined) options.viewport.width = width;
    if (height !== undefined) options.viewport.height = height;
  }
  if (timeout !== undefined || waitUntil !== undefined) {
    options.pageGotoParams = {};
    if (timeout !== undefined) options.pageGotoParams.timeout = timeout;
    if (waitUntil !== undefined) options.pageGotoParams.waitUntil = waitUntil;
  }

  // The executable caches nothing unless told where; the API defaults to a
  // directory under the temporary directory. The command line follows the
  // executable, so a script that swaps one for the other sees the same disk.
  const engine: StartOptions = {cacheDir: values['cache-dir'] ?? null};
  const cacheMaxBytes = integer('cache-max-bytes', values['cache-max-bytes']);
  if (cacheMaxBytes !== undefined) engine.cacheMaxBytes = cacheMaxBytes;
  if (values['user-agent'] !== undefined) engine.userAgent = values['user-agent'];

  return {
    input: values.stdin ? {kind: 'stdin'} : {kind: 'file', file: values.file ?? positionals[0]!},
    output: values.output ?? 'screenshot.png',
    tileHeight: integer('tile-height', values['tile-height']),
    options,
    engine,
  };
}

// page.png -> page-{n}.png, the executable's rule for a --tile-height output
// that did not say where the number goes.
function numbered(output: string): string {
  if (output.includes('{n}')) return output;
  const extension = path.extname(output);
  return `${output.slice(0, output.length - extension.length)}-{n}${extension}`;
}

async function run(parsed: Parsed): Promise<void> {
  // Stdin becomes a file: the engine loads by URL or path, and a data: URL is
  // rejected on purpose (see lib/daemon.ts). The directory is private to this
  // run and removed with it.
  let scratch: string|null = null;
  let file: string;
  if (parsed.input.kind === 'stdin') {
    scratch = mkdtempSync(path.join(os.tmpdir(), 'shotium-cli-'));
    file = path.join(scratch, 'input.html');
    writeFileSync(file, readFileSync(0));
  } else {
    file = parsed.input.file;
  }
  try {
    start(parsed.engine);
    if (parsed.tileHeight !== undefined) {
      await screenshotTiles({...parsed.options, file, path: numbered(parsed.output), tile: {height: parsed.tileHeight}});
    } else {
      await screenshot({...parsed.options, file, path: parsed.output});
    }
  } finally {
    try {
      await stop();
    } finally {
      if (scratch) rmSync(scratch, {recursive: true, force: true});
    }
  }
}

async function main(argv: string[]): Promise<number> {
  let parsed: Parsed|'help';
  try {
    parsed = parse(argv);
  } catch (error) {
    if (error instanceof UsageError) {
      process.stderr.write(`${COMMAND}: ${error.message}\n\n${USAGE}`);
      return 2;
    }
    throw error;
  }
  if (parsed === 'help') {
    process.stdout.write(USAGE);
    return 0;
  }
  try {
    await run(parsed);
    return 0;
  } catch (error) {
    process.stderr.write(`${COMMAND}: ${error instanceof Error ? error.message : String(error)}\n`);
    return 1;
  }
}

process.exitCode = await main(process.argv.slice(2));
