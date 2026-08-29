<script setup lang="ts">
import {computed, onMounted, ref, watch} from 'vue';
import {useI18n} from 'vue-i18n';
import {loadIndex, loadRun} from '../lib/data';
import {rankPlatform, ratioForScenario, scenarioP50, type RankingExclusion} from '../lib/ranking';
import {
  PLATFORM_IDS,
  type BenchmarkIndex,
  type LoadedRun,
  type PlatformId,
  type ResultStatus,
  type ScenarioSummary,
} from '../lib/types';

const {locale, t} = useI18n();
const index = ref<BenchmarkIndex | null>(null);
const selectedPath = ref('');
const run = ref<LoadedRun | null>(null);
const selectedPlatform = ref<PlatformId>('linux-x64');
const loading = ref(true);
const error = ref('');
const engineFilter = ref('all');
const scenarioFilter = ref('all');
const statusFilter = ref('all');
let loadSequence = 0;

const platformTranslationKeys: Record<PlatformId, string> = {
  'linux-x64': 'platform.linuxX64',
  'linux-arm64': 'platform.linuxArm64',
  'win32-x64': 'platform.win32X64',
  'win32-arm64': 'platform.win32Arm64',
  'darwin-x64': 'platform.darwinX64',
  'darwin-arm64': 'platform.darwinArm64',
};

const scenarioTranslationKeys: Record<string, string> = {
  cold: 'scenario.cold',
  'cold-settled': 'scenario.coldSettled',
  warm: 'scenario.warm',
  batch: 'scenario.batch',
  parallel: 'scenario.parallel',
  resident: 'scenario.resident',
  'reuse-page': 'scenario.reusePage',
  lifecycle: 'scenario.lifecycle',
  faults: 'scenario.faults',
  soak: 'scenario.soak',
};

const shardTranslationKeys: Record<string, string> = {
  startup: 'shard.startup',
  throughput: 'shard.throughput',
  parallel: 'shard.parallel',
  resident: 'shard.resident',
  resilience: 'shard.resilience',
};

const selectedData = computed(() => run.value?.platforms[selectedPlatform.value] ?? null);
const selectedSummary = computed(() => selectedData.value?.summary ?? null);
const ranking = computed(() => selectedSummary.value ? rankPlatform(selectedSummary.value) : []);
const rankNumbers = computed(() => {
  let position = 0;
  let previousRank = 0;
  let previousScore: number | null = null;
  return new Map(ranking.value.map((row) => {
    if (!row.official || row.geometricMean === null) return [row.engine, null] as const;
    position += 1;
    const tied = previousScore !== null && Math.abs(row.geometricMean - previousScore) <= 1e-9;
    const rank = tied ? previousRank : position;
    previousRank = rank;
    previousScore = row.geometricMean;
    return [row.engine, rank] as const;
  }));
});
const engineOptions = computed(() => selectedSummary.value ?
  [...new Set(selectedSummary.value.scenarios.map((row) => row.engine))].sort() : []);
const scenarioOptions = computed(() => selectedSummary.value ?
  [...new Set(selectedSummary.value.scenarios.map((row) => row.scenario))].sort() : []);
const filteredScenarios = computed(() => (selectedSummary.value?.scenarios ?? []).filter((row) =>
  (engineFilter.value === 'all' || row.engine === engineFilter.value) &&
  (scenarioFilter.value === 'all' || row.scenario === scenarioFilter.value) &&
  (statusFilter.value === 'all' || row.status === statusFilter.value)));
const unhealthyScenarios = computed(() =>
  (selectedSummary.value?.scenarios ?? []).filter((row) => row.status !== 'pass'));
const excludedEngines = computed(() =>
  (selectedSummary.value?.engines ?? []).filter((engine) => engine.status === 'n/a' || engine.reason));
const availableEngineCount = computed(() =>
  (selectedSummary.value?.engines ?? []).filter((engine) => engine.status !== 'n/a').length);

