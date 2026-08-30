<script setup lang="ts">
import {computed, ref, type Component} from 'vue';
import {
  Grid,
  Promotion,
  Lightning,
  Connection,
  House,
  FirstAidKit,
  DataAnalysis,
  Odometer,
} from '@element-plus/icons-vue';
import {useI18n} from '../lib/i18n';
import {formatMs, formatRatio, formatThroughput} from '../lib/format';
import {
  SHARD_ORDER,
  compareScenarios,
  engineColorVar,
  orderEngines,
  scenarioShard,
} from '../lib/labels';
import {isEligibleCell, ratioForScenario, scenarioKey, scenarioP50} from '../lib/ranking';
import type {PlatformSummary, ScenarioSummary} from '../lib/types';
import StatusChip from './StatusChip.vue';

const props = defineProps<{
  summary: PlatformSummary;
  mode: 'ms' | 'ratio' | 'throughput';
  label?: string;
}>();

const {t, locale, engineName, scenarioName, scenarioHelp, shardName} = useI18n();

const activeShard = ref<string>('all');

interface EngineItem {
  engine: string;
  row: ScenarioSummary;
  p50: number | null;
  ratio: number | null;
  throughput: number | null;
  eligible: boolean;
  status: ScenarioSummary['status'];
}

interface ScenarioRowData {
  key: string;
  scenario: string;
  concurrency: number;
  shard: string;
  label: string;
  help: string | null;
  items: EngineItem[];
  shotiumItem: EngineItem | null;
  fastestItem: EngineItem | null;
  maxP50: number;
  maxRatio: number;
  maxThroughput: number;
  shotsText: string;
}

interface ShardSection {
  shard: string;
  name: string;
  rows: ScenarioRowData[];
}

function compareEngineItems(a: EngineItem, b: EngineItem, mode: 'ms' | 'ratio' | 'throughput'): number {
  const aValid = a.status === 'pass' && a.eligible;
  const bValid = b.status === 'pass' && b.eligible;
  if (aValid && !bValid) return -1;
  if (!aValid && bValid) return 1;
  if (!aValid && !bValid) {
    return (a.status === 'noisy' ? 0 : 1) - (b.status === 'noisy' ? 0 : 1);
  }

  if (mode === 'throughput') {
    const aVal = a.throughput ?? -Infinity;
    const bVal = b.throughput ?? -Infinity;
    if (aVal !== bVal) return bVal - aVal; // descending: higher is better
  } else if (mode === 'ratio') {
    const aVal = a.ratio ?? Infinity;
    const bVal = b.ratio ?? Infinity;
    if (aVal !== bVal) return aVal - bVal; // ascending: smaller ratio is faster
  } else {
    // mode === 'ms'
    const aVal = a.p50 ?? Infinity;
    const bVal = b.p50 ?? Infinity;
    if (aVal !== bVal) return aVal - bVal; // ascending: lower ms is faster
  }

  if (a.engine === 'shotium') return -1;
  if (b.engine === 'shotium') return 1;
  return a.engine.localeCompare(b.engine);
}

