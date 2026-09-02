type ReportLocale = 'en' | 'zh-CN';

export const BENCHMARK_SITE_URL = 'https://sj817.github.io/shotium/';

export function isPublishableResult(result: any): boolean {
  return result?.status === 'complete' && result?.quality_status === 'pass' &&
    result?.evidence_status === 'complete';
}

type ComparisonEntry = {
  rank: number | null;
  engine: string;
  geometric_mean_ratio: number;
  eligible_scenarios: number;
  eligible_cells: number;
  total_cells: number;
  champion_count: number;
  cell_keys: string[];
};

type ComparisonCell = {
  key: string;
  scenario: string;
  concurrency: number;
  ratios: Map<string, number>;
};

export type PlatformRanking = {
  platform: string;
  total_cells: number;
  cells: ComparisonCell[];
  entries: ComparisonEntry[];
};

const STATUS_ZH = Object.freeze({
  pass: '通过',
  fail: '失败',
  noisy: '噪声过大',
  'n/a': '不适用',
  'infra-error': '基础设施错误',
  complete: '完整',
  incomplete: '不完整',
  unknown: '未知',
});

const PROFILE_ZH = Object.freeze({
  smoke: '冒烟测试',
  full: '完整测试',
  unknown: '未知配置',
});

const SCENARIO_ZH = Object.freeze({
  cold: '冷启动',
  'cold-settled': '冷启动稳定后首张',
  warm: '预热截图',
  batch: '顺序批量',
  parallel: '并发截图',
  resident: '常驻引擎新客户端',
  'reuse-page': '复用页面',
  lifecycle: '启动—截图—关闭循环',
  faults: '异常与恢复',
  soak: '持续压力',
});

const SHARD_ZH = Object.freeze({
  startup: '启动场景',
  throughput: '吞吐场景',
  parallel: '并发场景',
  resident: '常驻场景',
  resilience: '韧性场景',
});

const REASON_ZH: Array<[string | RegExp, string]> = [
  ['the package has no native browser for this platform architecture', '该软件包没有适用于此平台架构的原生浏览器'],
  ['the package currently supplies an x64 browser on Windows arm64', '该软件包目前在 Windows arm64 上仅提供 x64 浏览器'],
  ['package browser or addon is not installed', '软件包浏览器或原生扩展未安装'],
  ['binary format or architecture could not be identified', '无法识别二进制格式或架构'],
  ['native binary version could not be read', '无法读取原生二进制版本'],
  ['engine availability differs between scenario shards', '各场景分片的引擎可用性不一致'],
  ['engine binary identity differs between scenario shards', '各场景分片的引擎二进制身份不一致'],
  [/binary architecture ([^;]+?) is not ([^;]+)/g, '二进制架构 $1 与要求的 $2 不一致'],
  [/not installed in merger fixture/g, '合并测试夹具中未安装'],
];

function finitePositive(value: unknown): value is number {
  return typeof value === 'number' && Number.isFinite(value) && value > 0;
}

function elapsedP50(row: any): number | null {
  const value = row?.latency_ms?.p50 ?? row?.wall_time_ms?.p50;
  return finitePositive(value) ? value : null;
}

function eligible(row: any): boolean {
  return row?.status === 'pass' && row?.ranking_eligible === true && elapsedP50(row) !== null;
}

function cellKey(row: any): string {
  return `${row.scenario}|${row.concurrency}`;
}

function displayCellKey(cell: {scenario: string; concurrency: number}, locale: ReportLocale): string {
  const scenario = locale === 'zh-CN' ? scenarioLabel(cell.scenario, locale) : cell.scenario;
  return locale === 'zh-CN' ? `${scenario}/并发${cell.concurrency}` : `${scenario}/c${cell.concurrency}`;
}

