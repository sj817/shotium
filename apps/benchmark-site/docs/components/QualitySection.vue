<script setup lang="ts">
import {computed} from 'vue';
import {withBase} from 'vitepress';
import {useI18n} from '../lib/i18n';
import {firstLine} from '../lib/format';
import {compareScenarios, engineColorVar, orderEngines} from '../lib/labels';
import type {PlatformReport} from '../lib/report';
import StatusChip from './StatusChip.vue';

/*
 * The trust section: counts as Element Plus statistics, warnings as alerts,
 * every scenario that did not pass, every engine that is not healthy with its
 * reason and shard statuses, the failure records in a collapse (full error
 * text inside), and links to the raw archive files.
 */
const props = defineProps<{platform: PlatformReport; runPath: string}>();

const {t, engineName, scenarioName, shardName} = useI18n();

const summary = computed(() => props.platform.data.summary);
const failures = computed(() => props.platform.data.failures);
const failureCount = computed(() => summary.value?.failures ?? failures.value.length);

const engineCount = computed(() => {
  const engines = summary.value?.engines ?? [];
  return {available: engines.filter((engine) => engine.status !== 'n/a').length, total: engines.length};
});

const anomalies = computed(() => {
  const rows = (summary.value?.scenarios ?? []).filter((row) => row.status !== 'pass');
  const order = orderEngines(rows.map((row) => row.engine));
  return [...rows].sort((left, right) =>
    compareScenarios(left, right) || order.indexOf(left.engine) - order.indexOf(right.engine));
});

const engineIssues = computed(() => {
  const engines = summary.value?.engines ?? [];
  const order = orderEngines(engines.map((engine) => engine.engine));
  return engines
    .filter((engine) => engine.status !== 'pass')
    .sort((left, right) => order.indexOf(left.engine) - order.indexOf(right.engine))
    .map((engine) => ({
      ...engine,
      reasons: uniqueReasons(engine.reason),
      shards: Object.entries(engine.shard_statuses ?? {}),
    }));
});

function uniqueReasons(reason: string | null): string[] {
  if (!reason) return [];
  const seen = new Set<string>();
  for (const part of reason.split(/;\s*/)) {
    const text = part.replace(/^[a-z-]+:\s*/i, '').trim();
    if (text) seen.add(text);
  }
  return [...seen];
}

const warnings = computed(() => {
  const out: Array<{tone: 'warning' | 'error'; text: string}> = [];
  const value = summary.value;
  if (!value) {
    out.push({tone: 'error', text: t('quality.notArchived')});
    if (props.platform.data.loadError) {
      out.push({tone: 'error', text: t('quality.loadError', {error: props.platform.data.loadError})});
    }
    return out;
  }
  if (value.shards_complete === false) {
    out.push({tone: 'warning', text: t('quality.missingShards', {shards: (value.missing_shards ?? []).join(', ') || '?'})});
  }
  if (value.error) out.push({tone: 'warning', text: t('quality.summaryError', {error: value.error})});
  if (props.platform.manifest?.evidence_complete === false) out.push({tone: 'warning', text: t('quality.evidenceIncomplete')});
  if (props.platform.manifest?.artifact_error) {
    out.push({tone: 'error', text: t('quality.artifactError', {error: props.platform.manifest.artifact_error})});
  }
  return out;
});

const PLATFORM_FILES = ['summary.json', 'failures.json', 'quality.json', 'samples.jsonl'];
const RUN_FILES = ['manifest.json', 'report.md', 'report.zh-CN.md', 'summary.csv'];

function platformFile(name: string): string {
  return withBase(`/benchmark-results/${props.runPath}/${props.platform.id}/${name}`);
}

function runFile(name: string): string {
  return withBase(`/benchmark-results/${props.runPath}/${name}`);
}

function failureTitle(failure: {engine?: string; scenario?: string; shard?: string; repeat?: number; attempt?: number}): string {
  const parts: string[] = [];
  if (failure.engine) parts.push(engineName(failure.engine));
  if (failure.scenario) parts.push(scenarioName(failure.scenario));
  if (failure.shard) parts.push(shardName(failure.shard));
  if (failure.repeat !== undefined || failure.attempt !== undefined) {
    parts.push(t('quality.repeat', {repeat: failure.repeat ?? '—', attempt: failure.attempt ?? '—'}));
  }
  return parts.join(' · ');
}
</script>

