// Draws the two comparison cards at the top of the READMEs from one benchmark
// archive: apps/docs/assets/hero.svg (English) and hero.zh.svg (Chinese).
//
//   pnpm docs:hero                                          # newest archive
//   pnpm docs:hero --archive apps/docs/benchmarks/v0.7.3/<run>
//   pnpm docs:hero --platform darwin-arm64 --against playwright-chrome
//
// The cards read the soak cells -- 1,000 captures at concurrency 4 -- of
// shotium and one competitor on one platform: the wall time of the whole
// batch, and the peak RSS of each engine's process tree. Both cells must
// have passed and be ranking-eligible; a noisy or failed cell is refused
// rather than drawn, because a picture with no error bars has to come from
// a measurement the archive itself trusts. The multiplier is rounded down.
// The archive, platform, versions and metrics are written into the image,
// so the picture cannot outlive its source unnoticed.
//
// Run it after `bench(results)` lands for a release, and commit the SVGs.

import {existsSync, readdirSync, readFileSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';

import {assetsDir, repoRoot} from '../lib/docs-workspace.ts';
import {resolve} from '../lib/repo.ts';

export const ARCHIVES = path.join(repoRoot, 'apps', 'docs', 'benchmarks');
export const SCENARIO = 'soak';

const ENGINES: Record<string, {en: string; zh: string}> = {
  'puppeteer-chrome': {en: 'Puppeteer with Chrome', zh: 'Puppeteer + Chrome'},
  'puppeteer-shell': {en: 'Puppeteer with chrome-headless-shell', zh: 'Puppeteer + chrome-headless-shell'},
  'playwright-chrome': {en: 'Playwright with Chromium', zh: 'Playwright + Chromium'},
  'playwright-shell': {en: 'Playwright with chromium-headless-shell', zh: 'Playwright + chromium-headless-shell'},
};

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
  against: string;
  shotium: string;
  competitor: string;   // e.g. "Puppeteer 25.8.0 + Chrome 152.0.7977.42"
  shots: number;
  concurrency: number;
  seconds: {ours: number; theirs: number};
  bytes: {ours: number; theirs: number};
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

function soakCell(summary: PlatformSummary, engine: string): Cell {
  const cell = summary.scenarios.find((c) => c.scenario === SCENARIO && c.engine === engine);
  if (!cell) throw new Error(`${summary.platform}: no ${SCENARIO} cell for ${engine}`);
  if (cell.status !== 'pass' || !cell.ranking_eligible || !cell.shots) {
    throw new Error(`${summary.platform}: the ${SCENARIO} cell for ${engine} is ${cell.status}` +
      `${cell.ranking_eligible ? '' : ', not ranking-eligible'} (${cell.shots ?? 0} shots); refusing to draw it`);
  }
  if (!cell.wall_time_ms?.p50 || !cell.peak_rss_bytes?.max) {
    throw new Error(`${summary.platform}: the ${SCENARIO} cell for ${engine} has no wall time or peak RSS`);
  }
  return cell;
}

// The competitor line names what a user would install: the driver package
// version from the harness lockfile and the browser it downloaded.
function competitorLabel(summary: PlatformSummary, against: string): string {
  const driver = against.split('-')[0];
  const driverVersion = summary.packages[driver] ?? '';
  const browser = summary.engines.find((e) => e.engine === against)?.binary_version ?? '';
  const browserVersion = browser.match(/\d+(\.\d+)+/)?.[0] ?? browser;
  const product = against === 'puppeteer-chrome' ? 'Chrome'
    : against === 'puppeteer-shell' ? 'chrome-headless-shell'
    : against === 'playwright-chrome' ? 'Chromium'
    : 'chromium-headless-shell';
  const name = driver === 'puppeteer' ? 'Puppeteer' : 'Playwright';
  return `${name} ${driverVersion} + ${product} ${browserVersion}`.replace(/\s+/g, ' ').trim();
}

export function figures(summary: PlatformSummary, against: string): Figures {
  if (!(against in ENGINES)) throw new Error(`unknown competitor ${against}; one of ${Object.keys(ENGINES).join(', ')}`);
  const ours = soakCell(summary, 'shotium');
  const theirs = soakCell(summary, against);
  if (ours.shots !== theirs.shots || ours.concurrency !== theirs.concurrency) {
    throw new Error(`${summary.platform}: shotium ran ${ours.shots} shots at c${ours.concurrency}, ` +
      `${against} ${theirs.shots} at c${theirs.concurrency}; the cards compare like with like`);
  }
  return {
    platform: summary.platform,
    against,
    shotium: summary.shotium_version,
    competitor: competitorLabel(summary, against),
    shots: ours.shots!,
    concurrency: ours.concurrency,
    seconds: {ours: ours.wall_time_ms!.p50 / 1000, theirs: theirs.wall_time_ms!.p50 / 1000},
    bytes: {ours: ours.peak_rss_bytes!.max, theirs: theirs.peak_rss_bytes!.max},
  };
}

// Rounded down: "4×" for 4.3 and for 4.9. The exact values sit on the bars.
export function multiplier(theirs: number, ours: number): string {
  return `${Math.floor(theirs / ours)}×`;
}

export function seconds(value: number): string {
  return value >= 100 ? `${Math.round(value)} s` : `${value.toFixed(1)} s`;
}

export function bytes(value: number): string {
  return value >= 1e9 ? `${(value / 1e9).toFixed(1)} GB` : `${Math.round(value / 1e6)} MB`;
}

const FONT = "-apple-system, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'PingFang SC', 'Microsoft YaHei', sans-serif";
const WIDTH = 820;
const CARD = {width: 396, height: 150, gap: 28, radius: 14};
const BAR = {x: 58, width: 256, height: 8};
const ROWS = [88, 118];
const FOOT = 40;

const COLOUR = {
  card: '#1b1b1f',
  edge: '#2c2c33',
  title: '#ffffff',
  dim: '#c6c6cd',
  value: '#b4b4bc',
  accent: '#7cc4ff',
  theirs: '#4a4a54',
  foot: '#8b8b95',
};

function escape(text: string): string {
  return text.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

// A lens: the only mark shotium has, and it says "capture".
function shotiumIcon(cx: number, cy: number): string {
  return `<circle cx="${cx}" cy="${cy}" r="10" fill="#2f7cf6"/>` +
    `<circle cx="${cx}" cy="${cy}" r="5.5" fill="none" stroke="#ffffff" stroke-width="2.2"/>` +
    `<circle cx="${cx}" cy="${cy}" r="2" fill="#ffffff"/>`;
}

// Chrome's mark reduced to its geometry: three sectors and the blue centre.
function chromeIcon(cx: number, cy: number): string {
  const r = 10;
  const point = (degrees: number): string => {
    const a = (degrees * Math.PI) / 180;
    return `${(cx + r * Math.cos(a)).toFixed(2)} ${(cy + r * Math.sin(a)).toFixed(2)}`;
  };
  const sector = (from: number, to: number, fill: string): string =>
    `<path d="M ${cx} ${cy} L ${point(from)} A ${r} ${r} 0 0 1 ${point(to)} Z" fill="${fill}"/>`;
  return sector(210, 330, '#db4437') + sector(330, 90, '#0f9d58') + sector(90, 210, '#f4b400') +
    `<circle cx="${cx}" cy="${cy}" r="4.6" fill="#ffffff"/>` +
    `<circle cx="${cx}" cy="${cy}" r="3.4" fill="#4285f4"/>`;
}

interface Card {
  title: string;
  multiplier: string;
  suffix: string;
  ours: {label: string; fraction: number};
  theirs: {label: string};
}

function card(x: number, c: Card): string {
  const rows = [
    {y: ROWS[0], icon: shotiumIcon(24 + 10, ROWS[0]), width: Math.max(6, Math.round(BAR.width * c.ours.fraction)), fill: COLOUR.accent, label: c.ours.label},
    {y: ROWS[1], icon: chromeIcon(24 + 10, ROWS[1]), width: BAR.width, fill: COLOUR.theirs, label: c.theirs.label},
  ];
  return `<g transform="translate(${x} 0)">` +
    `<rect x="0.5" y="0.5" width="${CARD.width - 1}" height="${CARD.height - 1}" rx="${CARD.radius}" fill="${COLOUR.card}" stroke="${COLOUR.edge}"/>` +
    `<text x="24" y="50" font-family="${FONT}" font-size="19" font-weight="700" fill="${COLOUR.title}">${escape(c.title)}` +
    `<tspan font-weight="400" fill="${COLOUR.dim}"> – </tspan>` +
    `<tspan font-size="30" fill="${COLOUR.accent}">${escape(c.multiplier)}</tspan>` +
    `<tspan font-size="15" font-weight="500" fill="${COLOUR.dim}"> ${escape(c.suffix)}</tspan></text>` +
    rows.map((row) =>
      row.icon +
      `<rect x="${BAR.x}" y="${row.y - BAR.height / 2}" width="${row.width}" height="${BAR.height}" rx="${BAR.height / 2}" fill="${row.fill}"/>` +
      `<text x="${BAR.x + row.width + 8}" y="${row.y + 4}" font-family="${FONT}" font-size="11" fill="${COLOUR.value}">${escape(row.label)}</text>`,
    ).join('') +
    '</g>';
}

export function render(f: Figures, locale: 'en' | 'zh'): string {
  // "1,000" in English; Chinese text does not group digits.
  const shots = locale === 'en' ? f.shots.toLocaleString('en-US') : String(f.shots);
  const text = locale === 'en'
    ? {
      time: `${shots} captures`, faster: 'faster',
      memory: 'Memory peak', less: 'less memory',
      foot1: `${shots} screenshots at concurrency ${f.concurrency}, 1280×720 PNG · ${f.platform} GitHub Actions runner · every engine, every platform in the benchmark section below`,
      foot2: `shotium ${f.shotium} vs ${f.competitor} · wall time of the batch · peak RSS of the process tree`,
      title: `shotium ${f.shotium} against ${f.competitor}: ${shots} screenshots at concurrency ${f.concurrency} on ${f.platform}`,
    }
    : {
      time: `${shots} 张截图`, faster: '更快',
      memory: '内存峰值', less: '更省',
      foot1: `并发 ${f.concurrency}、连续 ${shots} 张 1280×720 PNG · GitHub Actions ${f.platform} · 全部引擎与平台的数据见下方基准一节`,
      foot2: `shotium ${f.shotium} vs ${f.competitor} · 整批总耗时 · 进程树 RSS 峰值`,
      title: `shotium ${f.shotium} 与 ${f.competitor}：${f.platform} 上并发 ${f.concurrency}、${shots} 张截图`,
    };
  const height = CARD.height + FOOT;
  const cards = [
    card(0, {
      title: text.time, multiplier: multiplier(f.seconds.theirs, f.seconds.ours), suffix: text.faster,
      ours: {label: seconds(f.seconds.ours), fraction: f.seconds.ours / f.seconds.theirs},
      theirs: {label: seconds(f.seconds.theirs)},
    }),
    card(CARD.width + CARD.gap, {
      title: text.memory, multiplier: multiplier(f.bytes.theirs, f.bytes.ours), suffix: text.less,
      ours: {label: bytes(f.bytes.ours), fraction: f.bytes.ours / f.bytes.theirs},
      theirs: {label: bytes(f.bytes.theirs)},
    }),
  ];
  const foot = (y: number, line: string): string =>
    `<text x="${WIDTH / 2}" y="${y}" text-anchor="middle" font-family="${FONT}" font-size="11" fill="${COLOUR.foot}">${escape(line)}</text>`;
  return `<svg xmlns="http://www.w3.org/2000/svg" width="${WIDTH}" height="${height}" viewBox="0 0 ${WIDTH} ${height}" role="img" aria-labelledby="title">\n` +
    `<title id="title">${escape(text.title)}</title>\n` +
    cards.join('\n') + '\n' +
    foot(CARD.height + 17, text.foot1) + '\n' +
    foot(CARD.height + 32, text.foot2) + '\n' +
    '</svg>\n';
}

if (process.argv[1] && path.resolve(process.argv[1]) === import.meta.filename) {
  const cli = cac('pnpm docs:hero');
  cli.command('', 'draw the README comparison cards from a benchmark archive')
    .option('--archive <dir>', 'archive directory, relative to the repository root; the newest publishable one under apps/docs/benchmarks by default')
    .option('--platform <id>', 'platform whose cells are drawn', {default: 'linux-x64'})
    .option('--against <engine>', `competitor: ${Object.keys(ENGINES).join(', ')}`, {default: 'puppeteer-chrome'})
    .action((options: {archive?: string; platform: string; against: string}) => {
      const archive = options.archive ? resolve(options.archive) : newestArchive();
      const summary = readPlatform(archive, options.platform);
      const f = figures(summary, options.against);
      for (const [locale, file] of [['en', 'hero.svg'], ['zh', 'hero.zh.svg']] as const) {
        const target = path.join(assetsDir, file);
        writeFileSync(target, render(f, locale));
        console.log(`wrote ${path.relative(repoRoot, target)}`);
      }
      console.log(`${path.relative(repoRoot, archive)} · ${f.platform} · shotium ${f.shotium} vs ${f.competitor}`);
      console.log(`  ${f.shots} shots at c${f.concurrency}: ${seconds(f.seconds.ours)} vs ${seconds(f.seconds.theirs)} ` +
        `(${(f.seconds.theirs / f.seconds.ours).toFixed(2)}×, drawn as ${multiplier(f.seconds.theirs, f.seconds.ours)})`);
      console.log(`  peak RSS: ${bytes(f.bytes.ours)} vs ${bytes(f.bytes.theirs)} ` +
        `(${(f.bytes.theirs / f.bytes.ours).toFixed(2)}×, drawn as ${multiplier(f.bytes.theirs, f.bytes.ours)})`);
    });
  cli.help();
  cli.parse();
}