const allRows = computed<ScenarioRowData[]>(() => {
  const byKey = new Map<string, ScenarioSummary[]>();
  for (const row of props.summary.scenarios) {
    const key = scenarioKey(row);
    const list = byKey.get(key) ?? [];
    list.push(row);
    byKey.set(key, list);
  }

  const result: ScenarioRowData[] = [];

  for (const [key, rows] of byKey.entries()) {
    const first = rows[0];
    if (!first) continue;

    const shard = scenarioShard(first);
    const orderedEngines = orderEngines(rows.map((r) => r.engine));
    const rowByEngine = new Map(rows.map((r) => [r.engine, r]));

    const items: EngineItem[] = [];
    let shotiumItem: EngineItem | null = null;
    let maxP50 = 0;
    let maxRatio = 0;
    let maxThroughput = 0;

    for (const engine of orderedEngines) {
      const row = rowByEngine.get(engine);
      if (!row) continue;

      const p50 = scenarioP50(row);
      const ratio = ratioForScenario(row, props.summary);
      const throughput = row.throughput_per_second;
      const eligible = isEligibleCell(row);

      const item: EngineItem = {
        engine,
        row,
        p50,
        ratio,
        throughput,
        eligible,
        status: row.status,
      };

      items.push(item);
      if (engine === 'shotium') shotiumItem = item;

      if (p50 !== null && p50 > maxP50) maxP50 = p50;
      if (ratio !== null && ratio > maxRatio) maxRatio = ratio;
      if (throughput !== null && throughput > maxThroughput) maxThroughput = throughput;
    }

    // Sort items by performance in current mode: fastest first
    items.sort((a, b) => compareEngineItems(a, b, props.mode));

    const eligibleWithP50 = items.filter((item) => item.eligible && item.p50 !== null && item.p50 > 0);
    const fastestItem = eligibleWithP50.length
      ? [...eligibleWithP50].sort((a, b) => (a.p50 ?? Infinity) - (b.p50 ?? Infinity))[0] ?? null
      : null;

    const shotsCount = first.shots || first.runs || 0;
    const shotsText = shotsCount > 0 ? t('scenario.shots', {n: shotsCount}) : '';

    result.push({
      key,
      scenario: first.scenario,
      concurrency: first.concurrency,
      shard,
      label: scenarioName(first.scenario, first.concurrency),
      help: scenarioHelp(first.scenario),
      items,
      shotiumItem,
      fastestItem,
      maxP50: maxP50 > 0 ? maxP50 : 1,
      maxRatio: maxRatio > 0 ? maxRatio : 1,
      maxThroughput: maxThroughput > 0 ? maxThroughput : 1,
      shotsText,
    });
  }

  return result.sort((a, b) => {
    const aFirst = a.items[0]?.row;
    const bFirst = b.items[0]?.row;
    if (!aFirst || !bFirst) return 0;
    return compareScenarios(aFirst, bFirst);
  });
});

const shardSections = computed<ShardSection[]>(() => {
  const map = new Map<string, ScenarioRowData[]>();
  for (const row of allRows.value) {
    if (activeShard.value !== 'all' && row.shard !== activeShard.value) continue;
    const list = map.get(row.shard) ?? [];
    list.push(row);
    map.set(row.shard, list);
  }

  const sections: ShardSection[] = [];
  for (const shard of SHARD_ORDER) {
    const rows = map.get(shard);
    if (rows && rows.length) {
      sections.push({
        shard,
        name: shardName(shard),
        rows,
      });
    }
  }

  // Any other shards
  for (const [shard, rows] of map.entries()) {
    if (!(SHARD_ORDER as readonly string[]).includes(shard) && rows.length) {
      sections.push({
        shard,
        name: shardName(shard),
        rows,
      });
    }
  }

  return sections;
});

const availableShards = computed(() => {
  const shards = new Set(allRows.value.map((r) => r.shard));
  const ordered = SHARD_ORDER.filter((s) => shards.has(s));
  const rest = [...shards].filter((s) => !(SHARD_ORDER as readonly string[]).includes(s)).sort();
  return [...ordered, ...rest];
});

const SHARD_ICONS: Record<string, Component> = {
  all: Grid,
  startup: Promotion,
  throughput: Lightning,
  parallel: Connection,
  resident: House,
  resilience: FirstAidKit,
};

function shardIconComponent(shard: string): Component {
  return SHARD_ICONS[shard] ?? DataAnalysis;
}

