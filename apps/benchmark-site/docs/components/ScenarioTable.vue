<script setup lang="ts">
import {computed, ref, watch} from 'vue';
import {useI18n} from '../lib/i18n';
import {formatInt, formatMs, formatRatio, formatThroughput} from '../lib/format';
import {ENGINE_IDS, SHARD_ORDER, compareScenarios, engineColorVar, orderEngines, scenarioShard} from '../lib/labels';
import {isEligibleCell, ratioForScenario, scenarioKey, scenarioP50} from '../lib/ranking';
import type {PlatformSummary, ScenarioSummary} from '../lib/types';
import StatusChip from './StatusChip.vue';
import Tip from './Tip.vue';

/*
 * Every scenario row of the platform in an Element Plus table: filter by
 * engine / shard / status / ranked-only, sort by any column (server-style
 * sorting over the whole filtered set, nulls last), 20 rows a page with
 * Element Plus pagination or everything at once.
 */
const props = defineProps<{summary: PlatformSummary}>();

const {t, locale, engineName, scenarioName, scenarioHelp, shardName, statusLabel} = useI18n();

interface Row {
  row: ScenarioSummary;
  key: string;
  engine: string;
  shard: string;
  p50: number | null;
  p95: number | null;
  worst: number | null;
  throughput: number | null;
  ratio: number | null;
  eligible: boolean;
}

const all = computed<Row[]>(() => props.summary.scenarios.map((row) => {
  const distribution = row.latency_ms ?? row.wall_time_ms;
  return {
    row,
    key: scenarioKey(row),
    engine: row.engine,
    shard: scenarioShard(row),
    p50: scenarioP50(row),
    p95: distribution?.p95 ?? null,
    worst: distribution?.max ?? null,
    throughput: row.throughput_per_second,
    ratio: ratioForScenario(row, props.summary),
    eligible: isEligibleCell(row),
  };
}));

const engineOptions = computed(() => orderEngines(all.value.map((row) => row.engine)));
const shardOptions = computed(() => {
  const present = new Set(all.value.map((row) => row.shard));
  return [
    ...SHARD_ORDER.filter((shard) => present.has(shard)),
    ...[...present].filter((shard) => !(SHARD_ORDER as readonly string[]).includes(shard)).sort(),
  ];
});
const STATUS_ORDER = ['pass', 'noisy', 'fail', 'infra-error'] as const;
const statusOptions = computed(() => {
  const present = new Set(all.value.map((row) => row.row.status));
  return STATUS_ORDER.filter((status) => present.has(status));
});

const engineFilter = ref('all');
const shardFilter = ref('all');
const statusFilter = ref('all');
const rankedOnly = ref(false);

const filtered = computed(() => all.value.filter((row) =>
  (engineFilter.value === 'all' || row.engine === engineFilter.value) &&
  (shardFilter.value === 'all' || row.shard === shardFilter.value) &&
  (statusFilter.value === 'all' || row.row.status === statusFilter.value) &&
  (!rankedOnly.value || row.eligible)));

type SortKey = 'engine' | 'scenario' | 'concurrency' | 'status' | 'ranked' | 'p50' | 'p95' | 'worst' | 'throughput' | 'ratio';
const SORT_KEYS: readonly string[] = ['engine', 'scenario', 'concurrency', 'status', 'ranked', 'p50', 'p95', 'worst', 'throughput', 'ratio'];
const sortKey = ref<SortKey>('scenario');
const sortDir = ref<1 | -1>(1);

function engineIndex(engine: string): number {
  const index = (ENGINE_IDS as readonly string[]).indexOf(engine);
  return index === -1 ? ENGINE_IDS.length : index;
}

function numeric(left: number | null, right: number | null, dir: number): number {
  if (left === null && right === null) return 0;
  if (left === null) return 1;
  if (right === null) return -1;
  return (left - right) * dir;
}

