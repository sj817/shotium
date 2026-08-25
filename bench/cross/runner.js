'use strict';

// One engine, one scenario, one process.
//
// Everything a scenario measures inside itself is here; everything measured
// from outside -- wall time of the whole process tree, memory, process and
// thread counts -- belongs to run.ps1, which starts this and watches it. The
// split is deliberate: a process cannot honestly measure its own startup, and
// a sampler outside cannot see which millisecond belonged to which page.
//
// Marks are epoch milliseconds so that the sampler's timeline and this
// process's phases can be laid over each other afterwards.
//
//   node runner.js --engine puppeteer-shell --scenario warm --iterations 10

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const {createEngine} = require('./lib/engines');

const CASES = path.join(__dirname, '..', 'cases.json');

function parseArgs(argv) {
  const options = {
    engine: 'shotium',
    scenario: 'cold',
    iterations: 10,
    warmup: 3,
    concurrency: 4,
    settleMs: 1000,
    reusePage: false,
    endpointFile: null,
    sampleDir: null,
  };
  for (let i = 0; i < argv.length; ++i) {
    const key = argv[i].replace(/^--/, '');
    if (key === 'reuse-page') {
      options.reusePage = true;
      continue;
    }
    const value = argv[++i];
    switch (key) {
      case 'engine': options.engine = value; break;
      case 'scenario': options.scenario = value; break;
      case 'iterations': options.iterations = Number(value); break;
      case 'warmup': options.warmup = Number(value); break;
      case 'concurrency': options.concurrency = Number(value); break;
      case 'settle-ms': options.settleMs = Number(value); break;
      case 'endpoint-file': options.endpointFile = value; break;
      case 'sample-dir': options.sampleDir = value; break;
      default: throw new Error(`unknown flag --${key}`);
    }
  }
  return options;
}

// The local half of bench/cases.json. The loopback case is left out on
// purpose: it measures a PowerShell fixture server as much as it measures an
// engine, and it is the one case where the four engines do not even share a
// network stack.
function loadCases() {
  const definitions = JSON.parse(fs.readFileSync(CASES, 'utf8'));
  return definitions.filter((c) => !c.loopback_http).map((c) => ({
    name: c.name,
    url: new URL(`file:///${path.join(__dirname, '..', c.file).replace(/\\/g, '/')}`)
             .href,
  }));
}

const now = () => Number(process.hrtime.bigint() / 1000n) / 1000;
const epoch = () => Date.now();
const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

function describe(image) {
  return {
    bytes: image.length,
    width: image.readUInt32BE(16),
    height: image.readUInt32BE(20),
    sha256: crypto.createHash('sha256').update(image).digest('hex'),
  };
}

async function main() {
  const options = parseArgs(process.argv.slice(2));
  const cases = loadCases();
  const marks = [];
  const mark = (name) => marks.push({name, at: epoch()});
  const shots = [];

  mark('process-start');
  const requireStarted = now();
  const engine = createEngine(options.engine, {reusePage: options.reusePage});
  const requireMs = now() - requireStarted;
  mark('required');

  const written = new Set();
  const record = async (label, url) => {
    const started = now();
    const image = await engine.shot(url);
    const ms = now() - started;
    shots.push({case: label, ms, ...describe(image)});
    // One copy of each case per engine, off the timed path, so that "faster"
    // can be checked against "and it drew the page". A number nobody looked at
    // a picture for is not a benchmark result.
    if (options.sampleDir && !written.has(label)) {
      written.add(label);
      fs.mkdirSync(options.sampleDir, {recursive: true});
      fs.writeFileSync(
          path.join(options.sampleDir, `${options.engine}.${label}.png`), image);
    }
    return image;
  };

  const timings = {require_ms: requireMs};

  if (options.scenario === 'reuse') {
    // The scenario the daemon exists for: a fresh process, an engine that is
    // already up, and nothing to pay but attaching to it.
    const endpoint = options.endpointFile ?
        JSON.parse(fs.readFileSync(options.endpointFile, 'utf8')) :
        {};
    const connectStarted = now();
    await engine.connect(endpoint.endpoint || {});
    timings.connect_ms = now() - connectStarted;
    mark('connected');
    await record(cases[0].name, cases[0].url);
    timings.first_shot_ms = shots[0].ms;
    mark('done');
    await engine.close();
  } else {
    const launchStarted = now();
    await engine.launch();
    timings.launch_ms = now() - launchStarted;
    mark('launched');

    if (options.scenario === 'cold') {
      await record(cases[0].name, cases[0].url);
      timings.first_shot_ms = shots[0].ms;
    } else if (options.scenario === 'cold-settled') {
      // The same first screenshot, taken a second later. Whatever an engine
      // finishes initialising on its own time shows up as the gap between this
      // and `cold`.
      const settleStarted = now();
      await sleep(options.settleMs);
      timings.settle_ms = now() - settleStarted;
      mark('settled');
      await record(cases[0].name, cases[0].url);
      timings.first_shot_ms = shots[0].ms;
    } else if (options.scenario === 'warm') {
      for (let i = 0; i < options.warmup; ++i) {
        await engine.shot(cases[0].url);
      }
      mark('warm');
      const started = now();
      for (let i = 0; i < options.iterations; ++i) {
        await record(cases[0].name, cases[0].url);
      }
      timings.total_ms = now() - started;
    } else if (options.scenario === 'batch') {
      for (let i = 0; i < options.warmup; ++i) {
        await engine.shot(cases[0].url);
      }
      mark('warm');
      const started = now();
      for (const item of cases) {
        await record(item.name, item.url);
      }
      timings.total_ms = now() - started;
    } else if (options.scenario === 'batch-parallel') {
      for (let i = 0; i < options.warmup; ++i) {
        await engine.shot(cases[0].url);
      }
      mark('warm');
      const started = now();
      // A fixed window rather than Promise.all over everything: four in flight
      // is what both sides were configured for -- four shotium.exe workers, four
      // pages -- and letting eleven go at once would measure how each engine
      // degrades under overload instead.
      const queue = cases.slice();
      const workers = Array.from({length: options.concurrency}, async () => {
        for (;;) {
          const item = queue.shift();
          if (!item) {
            return;
          }
          await record(item.name, item.url);
        }
      });
      await Promise.all(workers);
      timings.total_ms = now() - started;
    } else {
      throw new Error(`unknown scenario "${options.scenario}"`);
    }
    mark('done');
    const closeStarted = now();
    await engine.close();
    timings.close_ms = now() - closeStarted;
  }

  mark('closed');
  process.stdout.write(`${JSON.stringify({
    engine: options.engine,
    scenario: options.scenario,
    reuse_page: options.reusePage,
    pid: process.pid,
    node: process.version,
    marks,
    timings,
    shots,
  })}\n`);
}

main().catch((error) => {
  // Not every rejection is an Error. Puppeteer and playwright both reject with
  // plain objects in places, and `${error}` on one of those is the useless
  // string "[object Object]" -- which is exactly what the harness would record
  // for a failed cell, and exactly what nobody can debug afterwards.
  let detail;
  if (error && error.stack) {
    detail = error.stack;
  } else {
    try {
      detail = `${typeof error}: ${JSON.stringify(error)}`;
    } catch (cyclic) {
      detail = String(error);
    }
  }
  process.stderr.write(`${detail}\n`);
  process.exit(1);
});
