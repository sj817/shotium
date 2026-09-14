// Draws the two comparison cards at the top of the READMEs from one benchmark
// archive: apps/docs/assets/hero.svg (English) and hero.zh.svg (Chinese).
//
//   pnpm docs:hero                                          # newest archive
//   pnpm docs:hero --archive apps/docs/benchmarks/v0.8.0/<run>
//   pnpm docs:hero --platform linux-x64
//
// The cards compare Cold Start latency (Shotium vs Playwright) and Memory Peak
// under soak testing (Shotium vs Chrome) from the trusted benchmark archive.
//
// Run it after `bench(results)` lands for a release, and commit the SVGs.

import {existsSync, readdirSync, readFileSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';

import {assetsDir, repoRoot} from '../lib/docs-workspace.ts';
import {resolve} from '../lib/repo.ts';

export const ARCHIVES = path.join(repoRoot, 'apps', 'docs', 'benchmarks');

export interface Distribution {
  n: number;
  p50: number;
  max: number;
}

export interface Cell {
  engine: string;
  scenario: string;
  concurrency: number;
  status: string;
  ranking_eligible: boolean;
  shots: number | null;
  wall_time_ms: Distribution | null;
  peak_rss_bytes: Distribution | null;
}

export interface PlatformSummary {
  platform: string;
  status: string;
  shotium_version: string;
  packages: Record<string, string>;
  engines: {engine: string; status: string; binary_version: string | null}[];
  scenarios: Cell[];
}

export interface Figures {
  platform: string;
  shotium: string;
  cold: {
    ours_ms: number;
    theirs_ms: number;
    theirs_label: string;
    theirs_full: string;
    multiplier: string;
  };
  memory: {
    ours_bytes: number;
    theirs_bytes: number;
    theirs_label: string;
    theirs_full: string;
    multiplier: string;
  };
}

export function newestArchive(root = ARCHIVES): string {
  let best: {directory: string; generated: string} | null = null;
  for (const version of readdirSync(root, {withFileTypes: true})) {
    if (!version.isDirectory() || !version.name.startsWith('v')) continue;
    for (const run of readdirSync(path.join(root, version.name), {withFileTypes: true})) {
      const directory = path.join(root, version.name, run.name);
      const manifest = path.join(directory, 'manifest.json');
      if (!run.isDirectory() || !existsSync(manifest)) continue;
      const {generated_utc, publishable} = JSON.parse(readFileSync(manifest, 'utf8')) as {generated_utc: string; publishable: boolean};
      if (!publishable) continue;
      if (!best || generated_utc > best.generated) best = {directory, generated: generated_utc};
    }
  }
  if (!best) throw new Error(`no publishable archive under ${root}`);
  return best.directory;
}

export function readPlatform(archive: string, platform: string): PlatformSummary {
  const file = path.join(archive, platform, 'summary.json');
  if (!existsSync(file)) throw new Error(`${archive} has no ${platform}/summary.json`);
  return JSON.parse(readFileSync(file, 'utf8')) as PlatformSummary;
}

function verifyCell(summary: PlatformSummary, scenario: string, engine: string, concurrency: number): Cell {
  const cell = summary.scenarios.find((c) => c.scenario === scenario && c.engine === engine && c.concurrency === concurrency);
  if (!cell) throw new Error(`${summary.platform}: no ${scenario} cell for ${engine}`);
  if (cell.status !== 'pass' || !cell.ranking_eligible) {
    throw new Error(`${summary.platform}: the ${scenario} cell for ${engine} is ${cell.status}` +
      `${cell.ranking_eligible ? '' : ', not ranking-eligible'} (${cell.shots ?? 0} shots); refusing to draw it`);
  }
  return cell;
}

export function multiplier(theirs: number, ours: number): string {
  return `${(theirs / ours).toFixed(1)}×`;
}

export function bytes(value: number): string {
  return value >= 1e9 ? `${(value / 1e9).toFixed(1)} GB` : `${Math.round(value / 1e6)} MB`;
}

export function figures(summary: PlatformSummary): Figures {
  const coldOurs = verifyCell(summary, 'cold', 'shotium', 1);
  const coldTheirs = verifyCell(summary, 'cold', 'playwright-chrome', 1);
  if (!coldOurs.wall_time_ms?.p50 || !coldTheirs.wall_time_ms?.p50) {
    throw new Error(`${summary.platform}: missing cold start p50 timing`);
  }

  const soakOurs = verifyCell(summary, 'soak', 'shotium', 4);
  const soakTheirs = verifyCell(summary, 'soak', 'puppeteer-chrome', 4);
  if (!soakOurs.peak_rss_bytes?.max || !soakTheirs.peak_rss_bytes?.max) {
    throw new Error(`${summary.platform}: missing soak peak RSS memory`);
  }

  return {
    platform: summary.platform,
    shotium: summary.shotium_version,
    cold: {
      ours_ms: Math.round(coldOurs.wall_time_ms.p50),
      theirs_ms: Math.round(coldTheirs.wall_time_ms.p50),
      theirs_label: 'Playwright',
      theirs_full: 'Playwright + Chromium',
      multiplier: multiplier(coldTheirs.wall_time_ms.p50, coldOurs.wall_time_ms.p50),
    },
    memory: {
      ours_bytes: soakOurs.peak_rss_bytes.max,
      theirs_bytes: soakTheirs.peak_rss_bytes.max,
      theirs_label: 'Chrome',
      theirs_full: 'Puppeteer + Chrome',
      multiplier: multiplier(soakTheirs.peak_rss_bytes.max, soakOurs.peak_rss_bytes.max),
    },
  };
}

const FONT = "-apple-system, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'PingFang SC', 'Microsoft YaHei', sans-serif";

function shotiumIcon(cx: number, cy: number): string {
  return `<circle cx="${cx}" cy="${cy}" r="9.5" fill="url(#lensGrad)"/>` +
    `<circle cx="${cx}" cy="${cy}" r="5.2" fill="none" stroke="#ffffff" stroke-width="1.8" stroke-opacity="0.95"/>` +
    `<circle cx="${cx}" cy="${cy}" r="1.8" fill="#ffffff"/>`;
}

function chromeIcon(cx: number, cy: number): string {
  const r = 9.5;
  const point = (degrees: number): string => {
    const a = (degrees * Math.PI) / 180;
    return `${(cx + r * Math.cos(a)).toFixed(2)} ${(cy + r * Math.sin(a)).toFixed(2)}`;
  };
  const sector = (from: number, to: number, fill: string): string =>
    `<path d="M ${cx} ${cy} L ${point(from)} A ${r} ${r} 0 0 1 ${point(to)} Z" fill="${fill}"/>`;
  return sector(210, 330, '#db4437') + sector(330, 90, '#0f9d58') + sector(90, 210, '#f4b400') +
    `<circle cx="${cx}" cy="${cy}" r="4.3" fill="#ffffff"/>` +
    `<circle cx="${cx}" cy="${cy}" r="3.2" fill="#4285f4"/>`;
}

function escape(text: string): string {
  return text.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

export function render(f: Figures, locale: 'en' | 'zh'): string {
  const isZh = locale === 'zh';
  const leftTitle = isZh ? '冷启动延迟' : 'Cold start';
  const leftSuffix = isZh ? '更快' : 'faster';

  const rightTitle = isZh ? '内存峰值' : 'Memory peak';
  const rightSuffix = isZh ? '更省' : 'less memory';

  const foot1 = isZh
    ? `GitHub Actions ${f.platform} · 冷启动 p50 对比 Playwright Chromium；高压 1000 张连续测试对比 Chrome 进程树内存峰值`
    : `Cold start p50 on ${f.platform} (Shotium ${f.cold.ours_ms} ms vs Playwright ${f.cold.theirs_ms} ms) · Memory peak at c4 1,000 shots (${bytes(f.memory.ours_bytes)} vs ${bytes(f.memory.theirs_bytes)})`;
  const foot2 = isZh
    ? `shotium ${f.shotium} vs Playwright + Chromium &amp; Puppeteer + Chrome · 全部引擎与平台的数据见下方基准一节`
    : `shotium ${f.shotium} vs Playwright + Chromium &amp; Puppeteer + Chrome · ${f.platform} GitHub Actions runner · verified in benchmark section`;

  const barW = 175;
  const barX = 124;
  const valX = 371;

  const c1OursW = Math.max(10, Math.round(barW * (f.cold.ours_ms / f.cold.theirs_ms)));
  const c2OursW = Math.max(10, Math.round(barW * (f.memory.ours_bytes / f.memory.theirs_bytes)));

  return `<svg xmlns="http://www.w3.org/2000/svg" width="820" height="190" viewBox="0 0 820 190" role="img" aria-labelledby="title">
<title id="title">shotium benchmark comparison</title>
<defs>
  <!-- Card background: elevated rich slate-navy, no pitch-black falloff -->
  <linearGradient id="cardBg" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0%" stop-color="#1c1f2b"/>
    <stop offset="55%" stop-color="#161924"/>
    <stop offset="100%" stop-color="#131520"/>
  </linearGradient>

  <!-- Card borders with clear top-to-bottom definition -->
  <linearGradient id="cardBorderCyan" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0%" stop-color="#38bdf8" stop-opacity="0.6"/>
    <stop offset="35%" stop-color="#353b50" stop-opacity="0.95"/>
    <stop offset="100%" stop-color="#272c3d" stop-opacity="0.8"/>
  </linearGradient>

  <linearGradient id="cardBorderIndigo" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0%" stop-color="#818cf8" stop-opacity="0.6"/>
    <stop offset="35%" stop-color="#353b50" stop-opacity="0.95"/>
    <stop offset="100%" stop-color="#272c3d" stop-opacity="0.8"/>
  </linearGradient>

  <!-- Specular top highlight line -->
  <linearGradient id="specularGlow" x1="0" y1="0" x2="1" y2="0">
    <stop offset="0%" stop-color="#ffffff" stop-opacity="0"/>
    <stop offset="30%" stop-color="#ffffff" stop-opacity="0.28"/>
    <stop offset="70%" stop-color="#ffffff" stop-opacity="0.28"/>
    <stop offset="100%" stop-color="#ffffff" stop-opacity="0"/>
  </linearGradient>

  <!-- Key light (top-right) -->
  <radialGradient id="keyCyan" cx="85%" cy="12%" r="65%">
    <stop offset="0%" stop-color="#0ea5e9" stop-opacity="0.20"/>
    <stop offset="100%" stop-color="#0ea5e9" stop-opacity="0"/>
  </radialGradient>

  <radialGradient id="keyIndigo" cx="85%" cy="12%" r="65%">
    <stop offset="0%" stop-color="#6366f1" stop-opacity="0.20"/>
    <stop offset="100%" stop-color="#6366f1" stop-opacity="0"/>
  </radialGradient>

  <!-- Fill light (bottom-left) to illuminate the bottom and left areas -->
  <radialGradient id="fillCyan" cx="12%" cy="88%" r="60%">
    <stop offset="0%" stop-color="#0284c7" stop-opacity="0.14"/>
    <stop offset="100%" stop-color="#0284c7" stop-opacity="0"/>
  </radialGradient>

  <radialGradient id="fillIndigo" cx="12%" cy="88%" r="60%">
    <stop offset="0%" stop-color="#4f46e5" stop-opacity="0.14"/>
    <stop offset="100%" stop-color="#4f46e5" stop-opacity="0"/>
  </radialGradient>

  <!-- Metric text gradients -->
  <linearGradient id="metricCyan" x1="0%" y1="0%" x2="100%" y2="0%">
    <stop offset="0%" stop-color="#38bdf8"/>
    <stop offset="100%" stop-color="#60a5fa"/>
  </linearGradient>

  <linearGradient id="metricIndigo" x1="0%" y1="0%" x2="100%" y2="0%">
    <stop offset="0%" stop-color="#a78bfa"/>
    <stop offset="100%" stop-color="#c084fc"/>
  </linearGradient>

  <!-- Bar gradients -->
  <linearGradient id="barCyan" x1="0%" y1="0%" x2="100%" y2="0%">
    <stop offset="0%" stop-color="#38bdf8"/>
    <stop offset="100%" stop-color="#60a5fa"/>
  </linearGradient>

  <linearGradient id="barIndigo" x1="0%" y1="0%" x2="100%" y2="0%">
    <stop offset="0%" stop-color="#818cf8"/>
    <stop offset="100%" stop-color="#c084fc"/>
  </linearGradient>

  <linearGradient id="lensGrad" x1="0%" y1="0%" x2="100%" y2="100%">
    <stop offset="0%" stop-color="#38bdf8"/>
    <stop offset="100%" stop-color="#2563eb"/>
  </linearGradient>
</defs>

<!-- LEFT CARD: COLD START -->
<g transform="translate(0 0)">
  <!-- Subtle elevation shadow -->
  <rect x="0.5" y="2" width="395" height="149" rx="14" fill="#000000" opacity="0.18"/>
  <!-- Card base -->
  <rect x="0.5" y="0.5" width="395" height="149" rx="14" fill="url(#cardBg)" stroke="url(#cardBorderCyan)"/>
  <!-- Key light (top-right) -->
  <rect x="0.5" y="0.5" width="395" height="149" rx="14" fill="url(#keyCyan)"/>
  <!-- Fill light (bottom-left) -->
  <rect x="0.5" y="0.5" width="395" height="149" rx="14" fill="url(#fillCyan)"/>
  <!-- Specular top edge -->
  <path d="M 20 1 L 375 1" stroke="url(#specularGlow)" stroke-width="1"/>

  <!-- Title & Metric -->
  <text x="24" y="47" font-family="${FONT}">
    <tspan font-size="18" font-weight="700" fill="#ffffff">${escape(leftTitle)}</tspan>
    <tspan font-size="16" font-weight="400" fill="#64748b"> · </tspan>
    <tspan font-size="28" font-weight="800" fill="url(#metricCyan)">${escape(f.cold.multiplier)}</tspan>
    <tspan font-size="14" font-weight="500" fill="#94a3b8"> ${escape(leftSuffix)}</tspan>
  </text>

  <!-- Row 1: Shotium -->
  ${shotiumIcon(34, 88)}
  <text x="50" y="92" font-family="${FONT}" font-size="12" font-weight="600" fill="#f1f5f9">Shotium</text>
  <rect x="${barX}" y="83" width="${barW}" height="10" rx="5" fill="#1e222e" stroke="#2d3345" stroke-width="0.75"/>
  <rect x="${barX}" y="83" width="${c1OursW}" height="10" rx="5" fill="url(#barCyan)"/>
  <text x="${valX}" y="92" text-anchor="end" font-family="${FONT}" font-size="13" font-weight="700" fill="#38bdf8">${f.cold.ours_ms} ms</text>

  <!-- Row 2: Playwright -->
  ${chromeIcon(34, 118)}
  <text x="50" y="122" font-family="${FONT}" font-size="12" font-weight="500" fill="#a4adc0">${escape(f.cold.theirs_label)}</text>
  <rect x="${barX}" y="113" width="${barW}" height="10" rx="5" fill="#1e222e" stroke="#2d3345" stroke-width="0.75"/>
  <rect x="${barX}" y="113" width="${barW}" height="10" rx="5" fill="#44485a"/>
  <text x="${valX}" y="122" text-anchor="end" font-family="${FONT}" font-size="13" font-weight="500" fill="#a4adc0">${f.cold.theirs_ms} ms</text>
</g>

<!-- RIGHT CARD: MEMORY PEAK -->
<g transform="translate(425 0)">
  <!-- Subtle elevation shadow -->
  <rect x="0.5" y="2" width="395" height="149" rx="14" fill="#000000" opacity="0.18"/>
  <!-- Card base -->
  <rect x="0.5" y="0.5" width="395" height="149" rx="14" fill="url(#cardBg)" stroke="url(#cardBorderIndigo)"/>
  <!-- Key light (top-right) -->
  <rect x="0.5" y="0.5" width="395" height="149" rx="14" fill="url(#keyIndigo)"/>
  <!-- Fill light (bottom-left) -->
  <rect x="0.5" y="0.5" width="395" height="149" rx="14" fill="url(#fillIndigo)"/>
  <!-- Specular top edge -->
  <path d="M 20 1 L 375 1" stroke="url(#specularGlow)" stroke-width="1"/>

  <!-- Title & Metric -->
  <text x="24" y="47" font-family="${FONT}">
    <tspan font-size="18" font-weight="700" fill="#ffffff">${escape(rightTitle)}</tspan>
    <tspan font-size="16" font-weight="400" fill="#64748b"> · </tspan>
    <tspan font-size="28" font-weight="800" fill="url(#metricIndigo)">${escape(f.memory.multiplier)}</tspan>
    <tspan font-size="14" font-weight="500" fill="#94a3b8"> ${escape(rightSuffix)}</tspan>
  </text>

  <!-- Row 1: Shotium -->
  ${shotiumIcon(34, 88)}
  <text x="50" y="92" font-family="${FONT}" font-size="12" font-weight="600" fill="#f1f5f9">Shotium</text>
  <rect x="${barX}" y="83" width="${barW}" height="10" rx="5" fill="#1e222e" stroke="#2d3345" stroke-width="0.75"/>
  <rect x="${barX}" y="83" width="${c2OursW}" height="10" rx="5" fill="url(#barIndigo)"/>
  <text x="${valX}" y="92" text-anchor="end" font-family="${FONT}" font-size="13" font-weight="700" fill="#c084fc">${bytes(f.memory.ours_bytes)}</text>

  <!-- Row 2: Chrome -->
  ${chromeIcon(34, 118)}
  <text x="50" y="122" font-family="${FONT}" font-size="12" font-weight="500" fill="#a4adc0">${escape(f.memory.theirs_label)}</text>
  <rect x="${barX}" y="113" width="${barW}" height="10" rx="5" fill="#1e222e" stroke="#2d3345" stroke-width="0.75"/>
  <rect x="${barX}" y="113" width="${barW}" height="10" rx="5" fill="#44485a"/>
  <text x="${valX}" y="122" text-anchor="end" font-family="${FONT}" font-size="13" font-weight="500" fill="#a4adc0">${bytes(f.memory.theirs_bytes)}</text>
</g>

<!-- FOOTERS -->
<text x="410" y="167" text-anchor="middle" font-family="${FONT}" font-size="11" fill="#717a8c">${foot1}</text>
<text x="410" y="182" text-anchor="middle" font-family="${FONT}" font-size="11" fill="#717a8c">${foot2}</text>
</svg>`;
}

if (process.argv[1] && path.resolve(process.argv[1]) === import.meta.filename) {
  const cli = cac('pnpm docs:hero');
  cli.command('', 'draw the README comparison cards from a benchmark archive')
    .option('--archive <dir>', 'archive directory, relative to the repository root; the newest publishable one under apps/docs/benchmarks by default')
    .option('--platform <id>', 'platform whose cells are drawn', {default: 'linux-x64'})
    .action((options: {archive?: string; platform: string}) => {
      const archive = options.archive ? resolve(options.archive) : newestArchive();
      const summary = readPlatform(archive, options.platform);
      const f = figures(summary);
      for (const [locale, file] of [['en', 'hero.svg'], ['zh', 'hero.zh.svg']] as const) {
        const target = path.join(assetsDir, file);
        writeFileSync(target, render(f, locale));
        console.log(`wrote ${path.relative(repoRoot, target)}`);
      }
      console.log(`${path.relative(repoRoot, archive)} · ${f.platform} · shotium ${f.shotium}`);
      console.log(`  cold start: ${f.cold.ours_ms} ms vs ${f.cold.theirs_ms} ms (${f.cold.multiplier})`);
      console.log(`  memory peak: ${bytes(f.memory.ours_bytes)} vs ${bytes(f.memory.theirs_bytes)} (${f.memory.multiplier})`);
    });
  cli.help();
  cli.parse();
}