function barWidth(item: EngineItem, rowData: ScenarioRowData): string {
  if (item.status !== 'pass' || !item.eligible) {
    if (item.status === 'noisy' && item.p50 !== null) {
      const pct = Math.min(100, Math.max(8, (item.p50 / rowData.maxP50) * 100));
      return `${pct.toFixed(1)}%`;
    }
    return '0%';
  }

  if (props.mode === 'ratio') {
    if (item.ratio === null || item.ratio <= 0) return '0%';
    const pct = Math.min(100, Math.max(6, (item.ratio / rowData.maxRatio) * 100));
    return `${pct.toFixed(1)}%`;
  }

  if (props.mode === 'throughput') {
    if (item.throughput === null || item.throughput <= 0) return '0%';
    const pct = Math.min(100, Math.max(6, (item.throughput / rowData.maxThroughput) * 100));
    return `${pct.toFixed(1)}%`;
  }

  // mode === 'ms'
  if (item.p50 === null || item.p50 <= 0) return '0%';
  const pct = Math.min(100, Math.max(6, (item.p50 / rowData.maxP50) * 100));
  return `${pct.toFixed(1)}%`;
}
</script>

<template>
  <div class="overview-board" role="region" :aria-label="label ?? t('overview.title')">
    <!-- Shard Filter Nav -->
    <div class="overview-board__nav">
      <button
        type="button"
        class="nav-tab"
        :class="{'nav-tab--active': activeShard === 'all'}"
        @click="activeShard = 'all'"
      >
        <el-icon class="nav-tab__icon"><Grid /></el-icon>
        <span>{{ t('overview.filterAll') }}</span>
      </button>
      <button
        v-for="shard in availableShards"
        :key="shard"
        type="button"
        class="nav-tab"
        :class="{'nav-tab--active': activeShard === shard}"
        @click="activeShard = shard"
      >
        <el-icon class="nav-tab__icon"><component :is="shardIconComponent(shard)" /></el-icon>
        <span>{{ shardName(shard) }}</span>
      </button>
    </div>

    <!-- Shard Sections -->
    <div v-if="shardSections.length" class="overview-board__sections">
      <section v-for="section in shardSections" :key="section.shard" class="shard-block">
        <header class="shard-block__header">
          <div class="shard-block__title-wrap">
            <el-icon class="shard-block__icon"><component :is="shardIconComponent(section.shard)" /></el-icon>
            <h3 class="shard-block__title">{{ section.name }}</h3>
          </div>
          <div class="shard-block__line" aria-hidden="true" />
        </header>

        <div class="shard-block__rows">
          <article
            v-for="row in section.rows"
            :key="row.key"
            class="scenario-row"
          >
            <!-- Left Info Area -->
            <div class="scenario-row__meta">
              <div class="scenario-meta__header">
                <h4 class="scenario-meta__title">{{ row.label }}</h4>
                <span v-if="row.concurrency > 1" class="scenario-meta__conc">c={{ row.concurrency }}</span>
              </div>
              <p v-if="row.help" class="scenario-meta__desc">{{ row.help }}</p>
              <div class="scenario-meta__footer">
                <span v-if="row.shotsText" class="scenario-meta__shots">
                  <el-icon class="scenario-meta__icon"><Odometer /></el-icon>
                  <span>{{ row.shotsText }}</span>
                </span>
              </div>
            </div>

            <!-- Right Bars Comparison Area -->
            <div class="scenario-row__chart">
              <div
                v-for="item in row.items"
                :key="item.engine"
                class="engine-bar-row"
                :class="{
                  'engine-bar-row--hero': item.engine === 'shotium',
                  'engine-bar-row--dimmed': item.status !== 'pass' || !item.eligible,
                }"
              >
                <!-- Column 1: Engine Label -->
                <div class="engine-bar-row__label">
                  <span class="dot" :style="{backgroundColor: engineColorVar(item.engine)}" aria-hidden="true" />
                  <span class="engine-bar-row__name">{{ engineName(item.engine) }}</span>
                </div>

                <!-- Column 2: Bar Track & Fill -->
                <div class="engine-bar-row__track">
                  <div
                    class="engine-bar-row__bar"
                    :class="{
                      'engine-bar-row__bar--hero': item.engine === 'shotium',
                      'engine-bar-row__bar--noisy': item.status === 'noisy',
                      'engine-bar-row__bar--failed': item.status === 'fail' || item.status === 'infra-error',
                    }"
                    :style="{
                      width: barWidth(item, row),
                      backgroundColor: item.status === 'pass' ? engineColorVar(item.engine) : undefined,
                    }"
                  />
                </div>

                <!-- Column 3: Numeric Value (Right-Aligned) -->
                <div class="engine-bar-row__val">
                  <template v-if="item.status === 'pass' && item.eligible">
                    <strong v-if="mode === 'ms'" class="metric-num">{{ formatMs(item.p50, locale) }}</strong>
                    <strong v-else-if="mode === 'ratio'" class="metric-num">{{ item.engine === 'shotium' ? '1.00×' : formatRatio(item.ratio, locale) }}</strong>
                    <strong v-else-if="mode === 'throughput'" class="metric-num">{{ formatThroughput(item.throughput, locale) }}</strong>
                  </template>
                  <template v-else-if="item.status === 'noisy' && item.p50 !== null">
                    <span class="metric-sub">{{ formatMs(item.p50, locale) }}</span>
                  </template>
                  <template v-else>
                    <span class="metric-dash">—</span>
                  </template>
                </div>

                <!-- Column 4: Capsule Pill (Right-Aligned) -->
                <div class="engine-bar-row__badge">
                  <template v-if="item.status === 'pass' && item.eligible">
                    <template v-if="mode === 'ms'">
                      <span
                        v-if="item.ratio !== null"
                        class="metric-pill"
                        :class="{'metric-pill--hero': item.engine === 'shotium'}"
                      >
                        {{ item.engine === 'shotium' ? t('scenario.baselineRatio') : `${formatRatio(item.ratio, locale)}` }}
                      </span>
                    </template>
                    <template v-else-if="mode === 'ratio'">
                      <span v-if="item.p50 !== null" class="metric-sub-pill" :class="{'metric-sub-pill--hero': item.engine === 'shotium'}">
                        {{ item.engine === 'shotium' ? t('scenario.baseline') : formatMs(item.p50, locale) }}
                      </span>
                    </template>
                    <template v-else-if="mode === 'throughput'">
                      <span v-if="item.p50 !== null" class="metric-sub-pill">
                        {{ formatMs(item.p50, locale) }}
                      </span>
                    </template>
                  </template>
                  <template v-else>
                    <StatusChip :status="item.status" />
                  </template>
                </div>
              </div>
            </div>
          </article>
        </div>
      </section>
    </div>

    <!-- Empty State -->
    <div v-else class="overview-board__empty">
      <el-empty :description="t('overview.noData')" :image-size="64" />
    </div>
  </div>