const platformCounts = computed(() => {
  const counts = {pass: 0, noisy: 0, fail: 0, infra: 0, missing: 0};
  if (!run.value) return counts;
  for (const id of PLATFORM_IDS) {
    const data = run.value.platforms[id];
    if (!data.summary) counts.missing += 1;
    else if (data.summary.status === 'pass') counts.pass += 1;
    else if (data.summary.status === 'noisy') counts.noisy += 1;
    else if (data.summary.status === 'fail') counts.fail += 1;
    else if (data.summary.status === 'infra-error') counts.infra += 1;
  }
  return counts;
});
const platformOverviews = computed(() => PLATFORM_IDS.map((id) => {
  const summary = run.value?.platforms[id].summary ?? null;
  const rows = summary ? rankPlatform(summary) : [];
  const official = rows.filter((row) => row.official && row.geometricMean !== null);
  const best = official[0]?.geometricMean ?? null;
  const winners = best === null ? [] : official
      .filter((row) => Math.abs(row.geometricMean! - best) <= 1e-9)
      .map((row) => row.engine);
  return {
    id,
    status: summary?.status ?? platformStatus(id),
    winners,
    ranked: official.length,
    cells: official[0]?.totalBaselines ?? 0,
  };
}));

function statusKey(status: string | null | undefined): string {
  switch (status) {
    case 'pass': return 'status.pass';
    case 'fail': return 'status.fail';
    case 'noisy': return 'status.noisy';
    case 'n/a': return 'status.na';
    case 'infra-error': return 'status.infraError';
    case 'complete': return 'status.complete';
    case 'incomplete': return 'status.incomplete';
    default: return 'status.unknown';
  }
}

function statusLabel(status: string | null | undefined): string {
  return t(statusKey(status));
}

function scenarioLabel(scenario: string | null | undefined): string {
  const value = String(scenario ?? '');
  const key = scenarioTranslationKeys[value];
  return key ? t(key) : value || '—';
}

function shardLabel(shard: string | null | undefined): string {
  const value = String(shard ?? '');
  const key = shardTranslationKeys[value];
  return key ? t(key) : value || '—';
}

function profileLabel(profile: string | null | undefined): string {
  if (profile === 'smoke') return t('profile.smoke');
  if (profile === 'full') return t('profile.full');
  return profile || t('status.unknown');
}

function reasonLabel(reason: string | null | undefined): string {
  if (!reason || locale.value === 'en') return reason || '—';
  let text = reason;
  for (const [shard, key] of Object.entries(shardTranslationKeys)) {
    text = text.replace(new RegExp(`(^|;\\s*)${shard}:`, 'g'), `$1${t(key)}：`);
  }
  const replacements: Array<[string | RegExp, string]> = [
    ['the package has no native browser for this platform architecture', '该软件包没有适用于此平台架构的原生浏览器'],
    ['the package currently supplies an x64 browser on Windows arm64', '该软件包目前在 Windows ARM64 上仅提供 x64 浏览器'],
    ['package browser or addon is not installed', '软件包浏览器或原生扩展未安装'],
    ['binary format or architecture could not be identified', '无法识别二进制格式或架构'],
    ['native binary version could not be read', '无法读取原生二进制版本'],
    ['engine availability differs between scenario shards', '各场景分片的引擎可用性不一致'],
    ['engine binary identity differs between scenario shards', '各场景分片的引擎二进制身份不一致'],
    [/binary architecture ([^;]+?) is not ([^;]+)/g, '二进制架构 $1 与要求的 $2 不一致'],
  ];
  for (const [source, replacement] of replacements) {
    text = typeof source === 'string' ? text.split(source).join(replacement) : text.replace(source, replacement);
  }
  return text.replace(/;\s*/g, '；').replace(/：\s+/g, '：');
}

function statusClass(status: string | null | undefined): string {
  return `status-${String(status ?? 'unknown').replace(/[^a-z]/g, '-')}`;
}

function platformStatus(id: PlatformId): string {
  return run.value?.platforms[id].summary?.status ??
    run.value?.manifest.platforms.find((platform) => platform.platform === id)?.status ?? 'missing';
}

function formatDate(value: string): string {
  const date = new Date(value);
  if (!Number.isFinite(date.getTime())) return value;
  return new Intl.DateTimeFormat(locale.value === 'en' ? 'en-US' : 'zh-CN', {
    dateStyle: 'medium', timeStyle: 'short', hour12: false,
  }).format(date);
}

