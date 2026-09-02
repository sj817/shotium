import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';
import {renderArchivedResult} from '../src/render-report.ts';
import {
  BENCHMARK_SITE_URL, buildPlatformRanking, renderLatest, renderReport, renderSummaryCsv,
} from '../src/report.ts';

function distribution(p50: number) {
  return {n: 1, min: p50, p50, p95: p50, max: p50, mean: p50, mad: 0};
}

function row(engine: string, scenario: string, concurrency: number, p50: number,
    status = 'pass', rankingEligible = true) {
  return {
    engine, scenario, concurrency, status, ranking_eligible: rankingEligible,
    runs: 1, shots: 1, latency_ms: distribution(p50), wall_time_ms: distribution(p50),
    orchestration_wall_time_ms: distribution(p50), peak_rss_bytes: null,
    rss_slope_bytes_per_minute: null, throughput_per_second: 10, failure_rate: null,
  };
}

function platform(executionShardCount = 5) {
  return {
    platform: 'linux-x64',
    status: 'noisy',
    execution_shards: Array.from({length: executionShardCount}, (_, index) => ({shard: `legacy-${index}`})),
    engines: [
      {engine: 'shotium', status: 'pass', reason: null, architectures: ['x64'], executable: '/shotium.node'},
      {engine: 'browser-a', status: 'pass', reason: null, architectures: ['x64'], executable: '/browser-a'},
      {engine: 'browser-b', status: 'pass', reason: null, architectures: ['x64'], executable: '/browser-b'},
      {engine: 'browser-unavailable', status: 'n/a',
        reason: 'startup: the package has no native browser for this platform architecture',
        architectures: [], executable: null},
    ],
    scenarios: [
      row('shotium', 'warm', 1, 10),
      row('browser-a', 'warm', 1, 5),
      row('browser-b', 'warm', 1, 1),
      row('shotium', 'cold', 1, 20),
      row('browser-a', 'cold', 1, 10),
      row('browser-b', 'cold', 1, 40, 'noisy', true),
      row('shotium', 'parallel', 2, 10, 'fail', true),
      row('browser-a', 'parallel', 2, 1),
      row('shotium', 'resident', 1, 10),
      row('browser-a', 'resident', 1, 1, 'pass', false),
    ],
  };
}

const manifest = {
  shotium_version: '0.3.2', status: 'complete', quality_status: 'fail', evidence_status: 'complete',
  profile: 'smoke', seed: 'report-seed', platforms: [{platform: 'linux-x64', missing: false}],
};

test('platform ranking uses only paired pass and ranking-eligible cells', () => {
  const ranking = buildPlatformRanking(platform());
  assert.equal(ranking.total_cells, 2);
  assert.deepEqual(ranking.cells.map((cell) => cell.key), ['cold|1', 'warm|1']);
  assert.deepEqual(ranking.entries.map((entry) => entry.engine), ['browser-a', 'shotium', 'browser-b']);
  assert.equal(ranking.entries[0].geometric_mean_ratio, 0.5);
  assert.equal(ranking.entries[0].eligible_scenarios, 2);
  assert.equal(ranking.entries[0].eligible_cells, 2);
  assert.equal(ranking.entries[0].champion_count, 1);
  assert.equal(ranking.entries[1].geometric_mean_ratio, 1);
  assert.equal(ranking.entries[2].eligible_cells, 1);
  assert.ok(Math.abs(ranking.entries[2].geometric_mean_ratio - 0.1) < 1e-12);
  assert.equal(ranking.entries[2].champion_count, 1);
  assert.equal(ranking.entries[2].rank, null);
});