</template>

<style scoped>
.overview-board {
  display: flex;
  flex-direction: column;
  gap: 20px;
  width: 100%;
  padding: 4px 0;
}

/* Shard Filter Nav (Frosted Glass Pills) */
.overview-board__nav {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px;
  margin-bottom: 4px;
}

.nav-tab {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 6px 14px;
  font-size: var(--fs-13, 13px);
  font-family: inherit;
  font-weight: 550;
  color: var(--ink-2);
  background: var(--glass-fill-1, rgba(255, 255, 255, 0.55));
  border: 1px solid var(--glass-edge, rgba(255, 255, 255, 0.7));
  border-radius: var(--radius-chip, 999px);
  backdrop-filter: blur(8px);
  cursor: pointer;
  transition: all 0.16s ease;
  user-select: none;
  white-space: nowrap;
}

.nav-tab:hover {
  color: var(--ink-1);
  background: var(--glass-fill-2, rgba(255, 255, 255, 0.85));
  border-color: var(--line-1, rgba(0, 0, 0, 0.14));
}

.nav-tab--active {
  color: var(--brand, #146c43) !important;
  background: var(--brand-soft, rgba(20, 108, 67, 0.14)) !important;
  border-color: color-mix(in srgb, var(--brand) 35%, transparent) !important;
  font-weight: 650;
}

.nav-tab__icon {
  font-size: 14px;
  display: flex;
  align-items: center;
  justify-content: center;
}

/* Sections */
.overview-board__sections {
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.shard-block {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.shard-block__header {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 4px 2px;
}

.shard-block__title-wrap {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  white-space: nowrap;
}

.shard-block__icon {
  font-size: 16px;
  color: var(--brand, #146c43);
  display: flex;
  align-items: center;
  justify-content: center;
}

.shard-block__title {
  margin: 0;
  font-size: var(--fs-15, 15px);
  font-weight: 700;
  color: var(--ink-1);
  letter-spacing: -0.01em;
}

.shard-block__line {
  flex: 1;
  height: 1px;
  background: var(--line-2, rgba(0, 0, 0, 0.08));
}

.shard-block__rows {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

/* Scenario Row */
.scenario-row {
  display: grid;
  grid-template-columns: 210px minmax(0, 1fr);
  gap: 20px;
  align-items: center;
  padding: 16px 20px;
  background: var(--glass-fill-1, rgba(255, 255, 255, 0.55));
  border: 1px solid var(--glass-edge, rgba(255, 255, 255, 0.7));
  border-radius: var(--radius-inner, 12px);
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.02);
  transition: background-color 0.15s ease, border-color 0.15s ease, box-shadow 0.15s ease;
}

.scenario-row:hover {
  background: var(--glass-fill-2, rgba(255, 255, 255, 0.9));
  border-color: var(--line-1, rgba(0, 0, 0, 0.14));
  box-shadow: 0 4px 12px -4px rgba(0, 0, 0, 0.06);
}

/* Left Info Column */
.scenario-row__meta {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding-right: 16px;
  border-right: 1px solid var(--line-2, rgba(0, 0, 0, 0.06));
}

.scenario-meta__header {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}

.scenario-meta__title {
  margin: 0;
  font-size: var(--fs-14, 14px);
  font-weight: 650;
  color: var(--ink-1);
  line-height: 1.35;
  letter-spacing: -0.01em;
}

.scenario-meta__conc {
  font-size: 11px;
  font-weight: 700;
  color: var(--ink-2);
  background: var(--control-fill, rgba(0, 0, 0, 0.06));
  padding: 1px 6px;
  border-radius: 4px;
  font-variant-numeric: tabular-nums;
}

.scenario-meta__desc {
  margin: 0;
  font-size: var(--fs-12, 12px);
  color: var(--ink-3);
  line-height: 1.45;
}

.scenario-meta__footer {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-top: 4px;
}

.scenario-meta__shots {
  display: inline-flex;
  align-items: center;
  gap: 5px;
  font-size: 11px;
  font-weight: 550;
  color: var(--ink-3);
  background: var(--line-2, rgba(0, 0, 0, 0.05));
  padding: 2px 7px;
  border-radius: 4px;
  font-variant-numeric: tabular-nums;
}

.scenario-meta__icon {
  font-size: 12px;
}

/* Right Chart Column */
.scenario-row__chart {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.engine-bar-row {
  display: grid;
  grid-template-columns: 125px minmax(0, 1fr) 72px 90px;
  align-items: center;
  gap: 12px;
  padding: 2px 4px;
  border-radius: 6px;
  transition: background-color 0.12s ease;
}

.engine-bar-row:hover {
  background: var(--row-hover, rgba(20, 108, 67, 0.06));
}

.engine-bar-row--dimmed {
  opacity: 0.6;
}

.engine-bar-row__label {
  display: inline-flex;
  align-items: center;
  gap: 7px;
  font-size: var(--fs-12, 12px);
  font-weight: 550;
  color: var(--ink-2);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.engine-bar-row--hero .engine-bar-row__label {
  font-weight: 700;
  color: var(--ink-1);
}

.dot {
  width: 7px;
  height: 7px;
  border-radius: 50%;
  flex-shrink: 0;
}

.engine-bar-row__track {
  width: 100%;
  height: 8px;
  background: var(--track, rgba(0, 0, 0, 0.06));
  border-radius: 999px;
  overflow: hidden;
  position: relative;
}

@keyframes bar-grow {
  0% {
    transform: scaleX(0);
    opacity: 0.4;
  }
  100% {
    transform: scaleX(1);
    opacity: 1;
  }
}

.engine-bar-row__bar {
  height: 100%;
  border-radius: 999px;
  transform-origin: left center;
  animation: bar-grow 0.5s cubic-bezier(0.16, 1, 0.3, 1) both;
  transition: width 0.35s cubic-bezier(0.16, 1, 0.3, 1);
}

.engine-bar-row__bar--hero {
  box-shadow: 0 0 6px rgba(20, 108, 67, 0.4);
}

.engine-bar-row__bar--noisy {
  background: repeating-linear-gradient(
    45deg,
    var(--status-noisy, #d97706),
    var(--status-noisy, #d97706) 4px,
    rgba(217, 119, 6, 0.35) 4px,
    rgba(217, 119, 6, 0.35) 8px
  ) !important;
}

.engine-bar-row__bar--failed {
  background: var(--status-fail, #dc2626) !important;
}

/* Numeric column (Col 3, right aligned) */
.engine-bar-row__val {
  text-align: right;
  justify-self: end;
  font-size: var(--fs-12, 12px);
  font-variant-numeric: tabular-nums;
  white-space: nowrap;
}

.metric-num {
  font-weight: 650;
  color: var(--ink-1);
}

.metric-sub {
  font-size: 11px;
  color: var(--ink-3);
}

.metric-dash {
  color: var(--ink-3);
  font-size: 12px;
}

/* Capsule pill column (Col 4, right aligned) */
.engine-bar-row__badge {
  display: flex;
  justify-content: flex-end;
  justify-self: end;
  font-size: var(--fs-12, 12px);
  font-variant-numeric: tabular-nums;
  white-space: nowrap;
}

.metric-pill {
  font-size: 11px;
  font-weight: 600;
  color: var(--ink-2);
  padding: 1px 7px;
  border-radius: var(--radius-chip, 999px);
  background: var(--control-fill, rgba(0, 0, 0, 0.06));
  border: 1px solid var(--line-2, rgba(0, 0, 0, 0.06));
  display: inline-flex;
  align-items: center;
  justify-content: center;
}

.metric-pill--hero {
  color: var(--brand, #146c43);
  background: var(--brand-soft, rgba(20, 108, 67, 0.12));
  border-color: color-mix(in srgb, var(--brand) 25%, transparent);
  font-weight: 700;
}

.metric-sub-pill {
  font-size: 11px;
  font-weight: 550;
  color: var(--ink-3);
  padding: 1px 6px;
  border-radius: var(--radius-chip, 999px);
  background: var(--line-2, rgba(0, 0, 0, 0.04));
  border: 1px solid var(--line-2, rgba(0, 0, 0, 0.06));
}

.metric-sub-pill--hero {
  color: var(--brand, #146c43);
  background: var(--brand-soft, rgba(20, 108, 67, 0.12));
  font-weight: 650;
}

/* Responsive adjustments */
@media (max-width: 900px) {
  .scenario-row {
    grid-template-columns: 1fr;
    gap: 12px;
  }
  .scenario-row__meta {
    padding-right: 0;
    border-right: none;
    border-bottom: 1px solid var(--line-2, rgba(0, 0, 0, 0.06));
    padding-bottom: 10px;
  }
}

@media (max-width: 600px) {
  .engine-bar-row {
    grid-template-columns: 100px minmax(0, 1fr) 56px 72px;
    gap: 8px;
  }
}
</style>