function formatNumber(value: number | null | undefined, digits = 2): string {
  if (!Number.isFinite(value)) return '—';
  return new Intl.NumberFormat(locale.value === 'en' ? 'en-US' : 'zh-CN', {
    maximumFractionDigits: digits,
  }).format(Number(value));
}

function formatMilliseconds(value: number | null | undefined): string {
  return Number.isFinite(value) ? `${formatNumber(value, 2)} ms` : '—';
}

function formatRatio(value: number | null): string {
  return value === null ? '—' : `${formatNumber(value, 3)}×`;
}

function rowP95(row: ScenarioSummary): number | null {
  return row.latency_ms?.p95 ?? row.wall_time_ms?.p95 ?? null;
}

function rowWorst(row: ScenarioSummary): number | null {
  return row.latency_ms?.max ?? row.wall_time_ms?.max ?? null;
}

function exclusionText(exclusion: RankingExclusion): string {
  const keys = {
    'engine-unavailable': 'ranking.exclusions.engineUnavailable',
    'missing-baseline': 'ranking.exclusions.missingBaseline',
    'missing-cell': 'ranking.exclusions.missingCell',
    'invalid-cell': 'ranking.exclusions.invalidCell',
    'partial-coverage': 'ranking.exclusions.partialCoverage',
  } as const;
  const text = t(keys[exclusion.code], {count: exclusion.count});
  return exclusion.detail ? `${text}：${reasonLabel(exclusion.detail)}` : text;
}

function switchLanguage(): void {
  locale.value = locale.value === 'en' ? 'zh-CN' : 'en';
}

async function refreshRun(): Promise<void> {
  const entry = index.value?.results.find((candidate) => candidate.path === selectedPath.value);
  if (!entry) return;
  const sequence = ++loadSequence;
  loading.value = true;
  error.value = '';
  try {
    const next = await loadRun(entry);
    if (sequence !== loadSequence) return;
    run.value = next;
    engineFilter.value = 'all';
    scenarioFilter.value = 'all';
    statusFilter.value = 'all';
    const available = PLATFORM_IDS.find((id) => next.platforms[id].summary);
    if (!next.platforms[selectedPlatform.value].summary && available) selectedPlatform.value = available;
  } catch (cause) {
    if (sequence === loadSequence) error.value = cause instanceof Error ? cause.message : String(cause);
  } finally {
    if (sequence === loadSequence) loading.value = false;
  }
}

async function initialize(): Promise<void> {
  loading.value = true;
  error.value = '';
  try {
    index.value = await loadIndex();
    const first = index.value.results[0];
    if (first) selectedPath.value = first.path;
    else loading.value = false;
  } catch (cause) {
    error.value = cause instanceof Error ? cause.message : String(cause);
    loading.value = false;
  }
}

watch(selectedPath, () => void refreshRun());
watch(locale, (value) => {
  if (typeof document === 'undefined') return;
  const language = value === 'en' ? 'en' : 'zh-CN';
  document.documentElement.lang = language;
  localStorage.setItem('benchmark-locale', language);
}, {immediate: true});
onMounted(() => {
  if (localStorage.getItem('benchmark-locale') === 'en') locale.value = 'en';
  void initialize();
});
</script>