test('Chinese report localizes statuses, profile, scenarios and common reasons', () => {
  const chinese = renderReport([platform()], manifest, 'zh-CN');
  assert.match(chinese, new RegExp(BENCHMARK_SITE_URL.replaceAll('.', '\\.')));
  assert.match(chinese, /结果：\*\*完整\*\*/);
  assert.match(chinese, /质量：\*\*失败\*\*/);
  assert.match(chinese, /配置：\*\*冒烟测试\*\*/);
  assert.match(chinese, /## 六平台总览/);
  assert.match(chinese, /\| 平台 \| 质量状态 \| 正式第一名 \| 正式参赛引擎数 \| 平台可比项数 \|/);
  assert.match(chinese, /\| linux-x64 \| 噪声过大 \| 无有效排名 \| 0 \| 2 \|/);
  assert.match(chinese, /平台内综合排名/);
  assert.match(chinese, /本平台质量状态为“噪声过大”/);
  assert.match(chinese, /无有效排名/);
  assert.doesNotMatch(chinese, /结论：browser-a 在本平台排名第一/);
  assert.match(chinese, /\| 预热截图 \|/);
  assert.match(chinese, /启动场景：该软件包没有适用于此平台架构的原生浏览器/);
  assert.doesNotMatch(chinese, /\*\*complete\*\*|\*\*fail\*\*|\*\*smoke\*\*/);

  const english = renderReport([{...platform(), status: 'pass'}], manifest, 'en');
  assert.match(english, /Within-platform ranking/);
  assert.match(english, /Six-platform overview/);
  assert.match(english, /lower is better/);
  assert.match(english, /\| warm \|/);
});

test('overview names missing platforms without inventing a ranking', () => {
  const report = renderReport([platform()], {...manifest, platforms: [
    {platform: 'linux-x64', missing: false, status: 'noisy'},
    {platform: 'win32-arm64', missing: true, status: 'infra-error'},
  ]}, 'zh-CN');
  assert.match(report, /\| win32-arm64 \| 基础设施错误 \| 无有效排名 \| 0 \| 0 \|/);
});

test('latest report keeps machine-readable status and adds Chinese status values', () => {
  const latest = renderLatest({
    path: 'v0.3.2/run', shotium_version: '0.3.2', generated_utc: '2026-08-30T00:00:00Z',
    status: 'complete', quality_status: 'fail', evidence_status: 'complete',
  });
  assert.match(latest, /status complete \/ 状态 完整/);
  assert.match(latest, /quality fail \/ 质量 失败/);
  assert.match(latest, /evidence complete \/ 证据 完整/);
});

test('CSV exposes strict paired eligibility and normalized ratios', () => {
  const csv = renderSummaryCsv([{...platform(), status: 'pass'}]);
  assert.match(csv, /paired_ranking_eligible/);
  assert.match(csv, /browser-a,warm,1,pass,true,true/);
  assert.match(csv, /browser-a,parallel,2,pass,true,false/);

  const untrusted = renderSummaryCsv([platform()]);
  assert.match(untrusted, /browser-a,warm,1,pass,true,false/);
  assert.doesNotMatch(untrusted, /browser-a,warm,1,pass,true,true/);

  const missingEvidence = renderSummaryCsv([{...platform(), status: 'pass'}], {
    platforms: [{platform: 'linux-x64', missing: false, evidence_complete: false}],
  });
  assert.match(missingEvidence, /browser-a,warm,1,pass,true,false/);
  assert.doesNotMatch(missingEvidence, /browser-a,warm,1,pass,true,true/);
});

for (const shardCount of [4, 5]) {
  test(`safely re-renders an archived ${shardCount}-shard result without current-schema migration`, () => {
    const root = fs.mkdtempSync(path.join(os.tmpdir(), `shotium-render-${shardCount}-`));
    try {
      const resultsRoot = path.join(root, 'benchmark-results');
      const directory = path.join(resultsRoot, 'v0.3.2', `run-${shardCount}`);
      fs.mkdirSync(path.join(directory, 'linux-x64'), {recursive: true});
      fs.writeFileSync(path.join(directory, 'manifest.json'), `${JSON.stringify(manifest)}\n`);
      fs.writeFileSync(path.join(directory, 'linux-x64', 'summary.json'),
          `${JSON.stringify(platform(shardCount))}\n`);
      for (const name of ['report.md', 'report.zh-CN.md', 'summary.csv']) {
        fs.writeFileSync(path.join(directory, name), 'old generated content\n');
      }
      fs.writeFileSync(path.join(resultsRoot, 'index.json'), `${JSON.stringify({results: [{
        path: `v0.3.2/run-${shardCount}`, shotium_version: '0.3.2', generated_utc: '2026-08-30T00:00:00Z',
        status: 'complete', quality_status: 'fail', evidence_status: 'complete',
      }, {
        path: 'v0.3.1/valid-run', shotium_version: '0.3.1', generated_utc: '2026-08-29T00:00:00Z',
        status: 'complete', quality_status: 'pass', evidence_status: 'complete',
      }]})}\n`);
      const result = renderArchivedResult(directory);
      assert.deepEqual(result.platforms, ['linux-x64']);
      assert.match(fs.readFileSync(path.join(directory, 'report.md'), 'utf8'), /Within-platform ranking/);
      assert.match(fs.readFileSync(path.join(directory, 'report.zh-CN.md'), 'utf8'), /平台内综合排名/);
      assert.match(fs.readFileSync(path.join(directory, 'summary.csv'), 'utf8'), /paired_ranking_eligible/);
      assert.match(fs.readFileSync(path.join(resultsRoot, 'LATEST.md'), 'utf8'),
          /Interactive benchmark explorer \/ 交互式基准站点/);
      assert.match(fs.readFileSync(path.join(resultsRoot, 'LATEST.md'), 'utf8'),
          /v0\.3\.1\/valid-run\/report\.md/);
      assert.doesNotMatch(fs.readFileSync(path.join(resultsRoot, 'LATEST.md'), 'utf8'),
          new RegExp(`v0\\.3\\.2/run-${shardCount}/report\\.md`));
      assert.equal(fs.readdirSync(directory).some((name) => name.includes('.tmp-') || name.includes('.bak-')), false);
    } finally {
      fs.rmSync(root, {recursive: true, force: true});
    }
  });
}
