// Check the paired images of a perf-compare result independently of timing,
// retaining every failed case.
//
//   pnpm perf:images out/performance/result.json
//
// Writes result.pixels.json beside the result and exits 1 unless every
// required case passed. The evidence files are whatever the case's encoder
// produced (PNG, JPEG or WebP), so they are decoded with sharp.

import {createHash} from 'node:crypto';
import {existsSync, readFileSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import sharp from 'sharp';

import {resolve} from './lib/repo.ts';

interface Sample {
  workers: Array<{sha256: string; evidence: string}>;
}
interface Case {
  name: string;
  samples: Record<'baseline' | 'candidate', Sample[]>;
}
interface Result {
  cases: Case[];
  selectedCases: string[];
}
interface Check {
  name: string;
  status: 'error' | 'pass';
  error?: string;
  dimensions?: [number, number];
  mean_channel_error?: number[];
  fraction_channels_error_above_16?: number;
}

const sha256 = (data: Buffer) => createHash('sha256').update(data).digest('hex');

async function decode(file: string): Promise<{width: number; height: number; data: Buffer}> {
  const {data, info} = await sharp(file).ensureAlpha().raw().toBuffer({resolveWithObject: true});
  return {width: info.width, height: info.height, data};
}

// Population standard deviation per channel, as Pillow's ImageStat reports.
function stddev(image: {data: Buffer}, channels: number): number[] {
  const n = image.data.length / 4;
  const out: number[] = [];
  for (let c = 0; c < channels; c++) {
    let sum = 0, squares = 0;
    for (let i = 0; i < n; i++) {
      const v = image.data[i * 4 + c];
      sum += v;
      squares += v * v;
    }
    const mean = sum / n;
    out.push(Math.sqrt(Math.max(0, squares / n - mean * mean)));
  }
  return out;
}

export async function verify(resultArg: string): Promise<boolean> {
  const resultPath = resolve(resultArg);
  const result = JSON.parse(readFileSync(resultPath, 'utf8')) as Result;
  const checks: Check[] = [];
  for (const c of result.cases) {
    const record: Check = {name: c.name, status: 'error'};
    checks.push(record);
    try {
      const images: Record<string, {width: number; height: number; data: Buffer}> = {};
      for (const label of ['baseline', 'candidate'] as const) {
        const sample = c.samples[label][0].workers[0];
        const hashes = new Set(c.samples[label].flatMap((row) => row.workers.map((w) => w.sha256)));
        if (hashes.size !== 1 || !hashes.has(sample.sha256)) throw new Error('Output changed within the static case; every variant needs image verification');
        let file = sample.evidence;
        if (!existsSync(file)) {
          const basename = sample.evidence.replace(/\\/g, '/').split('/').pop()!;
          file = path.join(`${resultPath}.images`, c.name, basename);
        }
        if (sha256(readFileSync(file)) !== sample.sha256) throw new Error('Evidence hash mismatch');
        images[label] = await decode(file);
      }
      const a = images.baseline, b = images.candidate;
      if (a.width !== b.width || a.height !== b.height) throw new Error(`Dimensions changed: (${a.width}, ${a.height}) -> (${b.width}, ${b.height})`);
      if (!(Math.max(...stddev(a, 3)) > 1)) throw new Error('Baseline appears blank');
      if (!(Math.max(...stddev(b, 3)) > 1)) throw new Error('Candidate appears blank');
      const pixels = a.width * a.height;
      const sums = [0, 0, 0, 0];
      let changed = 0;
      for (let i = 0; i < pixels * 4; i++) {
        const d = Math.abs(a.data[i] - b.data[i]);
        sums[i % 4] += d;
        if (d >= 17) changed++;
      }
      const mean = sums.map((s) => s / pixels);
      const fraction = changed / (pixels * 4);
      record.dimensions = [a.width, a.height];
      record.mean_channel_error = mean;
      record.fraction_channels_error_above_16 = fraction;
      // Fixed before sampling. Allows sparse antialiasing/codec rounding; no
      // resizing or cropping that could hide missing rows or pictures.
      if (!(Math.max(...mean) <= 1 && fraction <= 0.01)) throw new Error('Paired pixels differ beyond tolerance');
      record.status = 'pass';
    } catch (error) {
      record.error = error instanceof Error ? error.message : String(error);
    }
  }
  const required = new Set(result.selectedCases);
  const passed = new Set(checks.filter((c) => c.status === 'pass').map((c) => c.name));
  const status = passed.size === required.size && [...required].every((r) => passed.has(r)) ? 'pass' : 'not-passed';
  const output = {result_sha256: sha256(readFileSync(resultPath)), status, checks};
  writeFileSync(resultPath.replace(/\.json$/, '') + '.pixels.json', JSON.stringify(output, null, 2));
  console.log(`Paired images: ${passed.size}/${required.size} passed`);
  for (const c of checks) if (c.status !== 'pass') console.log(c.name, c.error);
  return status === 'pass';
}

const cli = cac('perf-images');
cli.command('<result>', 'check the paired images of a perf-compare result')
    .action(async (result: string) => {
      process.exitCode = (await verify(result)) ? 0 : 1;
    });
cli.help();
cli.parse();