<template>
  <section class="section quality">
    <div class="section__head">
      <h3>{{ t('quality.title') }}</h3>
    </div>

    <el-row v-if="summary" :gutter="16" class="quality__stats">
      <el-col :xs="12" :sm="6"><el-statistic :title="t('quality.rawSamples')" :value="summary.raw_samples" /></el-col>
      <el-col :xs="12" :sm="6">
        <el-statistic :title="t('quality.failures')" :value="failureCount" :value-style="failureCount > 0 ? {color: 'var(--status-fail)'} : undefined" />
      </el-col>
      <el-col :xs="12" :sm="6"><el-statistic :title="t('quality.engines')" :value="engineCount.available" :suffix="`/ ${engineCount.total}`" /></el-col>
      <el-col :xs="12" :sm="6">
        <el-statistic :title="t('quality.anomalies')" :value="anomalies.length" :value-style="anomalies.length > 0 ? {color: 'var(--status-noisy)'} : undefined" />
      </el-col>
    </el-row>

    <div v-if="warnings.length" class="quality__warnings">
      <el-alert v-for="(warning, index) in warnings" :key="index" :title="warning.text" :type="warning.tone" show-icon :closable="false" />
    </div>

    <template v-if="summary">
      <el-row :gutter="32" class="quality__grid">
        <el-col :xs="24" :md="12" class="quality__block">
          <h4>{{ t('quality.anomalyList') }}</h4>
          <p v-if="!anomalies.length" class="hint">{{ t('quality.noAnomalies') }}</p>
          <ul v-else class="quality__list">
            <li v-for="row in anomalies" :key="`${row.engine}|${row.scenario}|${row.concurrency}`" class="quality__anomaly">
              <span class="engine" :style="{'--dot-color': engineColorVar(row.engine)}">
                <span class="dot" aria-hidden="true" />{{ engineName(row.engine) }}
              </span>
              <span class="quality__scenario">{{ scenarioName(row.scenario, row.concurrency) }}</span>
              <StatusChip :status="row.status" />
            </li>
          </ul>
        </el-col>

        <el-col :xs="24" :md="12" class="quality__block">
          <h4>{{ t('quality.engineExclusions') }}</h4>
          <p v-if="!engineIssues.length" class="hint">{{ t('quality.allEnginesPass') }}</p>
          <ul v-else class="quality__list">
            <li v-for="engine in engineIssues" :key="engine.engine" class="quality__engine">
              <div class="quality__engine-head">
                <span class="engine" :style="{'--dot-color': engineColorVar(engine.engine)}">
                  <span class="dot" aria-hidden="true" />{{ engineName(engine.engine) }}
                </span>
                <StatusChip :status="engine.status" />
                <span v-if="engine.binary_version" class="hint mono">{{ engine.binary_version }}</span>
              </div>
              <ul v-if="engine.reasons.length" class="quality__reasons">
                <li v-for="reason in engine.reasons" :key="reason">{{ reason }}</li>
              </ul>
              <div v-if="engine.shards.length" class="quality__shards">
                <span class="hint">{{ t('quality.shardStatuses') }}{{ t('common.colon') }}</span>
                <span v-for="[shard, status] in engine.shards" :key="shard" class="quality__shard">
                  <span class="hint">{{ shardName(shard) }}</span>
                  <StatusChip :status="status" />
                </span>
              </div>
            </li>
          </ul>
        </el-col>
      </el-row>

      <div class="quality__block">
        <h4>{{ t('quality.evidence') }} <span class="hint num">({{ failures.length }})</span></h4>
        <p v-if="!failures.length" class="hint">{{ t('quality.noFailures') }}</p>
        <el-collapse v-else class="quality__failures">
          <el-collapse-item v-for="(failure, index) in failures" :key="index" :name="index">
            <template #title>
              <span class="quality__failure-title">
                <span v-if="failure.engine" class="dot" :style="{'--dot-color': engineColorVar(failure.engine)}" aria-hidden="true" />
                <span class="quality__failure-meta">{{ failureTitle(failure) }}</span>
                <StatusChip v-if="failure.status" :status="failure.status" />
                <span class="quality__failure-text mono">{{ firstLine(failure.error) }}</span>
              </span>
            </template>
            <pre>{{ failure.error }}</pre>
          </el-collapse-item>
        </el-collapse>
      </div>
    </template>

    <div class="quality__files hint">
      <span v-if="summary">
        <span class="label">{{ t('quality.files') }}</span>
        <el-link v-for="name in PLATFORM_FILES" :key="name" :href="platformFile(name)" target="_blank" type="primary" class="mono">{{ name }}</el-link>
      </span>
      <span>
        <span class="label">{{ t('quality.runFiles') }}</span>
        <el-link v-for="name in RUN_FILES" :key="name" :href="runFile(name)" target="_blank" type="primary" class="mono">{{ name }}</el-link>
      </span>
    </div>
  </section>
</template>

<style scoped>
.quality__stats {
  margin-bottom: 16px;
  row-gap: 12px;
}

.quality__warnings {
  display: flex;
  flex-direction: column;
  gap: 8px;
  margin-bottom: 16px;
}

.quality__grid {
  margin-bottom: 20px;
  row-gap: 20px;
}

.quality__block {
  min-width: 0;
}

.quality__block h4 {
  margin-bottom: 8px;
}

.quality__list {
  list-style: none;
  margin: 0;
  padding: 0;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.quality__anomaly {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 6px 12px;
  font-size: var(--fs-13);
}

.quality__scenario {
  font-weight: 500;
}

.quality__engine {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 10px 12px;
  border-radius: var(--radius-inner);
  background: var(--control-fill);
  border: 1px solid var(--line-2);
  font-size: var(--fs-13);
}

.quality__engine-head {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 6px 12px;
}

.quality__reasons {
  margin: 0;
  padding-left: 18px;
  color: var(--ink-2);
}

.quality__shards {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 6px 10px;
}

.quality__shard {
  display: inline-flex;
  align-items: center;
  gap: 4px;
}

.quality__failure-title {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 6px 10px;
  font-size: var(--fs-13);
  line-height: 1.4;
  padding-right: 8px;
}

.quality__failure-meta {
  font-weight: 600;
}

.quality__failure-text {
  color: var(--status-fail);
  overflow-wrap: anywhere;
  white-space: normal;
}

.quality__files {
  display: flex;
  flex-wrap: wrap;
  gap: 8px 24px;
  margin-top: 16px;
  padding-top: 14px;
  border-top: 1px solid var(--line-2);
}

.quality__files > span {
  display: inline-flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 6px 12px;
}
</style>