export function buildPlatformRanking(platform: any): PlatformRanking {
  const rows = Array.isArray(platform?.scenarios) ? platform.scenarios : [];
  const grouped = new Map<string, any[]>();
  for (const row of rows) {
    const key = cellKey(row);
    const bucket = grouped.get(key) || [];
    bucket.push(row);
    grouped.set(key, bucket);
  }

  const cells: ComparisonCell[] = [];
  for (const [key, bucket] of grouped) {
    const baseline = bucket.find((row) => row.engine === 'shotium' && eligible(row));
    const baselineP50 = elapsedP50(baseline);
    if (!baseline || baselineP50 === null) continue;
    const competitors = bucket.filter((row) => row.engine !== 'shotium' && eligible(row));
    if (!competitors.length) continue;
    const ratios = new Map<string, number>([['shotium', 1]]);
    for (const row of competitors) {
      const value = elapsedP50(row);
      const ratio = value === null ? null : value / baselineP50;
      if (ratio !== null && finitePositive(ratio)) ratios.set(row.engine, ratio);
    }
    cells.push({
      key,
      scenario: baseline.scenario,
      concurrency: Number(baseline.concurrency),
      ratios,
    });
  }
  cells.sort((left, right) => left.scenario.localeCompare(right.scenario) || left.concurrency - right.concurrency);

  const engines = new Set(cells.flatMap((cell) => [...cell.ratios.keys()]));
  const entries = [...engines].map((engine) => {
    const covered = cells.filter((cell) => cell.ratios.has(engine));
    const ratios = covered.map((cell) => cell.ratios.get(engine)!);
    const championCount = covered.filter((cell) => {
      const best = Math.min(...cell.ratios.values());
      return Math.abs(cell.ratios.get(engine)! - best) <= Number.EPSILON * Math.max(1, best) * 8;
    }).length;
    return {
      rank: 0,
      engine,
      geometric_mean_ratio: Math.exp(ratios.reduce((sum, ratio) => sum + Math.log(ratio), 0) / ratios.length),
      eligible_scenarios: new Set(covered.map((cell) => cell.scenario)).size,
      eligible_cells: covered.length,
      total_cells: cells.length,
      champion_count: championCount,
      cell_keys: covered.map((cell) => cell.key),
    };
  }).sort((left, right) => {
    const leftComplete = left.eligible_cells === left.total_cells;
    const rightComplete = right.eligible_cells === right.total_cells;
    return Number(rightComplete) - Number(leftComplete) ||
    (leftComplete && rightComplete ? left.geometric_mean_ratio - right.geometric_mean_ratio : 0) ||
    right.eligible_cells - left.eligible_cells ||
    left.geometric_mean_ratio - right.geometric_mean_ratio ||
    right.champion_count - left.champion_count ||
    left.engine.localeCompare(right.engine);
  });

  let previousRatio: number | null = null;
  let previousRank = 0;
  let completeIndex = 0;
  entries.forEach((entry) => {
    if (entry.eligible_cells !== entry.total_cells) {
      entry.rank = null;
      return;
    }
    const tied = previousRatio !== null &&
      Math.abs(entry.geometric_mean_ratio - previousRatio) <= Number.EPSILON * Math.max(1, previousRatio) * 8;
    completeIndex += 1;
    entry.rank = tied ? previousRank : completeIndex;
    previousRatio = entry.geometric_mean_ratio;
    previousRank = entry.rank;
  });
  return {platform: String(platform?.platform || 'unknown'), total_cells: cells.length, cells, entries};
}

function statusLabel(status: unknown, locale: ReportLocale): string {
  const value = String(status ?? 'unknown');
  return locale === 'zh-CN' ? STATUS_ZH[value] || value : value;
}

function profileLabel(profile: unknown, locale: ReportLocale): string {
  const value = String(profile ?? 'unknown');
  return locale === 'zh-CN' ? PROFILE_ZH[value] || value : value;
}

function scenarioLabel(scenario: unknown, locale: ReportLocale): string {
  const value = String(scenario ?? 'unknown');
  return locale === 'zh-CN' ? SCENARIO_ZH[value] || value : value;
}