function compareRows(left: Row, right: Row): number {
  const dir = sortDir.value;
  const byScenario = compareScenarios(left.row, right.row);
  const byEngine = engineIndex(left.engine) - engineIndex(right.engine);
  switch (sortKey.value) {
    case 'engine':
      return (byEngine || byScenario) * dir;
    case 'scenario':
      return (byScenario || byEngine) * dir;
    case 'concurrency':
      return ((left.row.concurrency - right.row.concurrency) || byScenario || byEngine) * dir;
    case 'status':
      return ((STATUS_ORDER.indexOf(left.row.status) - STATUS_ORDER.indexOf(right.row.status)) || byScenario || byEngine) * dir;
    case 'ranked':
      return ((Number(right.eligible) - Number(left.eligible)) || byScenario || byEngine) * dir;
    case 'p50':
      return numeric(left.p50, right.p50, dir) || byScenario || byEngine;
    case 'p95':
      return numeric(left.p95, right.p95, dir) || byScenario || byEngine;
    case 'worst':
      return numeric(left.worst, right.worst, dir) || byScenario || byEngine;
    case 'throughput':
      return numeric(left.throughput, right.throughput, dir) || byScenario || byEngine;
    case 'ratio':
      return numeric(left.ratio, right.ratio, dir) || byScenario || byEngine;
    default:
      return 0;
  }
}

const sorted = computed(() => [...filtered.value].sort(compareRows));

const PAGE_SIZE = 20;
const page = ref(1);
const showAll = ref(false);
const pages = computed(() => Math.max(1, Math.ceil(sorted.value.length / PAGE_SIZE)));
const visible = computed(() =>
  showAll.value ? sorted.value : sorted.value.slice((page.value - 1) * PAGE_SIZE, page.value * PAGE_SIZE));

watch([engineFilter, shardFilter, statusFilter, rankedOnly, sortKey, sortDir, () => props.summary], () => {
  page.value = 1;
});

watch(pages, (count) => {
  if (page.value > count) page.value = count;
});

function onSort({prop, order}: {prop: string | null; order: 'ascending' | 'descending' | null}) {
  if (!order || !prop || !SORT_KEYS.includes(prop)) {
    sortKey.value = 'scenario';
    sortDir.value = 1;
    return;
  }
  sortKey.value = prop as SortKey;
  sortDir.value = order === 'descending' ? -1 : 1;
}
</script>