<template>
  <div id="top" class="benchmark-dashboard site-shell">
      <section class="hero">
        <div>
          <p class="eyebrow">{{ t('app.eyebrow') }}</p>
          <h1>{{ t('app.title') }}</h1>
          <p class="hero-copy">{{ t('app.subtitle') }}</p>
        </div>
        <div class="hero-actions">
          <button class="language-button" type="button" @click="switchLanguage">
            {{ t('app.language') }}
          </button>
          <label v-if="index?.results.length" class="run-picker">
            <span>{{ t('run.label') }}</span>
            <select v-model="selectedPath">
              <option v-for="entry in index.results" :key="entry.path" :value="entry.path">
                v{{ entry.shotium_version }} · {{ formatDate(entry.generated_utc) }} · #{{ entry.run_id }}
              </option>
            </select>
          </label>
        </div>
      </section>

      <div v-if="loading" class="state-panel" role="status">
        <span class="spinner" aria-hidden="true"></span>
        {{ t('app.loading') }}
      </div>
      <div v-else-if="error" class="state-panel error-panel" role="alert">
        <strong>{{ t('app.loadError') }}</strong>
        <span>{{ error }}</span>
        <button type="button" @click="initialize">{{ t('app.retry') }}</button>
      </div>
      <div v-else-if="!run" class="state-panel">{{ t('app.empty') }}</div>

      <template v-else>
        <section class="run-meta" aria-label="Run metadata">
          <div><span>{{ t('run.version') }}</span><strong>v{{ run.manifest.shotium_version }}</strong></div>
          <div><span>{{ t('run.profile') }}</span><strong>{{ profileLabel(run.manifest.profile) }}</strong></div>
          <div><span>{{ t('run.generated') }}</span><strong>{{ formatDate(run.manifest.generated_utc) }}</strong></div>
          <div><span>{{ t('run.source') }}</span><strong class="mono">{{ run.manifest.source_sha?.slice(0, 12) || '—' }}</strong></div>
          <a v-if="run.manifest.run_url" :href="run.manifest.run_url" target="_blank" rel="noreferrer">
            {{ t('run.workflow') }} ↗
          </a>
        </section>

        <section class="section-card overview-card">
          <div class="section-heading">
            <div>
              <p class="section-kicker">01</p>
              <h2>{{ t('overview.title') }}</h2>
            </div>
          </div>
          <div class="status-grid">
            <div>
              <span>{{ t('overview.complete') }}</span>
              <strong class="status-pill" :class="statusClass(run.manifest.status)">{{ statusLabel(run.manifest.status) }}</strong>
            </div>
            <div>
              <span>{{ t('overview.quality') }}</span>
              <strong class="status-pill" :class="statusClass(run.manifest.quality_status)">{{ statusLabel(run.manifest.quality_status) }}</strong>
            </div>
            <div>
              <span>{{ t('overview.evidence') }}</span>
              <strong class="status-pill" :class="statusClass(run.manifest.evidence_status)">{{ statusLabel(run.manifest.evidence_status) }}</strong>
            </div>
          </div>
          <p class="conclusion">
            {{ t('overview.conclusion', platformCounts) }}
          </p>
          <p class="fair-note">{{ t('overview.fairNote') }}</p>
          <div class="platform-overview-grid">
            <button
              v-for="item in platformOverviews"
              :key="item.id"
              type="button"
              :class="{active: selectedPlatform === item.id}"
              @click="selectedPlatform = item.id"
            >
              <span class="platform-overview-title">
                <strong>{{ t(platformTranslationKeys[item.id]) }}</strong>
                <span class="inline-status" :class="statusClass(item.status)">{{ statusLabel(item.status) }}</span>
              </span>
              <span class="platform-winner">
                {{ t('overview.winner') }}：<strong>{{ item.winners.join('、') || t('overview.noRanking') }}</strong>
              </span>
              <small>{{ t('overview.rankingCoverage', {ranked: item.ranked, cells: item.cells}) }}</small>
            </button>
          </div>
        </section>

        <section class="platform-section">
          <div class="section-heading compact-heading">
            <div>
              <p class="section-kicker">02</p>
              <h2>{{ t('platform.title') }}</h2>
            </div>
          </div>
          <div class="platform-tabs" role="tablist" :aria-label="t('platform.title')">
            <button
              v-for="id in PLATFORM_IDS"
              :key="id"
              type="button"
              role="tab"
              :aria-selected="selectedPlatform === id"
              :class="{active: selectedPlatform === id}"
              @click="selectedPlatform = id"
            >
              <span>{{ t(platformTranslationKeys[id]) }}</span>
              <span class="status-dot" :class="statusClass(platformStatus(id))"></span>
              <small>{{ statusLabel(platformStatus(id)) }}</small>
            </button>
          </div>
        </section>

        <div v-if="!selectedSummary" class="state-panel inline-state">
          <strong>{{ t('platform.unavailable') }}</strong>
          <span v-if="selectedData?.loadError">{{ selectedData.loadError }}</span>
        </div>

        <template v-else>
          <section class="section-card">
            <div class="section-heading">
              <div>
                <p class="section-kicker">03</p>
                <h2>{{ t('ranking.title') }}</h2>
                <p>{{ t('ranking.help') }}</p>
              </div>
              <span class="status-pill" :class="statusClass(selectedSummary.status)">
                {{ statusLabel(selectedSummary.status) }}
              </span>
            </div>
            <div class="table-scroll">
              <table class="ranking-table">
                <thead>
                  <tr>
                    <th>{{ t('ranking.rank') }}</th>
                    <th>{{ t('ranking.engine') }}</th>
                    <th>{{ t('ranking.ratio') }}</th>
                    <th>{{ t('ranking.compared') }}</th>
                    <th>{{ t('ranking.champions') }}</th>
                    <th>{{ t('ranking.excluded') }}</th>
                  </tr>
                </thead>
                <tbody>
                  <tr v-for="row in ranking" :key="row.engine" :class="{'unranked-row': !row.official}">
                    <td class="rank-cell">{{ rankNumbers.get(row.engine) ?? '—' }}</td>
                    <td>
                      <strong>{{ row.engine }}</strong>
                      <span class="inline-status" :class="statusClass(row.status)">{{ statusLabel(row.status) }}</span>
                    </td>
                    <td class="metric-cell">
                      {{ formatRatio(row.geometricMean) }}
                      <small v-if="row.engine === 'shotium'">{{ t('ranking.baseline') }}</small>
                    </td>
                    <td>{{ row.compared }} / {{ row.totalBaselines }}</td>
                    <td>{{ row.champions }}</td>
                    <td class="reason-cell">
                      <span v-if="!row.exclusions.length">—</span>
                      <span v-for="reason in row.exclusions" :key="`${reason.code}-${reason.count}`">
                        {{ exclusionText(reason) }}
                      </span>
                    </td>
                  </tr>
                </tbody>
              </table>
            </div>
          </section>

          <section class="section-card">
            <div class="section-heading filters-heading">
              <div>
                <p class="section-kicker">04</p>
                <h2>{{ t('scenarios.title') }}</h2>
              </div>
              <div class="filters">
                <label>
                  <span>{{ t('scenarios.engine') }}</span>
                  <select v-model="engineFilter">
                    <option value="all">{{ t('scenarios.all') }}</option>
                    <option v-for="engine in engineOptions" :key="engine" :value="engine">{{ engine }}</option>
                  </select>
                </label>
                <label>
                  <span>{{ t('scenarios.scenario') }}</span>
                  <select v-model="scenarioFilter">
                    <option value="all">{{ t('scenarios.all') }}</option>
                    <option v-for="scenario in scenarioOptions" :key="scenario" :value="scenario">{{ scenarioLabel(scenario) }}</option>
                  </select>
                </label>
                <label>
                  <span>{{ t('scenarios.status') }}</span>
                  <select v-model="statusFilter">
                    <option value="all">{{ t('scenarios.all') }}</option>
                    <option value="pass">{{ statusLabel('pass') }}</option>
                    <option value="noisy">{{ statusLabel('noisy') }}</option>
                    <option value="fail">{{ statusLabel('fail') }}</option>
                    <option value="infra-error">{{ statusLabel('infra-error') }}</option>
                  </select>
                </label>
              </div>
            </div>
            <div class="table-scroll">
              <table class="scenario-table">
                <thead>
                  <tr>
                    <th>{{ t('scenarios.engine') }}</th>
                    <th>{{ t('scenarios.scenario') }}</th>
                    <th>{{ t('scenarios.shard') }}</th>
                    <th>{{ t('scenarios.concurrency') }}</th>
                    <th>{{ t('scenarios.status') }}</th>
                    <th>{{ t('scenarios.ranked') }}</th>
                    <th>{{ t('scenarios.p50') }}</th>
                    <th>{{ t('scenarios.p95') }}</th>
                    <th>{{ t('scenarios.worst') }}</th>
                    <th>{{ t('scenarios.throughput') }}</th>
                    <th>{{ t('scenarios.ratio') }}</th>
                  </tr>
                </thead>
                <tbody>
                  <tr v-for="row in filteredScenarios" :key="`${row.engine}-${row.scenario}-${row.concurrency}`">
                    <td><strong>{{ row.engine }}</strong></td>
                    <td>{{ scenarioLabel(row.scenario) }}</td>
                    <td>{{ shardLabel(row.shard) }}</td>
                    <td>{{ row.concurrency }}</td>
                    <td><span class="inline-status" :class="statusClass(row.status)">{{ statusLabel(row.status) }}</span></td>
                    <td>{{ row.ranking_eligible ? t('scenarios.yes') : t('scenarios.no') }}</td>
                    <td>{{ formatMilliseconds(scenarioP50(row)) }}</td>
                    <td>{{ formatMilliseconds(rowP95(row)) }}</td>
                    <td>{{ formatMilliseconds(rowWorst(row)) }}</td>
                    <td>{{ formatNumber(row.throughput_per_second, 3) }}</td>
                    <td>{{ formatRatio(ratioForScenario(row, selectedSummary)) }}</td>
                  </tr>
                  <tr v-if="!filteredScenarios.length">
                    <td colspan="11" class="empty-cell">{{ t('scenarios.noRows') }}</td>
                  </tr>
                </tbody>
              </table>
            </div>
          </section>

          <section class="section-card quality-card">
            <div class="section-heading">
              <div>
                <p class="section-kicker">05</p>
                <h2>{{ t('quality.title') }}</h2>
              </div>
            </div>
            <div class="quality-stats">
              <div><span>{{ t('quality.rawSamples') }}</span><strong>{{ selectedSummary.raw_samples }}</strong></div>
              <div><span>{{ t('quality.failures') }}</span><strong>{{ selectedSummary.failures }}</strong></div>
              <div><span>{{ t('quality.engines') }}</span><strong>{{ availableEngineCount }} / {{ selectedSummary.engines.length }}</strong></div>
              <div><span>{{ t('quality.unhealthyScenarios') }}</span><strong>{{ unhealthyScenarios.length }}</strong></div>
            </div>

            <div v-if="selectedData?.loadError" class="notice error-notice">
              {{ t('quality.loadError', {error: selectedData.loadError}) }}
            </div>
            <div v-if="selectedSummary.error" class="notice error-notice">
              {{ t('quality.summaryError', {error: selectedSummary.error}) }}
            </div>
            <div v-if="selectedSummary.shards_complete === false" class="notice error-notice">
              {{ t('platform.shardsIncomplete') }}: {{ selectedSummary.missing_shards?.join(', ') || '—' }}
            </div>

            <div class="quality-columns">
              <div>
                <h3>{{ t('quality.engineExclusions') }}</h3>
                <ul v-if="excludedEngines.length" class="issue-list">
                  <li v-for="engine in excludedEngines" :key="engine.engine">
                    <span class="inline-status" :class="statusClass(engine.status)">{{ statusLabel(engine.status) }}</span>
                    <strong>{{ engine.engine }}</strong>
                    <p>{{ reasonLabel(engine.reason) }}</p>
                  </li>
                </ul>
                <p v-else class="muted">{{ t('quality.noIssues') }}</p>

                <h3>{{ t('quality.unhealthyScenarios') }}</h3>
                <ul v-if="unhealthyScenarios.length" class="issue-list compact-list">
                  <li v-for="row in unhealthyScenarios" :key="`${row.engine}-${row.scenario}-${row.concurrency}`">
                    <span class="inline-status" :class="statusClass(row.status)">{{ statusLabel(row.status) }}</span>
                    <strong>{{ row.engine }} · {{ scenarioLabel(row.scenario) }} · {{ t('scenarios.concurrency') }} {{ row.concurrency }}</strong>
                  </li>
                </ul>
                <p v-else class="muted">{{ t('quality.noIssues') }}</p>
              </div>
              <div>
                <h3>{{ t('quality.failureEvidence') }}</h3>
                <ol v-if="selectedData?.failures.length" class="failure-list">
                  <li v-for="(failure, failureIndex) in selectedData.failures" :key="`${failure.at || ''}-${failureIndex}`">
                    <div>
                      <span v-if="failure.shard" class="code-chip">{{ shardLabel(failure.shard) }}</span>
                      <span v-if="failure.engine" class="code-chip">{{ failure.engine }}</span>
                      <span v-if="failure.scenario" class="code-chip">{{ scenarioLabel(failure.scenario) }}</span>
                    </div>
                    <pre>{{ failure.error }}</pre>
                  </li>
                </ol>
                <p v-else class="muted">{{ t('quality.noFailures') }}</p>
              </div>
            </div>
          </section>
        </template>
      </template>
  </div>
</template>