function localizeReason(reason: unknown, locale: ReportLocale): string {
  let text = String(reason || '');
  if (locale !== 'zh-CN') return text;
  for (const [shard, label] of Object.entries(SHARD_ZH)) {
    text = text.replace(new RegExp(`(^|;\\s*)${shard}:`, 'g'), `$1${label}：`);
  }
  for (const [source, replacement] of REASON_ZH) text = text.replaceAll(source as any, replacement);
  return text.replace(/;\s*/g, '；').replace(/：\s+/g, '：');
}

function csv(value: unknown): string {
  const text = value === null || value === undefined ? '' : String(value);
  return /[",\r\n]/.test(text) ? `"${text.replaceAll('"', '""')}"` : text;
}

function comparisonLookup(ranking: PlatformRanking): Map<string, number> {
  const lookup = new Map<string, number>();
  for (const cell of ranking.cells) {
    for (const [engine, ratio] of cell.ratios) lookup.set(`${engine}|${cell.key}`, ratio);
  }
  return lookup;
}

function platformRankingAllowed(platform: any, manifestPlatform: any = null): boolean {
  return platform?.status === 'pass' && platform?.shards_complete !== false &&
    manifestPlatform?.missing !== true && manifestPlatform?.shards_complete !== false &&
    manifestPlatform?.evidence_complete !== false;
}

export function renderSummaryCsv(platforms: any[], manifest: any = null): string {
  const header = 'platform,platform_status,shard,engine,scenario,concurrency,status,ranking_eligible,paired_ranking_eligible,runs,shots,p50_ms,p95_ms,worst_ms,mad_ms,throughput_per_second,failure_rate,rss_slope_bytes_per_minute,ratio_to_shotium';
  const lines = [header];
  const manifestPlatforms = Array.isArray(manifest?.platforms) ? manifest.platforms : [];
  const manifestPlatformById = new Map(manifestPlatforms.map((entry: any) => [entry.platform, entry]));
  for (const platform of platforms) {
    const ranking = buildPlatformRanking(platform);
    const ratios = platformRankingAllowed(platform, manifestPlatformById.get(platform.platform)) ?
      comparisonLookup(ranking) : new Map<string, number>();
    for (const scenario of platform.scenarios || []) {
      const p50 = scenario.latency_ms?.p50 ?? scenario.wall_time_ms?.p50 ?? null;
      const ratio = ratios.get(`${scenario.engine}|${cellKey(scenario)}`) ?? null;
      lines.push([
        platform.platform, platform.status, scenario.shard || 'all', scenario.engine, scenario.scenario,
        scenario.concurrency, scenario.status, scenario.ranking_eligible, ratio !== null,
        scenario.runs, scenario.shots, p50, scenario.latency_ms?.p95 ?? null,
        scenario.latency_ms?.max ?? scenario.wall_time_ms?.max ?? null,
        scenario.latency_ms?.mad ?? null, scenario.throughput_per_second ?? null,
        scenario.failure_rate ?? null, scenario.rss_slope_bytes_per_minute?.p50 ?? null,
        ratio === null ? null : Number(ratio.toFixed(4)),
      ].map(csv).join(','));
    }
  }
  return `${lines.join('\n')}\n`;
}

const COPY = {
  en: {
    title: (version: string) => `# Shotium ${version} benchmark`,
    result: (manifest: any) =>
      `Result: **${statusLabel(manifest.status, 'en')}**; quality: **${statusLabel(manifest.quality_status, 'en')}**; ` +
      `evidence: **${statusLabel(manifest.evidence_status, 'en')}**. ` +
      `Profile **${profileLabel(manifest.profile, 'en')}**, seed \`${manifest.seed}\`.`,
    conclusion: (manifest: any, rankedPlatforms: number) => manifest.status !== 'complete' ?
      `Conclusion: this run is incomplete. ${rankedPlatforms} platform(s) contain valid within-platform comparisons; missing outputs are never inferred.` :
      manifest.quality_status === 'pass' ?
        `Conclusion: all platform outputs and evidence are complete. ${rankedPlatforms} platform(s) contain valid within-platform comparisons.` :
        `Conclusion: all platform outputs exist, but quality is ${manifest.quality_status}. Only rows marked pass and ranking-eligible are used; ${rankedPlatforms} platform(s) contain valid comparisons.`,
    ratios: 'Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.',
    overviewHeading: '## Six-platform overview',
    overviewHeader: '| platform | quality status | formal winner | formally ranked engines | comparable cells |',
    overviewAlign: '|:--|:--|:--|--:|--:|',
    noValidRanking: 'no valid ranking',
    sharded: 'Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.',
    rankingHeading: '### Within-platform ranking',
    rankingRule: 'Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.',
    rankingHeader: '| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |',
    rankingAlign: '|--:|:--|--:|--:|--:|--:|',
    coverageHeading: 'Coverage audit',
    noRanking: 'Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.',
    platformQualityNoRanking: (status: string) =>
      `Conclusion: platform quality is ${status}; its measurements remain available for diagnosis, but no formal ranking or winner is produced.`,
    platformConclusion: (winners: string, ratio: string, cells: number, wins: number) =>
      `Conclusion: ${winners} ranks first on this platform at ${ratio}× normalized elapsed time across ${cells} eligible cell(s), with ${wins} win(s).`,
    engineHeader: '| engine | availability | reason / binary architecture |',
    scenarioHeader: '| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |',
    ranked: (value: boolean) => value ? 'yes' : 'no',
    missingHeading: '## Missing platform outputs',
    notAvailable: 'N/A',
  },
  'zh-CN': {
    title: (version: string) => `# Shotium ${version} 基准报告`,
    result: (manifest: any) =>
      `结果：**${statusLabel(manifest.status, 'zh-CN')}**；质量：**${statusLabel(manifest.quality_status, 'zh-CN')}**；` +
      `证据：**${statusLabel(manifest.evidence_status, 'zh-CN')}**。` +
      `配置：**${profileLabel(manifest.profile, 'zh-CN')}**；种子：\`${manifest.seed}\`。`,
    conclusion: (manifest: any, rankedPlatforms: number) => manifest.status !== 'complete' ?
      `结论：本次结果不完整。${rankedPlatforms} 个平台形成了有效的平台内对比；缺失结果不会被推测或补齐。` :
      manifest.quality_status === 'pass' ?
        `结论：六个平台结果与证据完整，${rankedPlatforms} 个平台形成了有效的平台内对比。` :
        `结论：六个平台均已产出，但质量判定为“${statusLabel(manifest.quality_status, 'zh-CN')}”。综合排名只采用状态为“通过”且允许排名的数据；${rankedPlatforms} 个平台形成了有效对比。`,
    ratios: '每个比率只在 Shotium 与对比引擎位于同一平台、同一场景、同一并发度，且双方状态均为“通过”并允许排名时计算。本报告不进行跨平台混排。',
    overviewHeading: '## 六平台总览',
    overviewHeader: '| 平台 | 质量状态 | 正式第一名 | 正式参赛引擎数 | 平台可比项数 |',
    overviewAlign: '|:--|:--|:--|--:|--:|',
    noValidRanking: '无有效排名',
    sharded: '场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。',
    rankingHeading: '### 平台内综合排名',
    rankingRule: '每个合格测试项先以 Shotium = 1.000 归一化；**相对耗时越低越好**。综合分仅在本平台内取几何平均。只有覆盖本平台全部可比项的引擎才获得正式名次；部分覆盖仍展示成绩，但会标记为“覆盖不完整，不授予名次”。单项并列最快时各记一次冠军。',
    rankingHeader: '| 名次 | 引擎 | 相对耗时几何平均 | 参与排名场景数 | 参与项 / 平台可比项 | 冠军次数 |',
    rankingAlign: '|--:|:--|--:|--:|--:|--:|',
    coverageHeading: '覆盖项审计',
    noRanking: '结论：没有任何场景/并发项同时具备合格的 Shotium 与竞品结果，因此不生成综合排名。',
    platformQualityNoRanking: (status: string) =>
      `结论：本平台质量状态为“${status}”；测量数据仅保留用于诊断，不生成正式排名或第一名。`,
    platformConclusion: (winners: string, ratio: string, cells: number, wins: number) =>
      `结论：${winners} 在本平台排名第一，相对耗时为 ${ratio}×，覆盖 ${cells} 个合格测试项，获得 ${wins} 次单项冠军。`,
    engineHeader: '| 引擎 | 可用性 | 原因 / 二进制架构 |',
    scenarioHeader: '| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |',
    ranked: (value: boolean) => value ? '是' : '否',
    missingHeading: '## 缺失的平台输出',
    notAvailable: '不适用',
  },
} as const;

export function renderReport(platforms: any[], manifest: any, locale: ReportLocale): string {
  const copy = COPY[locale];
  const rankings = new Map(platforms.map((platform) => [platform.platform, buildPlatformRanking(platform)]));
  const platformById = new Map(platforms.map((platform) => [platform.platform, platform]));
  const manifestPlatforms = Array.isArray(manifest.platforms) ? manifest.platforms : [];
  const manifestPlatformById = new Map(manifestPlatforms.map((entry: any) => [entry.platform, entry]));
  const rankedPlatforms = platforms.filter((platform) => {
    const ranking = rankings.get(platform.platform);
    return platformRankingAllowed(platform, manifestPlatformById.get(platform.platform)) &&
      (ranking?.entries.filter((entry) => entry.rank !== null).length || 0) >= 2;
  }).length;
  const lines = [
    copy.title(manifest.shotium_version), '',
    locale === 'zh-CN' ? `[交互式基准站点](${BENCHMARK_SITE_URL})` :
      `[Interactive benchmark explorer](${BENCHMARK_SITE_URL})`, '',
    copy.result(manifest), '',
    copy.conclusion(manifest, rankedPlatforms), '', copy.ratios, '',
  ];
  const overviewPlatforms = [...new Set([
    ...manifestPlatforms.map((entry: any) => String(entry.platform || '')).filter(Boolean),
    ...platforms.map((platform) => String(platform.platform)),
  ])];
  lines.push(copy.overviewHeading, '', copy.overviewHeader, copy.overviewAlign);
  for (const platformId of overviewPlatforms) {
    const platform = platformById.get(platformId);
    const manifestPlatform = manifestPlatforms.find((entry: any) => entry.platform === platformId);
    const ranking = rankings.get(platformId);
    const allowed = platformRankingAllowed(platform, manifestPlatform);
    const formalEntries = allowed ? ranking?.entries.filter((entry) => entry.rank !== null) || [] : [];
    const winners = formalEntries.filter((entry) => entry.rank === 1).map((entry) => entry.engine);
    const winner = winners.length ? winners.join(locale === 'zh-CN' ? '、' : ', ') : copy.noValidRanking;
    lines.push(`| ${platformId} | ${statusLabel(platform?.status ?? manifestPlatform?.status, locale)} | ` +
      `${winner} | ${formalEntries.length} | ${ranking?.total_cells || 0} |`);
  }
  lines.push('');
  for (const platform of platforms) {
    const ranking = rankings.get(platform.platform)!;
    const allowed = platformRankingAllowed(platform, manifestPlatformById.get(platform.platform));
    const ratios = allowed ? comparisonLookup(ranking) : new Map<string, number>();
    lines.push(`## ${platform.platform}`, '');
    if (platform.execution_shards) lines.push(copy.sharded, '');
    if (!allowed) {
      lines.push(copy.rankingHeading, '',
          copy.platformQualityNoRanking(statusLabel(platform.status, locale)), '');
    } else if (!ranking.entries.length) {
      lines.push(copy.rankingHeading, '', copy.noRanking, '');
    } else {
      const winners = ranking.entries.filter((entry) => entry.rank === 1);
      lines.push(copy.rankingHeading, '', copy.rankingRule, '',
          copy.platformConclusion(
              winners.map((entry) => entry.engine).join('、'),
              winners[0].geometric_mean_ratio.toFixed(3),
              winners[0].eligible_cells,
              winners[0].champion_count), '',
          copy.rankingHeader, copy.rankingAlign);
      for (const entry of ranking.entries) {
        const rank = entry.rank ?? (locale === 'zh-CN' ? '不授予名次（覆盖不完整）' : 'not ranked (partial coverage)');
        lines.push(`| ${rank} | ${entry.engine} | ${entry.geometric_mean_ratio.toFixed(3)}× | ` +
          `${entry.eligible_scenarios} | ${entry.eligible_cells} / ${entry.total_cells} | ${entry.champion_count} |`);
      }
      lines.push('', `<details><summary>${copy.coverageHeading}</summary>`, '');
      for (const entry of ranking.entries) {
        const covered = ranking.cells.filter((cell) => entry.cell_keys.includes(cell.key))
            .map((cell) => `\`${displayCellKey(cell, locale)}\``).join(locale === 'zh-CN' ? '、' : ', ');
        lines.push(`- ${entry.engine}: ${covered}`);
      }
      lines.push('', '</details>', '');
    }
    lines.push(copy.engineHeader, '|:--|:--|:--|');
    for (const engine of platform.engines || []) {
      const detail = engine.reason ? localizeReason(engine.reason, locale) :
        `${(engine.architectures || []).join('+') || (locale === 'zh-CN' ? '未知' : 'unknown')}; ${engine.executable || ''}`;
      lines.push(`| ${engine.engine} | ${statusLabel(engine.status, locale)} | ${String(detail).replaceAll('|', '\\|')} |`);
    }
    lines.push('', copy.scenarioHeader, '|:--|--:|:--|:--|:--|--:|--:|--:|--:|');
    for (const row of platform.scenarios || []) {
      const p50 = row.latency_ms?.p50 ?? row.wall_time_ms?.p50;
      const worst = row.latency_ms?.max ?? row.wall_time_ms?.max;
      const ratio = ratios.get(`${row.engine}|${cellKey(row)}`);
      lines.push(`| ${scenarioLabel(row.scenario, locale)} | ${row.concurrency} | ${row.engine} | ` +
        `${statusLabel(row.status, locale)} | ${copy.ranked(ratio !== undefined)} | ` +
        `${p50 ?? copy.notAvailable} | ${worst ?? copy.notAvailable} | ` +
        `${row.throughput_per_second?.toFixed?.(3) ?? copy.notAvailable} | ` +
        `${ratio === undefined ? copy.notAvailable : `${ratio.toFixed(2)}×`} |`);
    }
    lines.push('');
  }
  const missing = (manifest.platforms || []).filter((entry: any) => entry.missing)
      .map((entry: any) => entry.platform);
  if (missing.length) lines.push(copy.missingHeading, '', missing.map((item: string) => `- ${item}`).join('\n'), '');
  return `${lines.join('\n')}\n`;
}

export function renderLatest(latest: any): string {
  if (!latest) {
    return `# Latest benchmark / 最新基准\n\n` +
      `[Interactive benchmark explorer / 交互式基准站点](${BENCHMARK_SITE_URL})\n\n` +
      `No publishable benchmark result yet; failed and noisy runs remain available in the archive. / ` +
      `暂无可发布的有效基准结果；失败与波动运行仍保留在归档中。\n`;
  }
  return `# Latest benchmark / 最新基准\n\n` +
    `[Interactive benchmark explorer / 交互式基准站点](${BENCHMARK_SITE_URL})\n\n` +
    `${latest.shotium_version} · ${latest.generated_utc}: ` +
    `[English](${latest.path}/report.md) · ` +
    `[简体中文](${latest.path}/report.zh-CN.md) ` +
    `— status ${latest.status} / 状态 ${statusLabel(latest.status, 'zh-CN')}, ` +
    `quality ${latest.quality_status} / 质量 ${statusLabel(latest.quality_status, 'zh-CN')}, ` +
    `evidence ${latest.evidence_status} / 证据 ${statusLabel(latest.evidence_status, 'zh-CN')}.\n`;
}