<template>
  <section class="section scenarios">
    <div class="section__head">
      <h3>{{ t('table.title') }}</h3>
      <span class="hint">{{ t('table.rows', {shown: formatInt(sorted.length, locale), total: formatInt(all.length, locale)}) }}</span>
    </div>

    <div class="scenarios__filters">
      <label class="scenarios__filter">
        <span class="label">{{ t('table.filterEngine') }}</span>
        <el-select v-model="engineFilter" size="small" popper-class="bench-pop" class="scenarios__select">
          <el-option value="all" :label="t('table.all')" />
          <el-option v-for="engine in engineOptions" :key="engine" :value="engine" :label="engineName(engine)" />
        </el-select>
      </label>
      <label class="scenarios__filter">
        <span class="label">{{ t('table.filterShard') }}</span>
        <el-select v-model="shardFilter" size="small" popper-class="bench-pop" class="scenarios__select scenarios__select--short">
          <el-option value="all" :label="t('table.all')" />
          <el-option v-for="shard in shardOptions" :key="shard" :value="shard" :label="shardName(shard)" />
        </el-select>
      </label>
      <label class="scenarios__filter">
        <span class="label">{{ t('table.filterStatus') }}</span>
        <el-select v-model="statusFilter" size="small" popper-class="bench-pop" class="scenarios__select scenarios__select--short">
          <el-option value="all" :label="t('table.all')" />
          <el-option v-for="status in statusOptions" :key="status" :value="status" :label="statusLabel(status)" />
        </el-select>
      </label>
      <el-switch v-model="rankedOnly" size="small" :active-text="t('table.rankedOnly')" class="scenarios__switch" />
    </div>

    <el-table
      :data="visible"
      size="small"
      table-layout="auto"
      :default-sort="{prop: 'scenario', order: 'ascending'}"
      :empty-text="t('table.empty')"
      class="scenarios__table"
      @sort-change="onSort"
    >
      <el-table-column prop="engine" :label="t('table.engine')" sortable="custom" min-width="140">
        <template #default="{row}">
          <span class="engine" :class="{'engine--shotium': row.engine === 'shotium'}" :style="{'--dot-color': engineColorVar(row.engine)}">
            <span class="dot" aria-hidden="true" />{{ engineName(row.engine) }}
          </span>
        </template>
      </el-table-column>
      <el-table-column prop="scenario" :label="t('table.scenario')" sortable="custom" min-width="150">
        <template #default="{row}">
          <Tip v-if="scenarioHelp(row.row.scenario)" :text="scenarioHelp(row.row.scenario) ?? ''">
            <span class="tip-trigger">{{ scenarioName(row.row.scenario) }}</span>
          </Tip>
          <span v-else>{{ scenarioName(row.row.scenario) }}</span>
          <span class="hint scenarios__shard"> · {{ shardName(row.shard) }}</span>
        </template>
      </el-table-column>
      <el-table-column prop="concurrency" :label="t('table.concurrency')" sortable="custom" align="right" width="72">
        <template #default="{row}"><span class="num">{{ row.row.concurrency }}</span></template>
      </el-table-column>
      <el-table-column prop="status" :label="t('table.status')" sortable="custom" width="104">
        <template #default="{row}"><StatusChip :status="row.row.status" /></template>
      </el-table-column>
      <el-table-column prop="ranked" sortable="custom" width="100">
        <template #header>{{ t('table.ranked') }}<Tip :text="t('table.rankedHelp')" /></template>
        <template #default="{row}">
          <span v-if="row.eligible" class="scenarios__yes">{{ t('common.yes') }}</span>
          <span v-else class="hint">{{ t('common.no') }}</span>
        </template>
      </el-table-column>
      <el-table-column prop="p50" sortable="custom" align="right" width="96">
        <template #header>{{ t('table.p50') }}<Tip :text="t('table.p50Help')" /></template>
        <template #default="{row}"><strong class="num">{{ formatMs(row.p50, locale) }}</strong></template>
      </el-table-column>
      <el-table-column prop="p95" :label="t('table.p95')" sortable="custom" align="right" width="88">
        <template #default="{row}"><span class="num">{{ formatMs(row.p95, locale) }}</span></template>
      </el-table-column>
      <el-table-column prop="worst" :label="t('table.worst')" sortable="custom" align="right" width="88">
        <template #default="{row}"><span class="num">{{ formatMs(row.worst, locale) }}</span></template>
      </el-table-column>
      <el-table-column prop="throughput" :label="t('table.throughput')" sortable="custom" align="right" width="92">
        <template #default="{row}"><span class="num">{{ formatThroughput(row.throughput, locale) }}</span></template>
      </el-table-column>
      <el-table-column prop="ratio" :label="t('table.ratio')" sortable="custom" align="right" width="112">
        <template #default="{row}">
          <strong v-if="row.ratio !== null" class="num">{{ formatRatio(row.ratio, locale) }}</strong>
          <span v-else class="hint">—</span>
        </template>
      </el-table-column>
    </el-table>

    <div class="scenarios__pager">
      <el-pagination
        v-if="!showAll"
        v-model:current-page="page"
        :page-size="PAGE_SIZE"
        :total="sorted.length"
        layout="total, prev, pager, next"
        background
        size="small"
      />
      <el-button v-if="sorted.length > PAGE_SIZE" size="small" link type="primary" @click="showAll = !showAll">
        {{ showAll ? t('table.paginate') : t('table.showAll') }}
      </el-button>
    </div>
  </section>
</template>

<style scoped>
.scenarios__filters {
  display: flex;
  flex-wrap: wrap;
  align-items: flex-end;
  gap: 10px 18px;
  margin-bottom: 14px;
}

.scenarios__filter {
  display: inline-flex;
  flex-direction: column;
  gap: 4px;
}

.scenarios__select {
  width: 190px;
}

.scenarios__select--short {
  width: 130px;
}

.scenarios__switch {
  padding-bottom: 4px;
}

.scenarios__shard {
  font-size: var(--fs-12);
}

.scenarios__yes {
  color: var(--status-pass);
  font-weight: 600;
}

.scenarios__pager {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 10px 16px;
  margin-top: 12px;
}
</style>
