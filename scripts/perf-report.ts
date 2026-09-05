// Render the complete six-platform acceptance report from perf-compare
// results, without selecting the best run.
//
//   pnpm perf:report out/evidence/performance-*/result.json --output out/performance-acceptance.md
//   pnpm perf:report ... --platform linux-x64 --platform win32-x64
//
// Exits 1 unless every platform in scope passed and every result came from
// the same source. The report is in Chinese, as the acceptance documents in
// this repository are.

import {createHash} from 'node:crypto';
import {readFileSync, existsSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';

import {resolve} from './lib/repo.ts';

const PLATFORMS = new Set(['linux', 'win32', 'darwin'].flatMap((system) => ['x64', 'arm64'].map((arch) => `${system}-${arch}`)));

interface Summary {
  p50: number;
  p95: number;
  mean: number;
}
interface Case {
  name: string;
  status: string;
  accepted?: boolean;
  class?: string;
  summary?: {baseline: Summary; candidate: Summary};
  metrics?: Record<string, {summary: {baseline: Summary; candidate: Summary}; status: string; samples?: number; intervals?: Record<string, {lo: number; hi: number}>}>;
}
interface Result {
  platform: string;
  arch: string;
  revision: string;
  sourceDiffSha256: string;
  harnessSha256: string;
  fixtureManifestSha256: string;
  requiredCases: string[];
  cases: Case[];
  complete?: boolean;
  shard?: string;
  calibration?: {tolerance?: number | {primary: number; tail: number}; source?: string};
  sampling?: {minimumPairs?: number; minSeconds?: number; precision?: number; maxSeconds?: number; maxPairs?: number};
  metadata: Record<string, {packageVersion: string; librarySha256: string}>;
}

const f2 = (n: number) => n.toFixed(2);
const f3 = (n: number) => n.toFixed(3);
// Python's :g for the precision percentage: 2 -> "2", 2.5 -> "2.5".
const g = (n: number) => String(Number(n.toPrecision(6)));

export function report(files: string[], output: string, platforms: Set<string> = PLATFORMS): boolean {
  const lines: string[] = [
    '# 本次候选构建与 npm 的完整矩阵验收', '',
    `验收范围：${[...platforms].sort().join(', ')}。只有范围内的完整矩阵、逐项耗时和像素检查全部通过，才满足这份验收。` +
        '倒退、未能确定方向、缺测和失败不会相互抵消。', '',
  ];
  const seen = new Set<string>();
  const identities = new Set<string>();
  const passing = new Set<string>();
  for (const file of files) {
    const data = JSON.parse(readFileSync(file, 'utf8')) as Result;
    const platform = `${data.platform}-${data.arch}`;
    if (!platforms.has(platform)) throw new Error(`Unexpected platform outside acceptance scope: ${platform}`);
    if (seen.has(platform)) throw new Error(`Duplicate platform ${platform}; do not select the best repeat`);
    seen.add(platform);
    identities.add(JSON.stringify([data.revision, data.sourceDiffSha256, data.harnessSha256, data.fixtureManifestSha256]));
    const required = new Set(data.requiredCases);
    const cases = data.cases;
    const observed = new Set(cases.map((c) => c.name));
    const pixelsFile = file.replace(/\.json$/, '') + '.pixels.json';
    const pixels = existsSync(pixelsFile) ? JSON.parse(readFileSync(pixelsFile, 'utf8')) as {status?: string; result_sha256?: string} : {};
    const pixelOk = pixels.status === 'pass' && pixels.result_sha256 === createHash('sha256').update(readFileSync(file)).digest('hex');
    const complete = Boolean(data.complete) && data.shard === 'all' &&
        observed.size === required.size && [...required].every((r) => observed.has(r)) && cases.length === required.size;
    const accepted = cases.filter((c) => Boolean(c.accepted)).length;
    const counts = Object.fromEntries(['faster', 'equivalent', 'slower', 'unproven'].map((s) => [s, cases.filter((c) => c.status === s).length]));
    const external = cases.filter((c) => c.class === 'external').length;
    const calibration = data.calibration ?? {};
    const band = calibration.tolerance ?? 0;
    // One band for the body and one for the tail; an older result carried a
    // single number for both.
    const [bodyBand, tailBand] = typeof band === 'object' ? [band.primary ?? 0, band.tail ?? 0] : [band, band];
    const bandSource = calibration.source ?? '未记录';
    const sampling = data.sampling;
    if (complete && accepted === required.size && pixelOk) passing.add(platform);
    lines.push(
        `## ${platform}`, '',
        `覆盖：${observed.size}/${required.size}；验收通过：${accepted}/${required.size}；完整采样：${complete ? '是' : '否'}；` +
            `独立像素检查：${pixelOk ? '通过' : '未通过或未运行'}。`, '',
        `噪声带：中位/均值 +${f2(bodyBand * 100)}%，尾部 +${f2(tailBand * 100)}%（${bandSource}）。` +
            (sampling ?
                 `采样：每项至少 ${sampling.minimumPairs} 对，每侧累计 ${sampling.minSeconds} s，` +
                     (sampling.precision ? `之后直到 p50 与均值比值区间半宽 ≤ ${g(sampling.precision * 100)}% 或每侧 ${sampling.maxSeconds} s，` : '') +
                     `最多 ${sampling.maxPairs} 对。` :
                 '') +
            `判定：${counts.faster} 项更快，${counts.equivalent} 项等价（噪声带内，不可分辨），${counts.slower} 项更慢，${counts.unproven} 项方向未定。` +
            `引擎场景须「更快」才算通过；${external} 项被外部等待占死的场景（服务端固定延迟、静默窗口）只要求不更慢。以下均为毫秒。`, '',
        '| 场景 | 本地 dev | npm | 中位差 | 判定 |', '|---|---:|---:|---:|---|');
    const labels: Record<string, string> = {faster: '更快', equivalent: '等价', slower: '更慢', unproven: '未定', 'insufficient-samples': '样本不足', error: '出错'};
    for (const c of cases) {
      const summary = c.summary;
      if (!summary) {
        lines.push(`| ${c.name} | — | — | — | 未完成 |`);
        continue;
      }
      const dev = summary.candidate.p50, npm = summary.baseline.p50;
      const difference = dev - npm;
      const delta = difference ? `${difference > 0 ? '慢' : '快'} ${f2(Math.abs(difference))} ms` : '相同';
      const verdict = labels[c.status] ?? c.status;
      const mark = c.accepted ? '✓' : '✗';
      lines.push(`| ${c.name} | ${f2(dev)} ms | ${f2(npm)} ms | ${delta} | ${mark} ${verdict} |`);
    }
    lines.push(
        '', '<details>', '<summary>完整区间与尾部耗时</summary>', '',
        '每项指标给出候选/npm 比值的 99% 配对 bootstrap 区间。`faster`：均值整段区间低于 1，或 p50 整段低于 1 且均值落在噪声带内，且没有指标更慢；' +
            '`equivalent`：p50 与均值区间都落在噪声带内、都不低于 1；`slower`：某项区间整段超出它的噪声带；' +
            '`unproven`：p50 或均值区间跨过噪声带边缘。p95 只在整段区间超出尾部带时把一项判为更慢，不做别的。', '',
        '| 场景 | 样本对 | npm p50 / p95 / mean ms | 本地 dev p50 / p95 / mean ms | 比值区间 p50 / mean / p95 | 状态 |', '|---|---:|---:|---:|---|---|');
    for (const c of cases) {
      const summary = c.summary;
      const wall = c.metrics?.wall;
      const numbers = (values?: Summary) => values ? ['p50', 'p95', 'mean'].map((k) => f3(values[k as keyof Summary])).join(' / ') : '—';
      const spans = () => {
        const intervals = wall?.intervals;
        if (!intervals) return '—';
        return ['p50', 'mean', 'p95'].map((k) => `[${f3(intervals[k].lo)}, ${f3(intervals[k].hi)}]`).join(' / ');
      };
      const pairs = wall?.samples ?? '—';
      lines.push(`| ${c.name} | ${pairs} | ${numbers(summary?.baseline)} | ${numbers(summary?.candidate)} | ${spans()} | ${c.status} |`);
      for (const [key, metric] of Object.entries(c.metrics ?? {})) {
        if (key === 'wall') continue;
        lines.push(`| ↳ ${key} | — | ${numbers(metric.summary.baseline)} | ${numbers(metric.summary.candidate)} | — | ${metric.status} |`);
      }
    }
    lines.push('', '</details>', '', `源码基点：\`${data.revision}\`；原生 diff：\`${data.sourceDiffSha256}\`。`, '');
    for (const [label, meta] of Object.entries(data.metadata)) {
      lines.push(`- ${label}：包 ${meta.packageVersion}；原生库 SHA256 \`${meta.librarySha256}\`。`);
    }
    lines.push('', `原始证据：[${path.basename(file)}](${path.resolve(file).replace(/\\/g, '/')})`, '');
  }
  const missing = [...platforms].filter((p) => !seen.has(p)).sort();
  const allPassed = passing.size === platforms.size && [...platforms].every((p) => passing.has(p));
  lines.splice(4, 0, `**总体验收：${allPassed && identities.size === 1 ? '通过' : '未通过'}。** 已验证通过 ${passing.size}/${platforms.size} 平台。缺少：${missing.join(', ') || '无'}。`);
  lines.push(
      '新增长图和分片能力还须分别通过 `check-bilibili`。新进程测试包含导入和启动，但操作系统文件缓存未清空；有限矩阵不能证明任意输入、任意机器和每一次调用均不变慢。', '');
  writeFileSync(output, lines.join('\n'));
  console.log(lines[4]);
  return allPassed && identities.size === 1;
}

const cli = cac('perf-report');
cli.command('<...files>', 'render the acceptance report from perf-compare results')
    .option('--output <file>', 'where the Markdown goes')
    .option('--platform <name>', 'an acceptance platform; repeat to require several (default: all six)')
    .action((files: string[], options: {output?: string; platform?: string | string[]}) => {
      if (!options.output) throw new Error('--output is required');
      const chosen = options.platform === undefined ? undefined : new Set(Array.isArray(options.platform) ? options.platform : [options.platform]);
      for (const p of chosen ?? []) if (!PLATFORMS.has(p)) throw new Error(`--platform must be one of ${[...PLATFORMS].sort().join(', ')}`);
      try {
        process.exitCode = report(files.map((f) => resolve(f)), resolve(options.output), chosen ?? PLATFORMS) ? 0 : 1;
      } catch (error) {
        console.error(error instanceof Error ? error.message : String(error));
        process.exitCode = 1;
      }
    });
cli.help();
cli.parse();
