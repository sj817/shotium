<script setup lang="ts">
import {computed} from 'vue';
import {ArrowDown, Clock, Link, Promotion} from '@element-plus/icons-vue';
import {useI18n} from '../lib/i18n';
import {hasMessage} from '../lib/messages';
import {formatUtc, shortSha} from '../lib/format';
import {REPO_URL} from '../lib/labels';
import type {BenchmarkIndex, IndexEntry, LoadedRun} from '../lib/types';
import GlassPanel from './GlassPanel.vue';
import StatusChip from './StatusChip.vue';

const props = defineProps<{
  index: BenchmarkIndex | null;
  run: LoadedRun | null;
  selected: string | null;
}>();

const emit = defineEmits<{select: [path: string]}>();

const {t} = useI18n();

const entries = computed<IndexEntry[]>(() => props.index?.results ?? []);
const entry = computed(() => entries.value.find((candidate) => candidate.path === props.selected) ?? null);
const manifest = computed(() =>
  props.run && props.run.entry.path === props.selected ? props.run.manifest : null);

const version = computed(() => manifest.value?.shotium_version ?? entry.value?.shotium_version ?? null);
const generated = computed(() => manifest.value?.generated_utc ?? entry.value?.generated_utc ?? null);
const profile = computed(() => manifest.value?.profile ?? null);
const sha = computed(() => manifest.value?.source_sha ?? entry.value?.source_sha ?? null);
const runId = computed(() => manifest.value?.run_id ?? entry.value?.run_id ?? null);
const runUrl = computed(() =>
  manifest.value?.run_url ?? (runId.value ? `${REPO_URL}/actions/runs/${runId.value}` : null));
const attempt = computed(() => manifest.value?.run_attempt ?? null);
const archive = computed(() => manifest.value?.status ?? entry.value?.status ?? null);
const quality = computed(() => manifest.value?.quality_status ?? entry.value?.quality_status ?? null);
const evidence = computed(() => manifest.value?.evidence_status ?? entry.value?.evidence_status ?? null);

const overallStatusTone = computed<'pass' | 'fail' | 'noisy'>(() => {
  const statuses = [archive.value, quality.value, evidence.value];
  if (statuses.some((s) => s === 'fail' || s === 'incomplete' || s === 'error')) return 'fail';
  if (statuses.some((s) => s === 'noisy' || s === 'limited' || s === 'partial')) return 'noisy';
  if (statuses.every((s) => s === 'pass' || s === 'complete')) return 'pass';
  return 'noisy';
});

const overallStatusLabel = computed(() => {
  const tone = overallStatusTone.value;
  if (tone === 'pass') return t('run.healthPass');
  if (tone === 'fail') return t('run.healthFail');
  return t('run.healthNoisy');
});

function profileLabel(value: string | null): string {
  if (!value) return '—';
  const key = `run.profile.${value}`;
  return hasMessage(key) ? t(key) : value;
}

function profileHelp(value: string | null): string {
  const key = `run.profileHelp.${value ?? ''}`;
  return hasMessage(key) ? t(key) : '';
}

function runStatusLabel(status: string): string {
  const key = `runStatus.${status}`;
  return hasMessage(key) ? t(key) : status;
}

function optionLabel(item: IndexEntry): string {
  return t('run.option', {
    version: item.shotium_version,
    date: formatUtc(item.generated_utc, false),
    status: runStatusLabel(item.status),
  });
}

function onChange(value: string) {
  if (value) emit('select', value);
}
</script>

<template>
  <GlassPanel>
    <div class="runbar">
      <!-- Left: Run Version Picker -->
      <div class="runbar__pick">
        <span class="runbar__pick-label">{{ t('run.label') }}</span>
        <el-select
          :model-value="selected ?? ''"
          :disabled="!entries.length"
          popper-class="bench-pop runbar-select-pop"
          class="runbar__select"
          @change="onChange"
        >
          <el-option
            v-for="item in entries"
            :key="item.path"
            :value="item.path"
            :label="optionLabel(item)"
          />
        </el-select>
      </div>

      <!-- Center: Compact Meta Pills -->
      <div class="runbar__meta">
        <div class="meta-pill">
          <el-tooltip
            v-if="profileHelp(profile)"
            :content="profileHelp(profile)"
            placement="top"
            popper-class="bench-pop"
          >
            <span class="meta-pill__text">{{ profileLabel(profile) }}</span>
          </el-tooltip>
          <span v-else class="meta-pill__text">{{ profileLabel(profile) }}</span>
        </div>

        <div class="meta-pill">
          <el-icon class="meta-pill__icon"><Clock /></el-icon>
          <span class="num">{{ formatUtc(generated) }}</span>
        </div>

        <el-link
          v-if="sha"
          :href="`${REPO_URL}/commit/${sha}`"
          target="_blank"
          underline="never"
          class="meta-pill meta-pill--link mono"
        >
          <el-icon class="meta-pill__icon"><Link /></el-icon>
          <span>{{ shortSha(sha) }}</span>
        </el-link>

        <el-link
          v-if="runUrl"
          :href="runUrl"
          target="_blank"
          underline="never"
          class="meta-pill meta-pill--link"
        >
          <el-icon class="meta-pill__icon"><Promotion /></el-icon>
          <span class="num">#{{ runId }}</span>
          <span v-if="attempt && attempt > 1" class="meta-pill__sub">({{ t('run.attempt', {n: attempt}) }})</span>
        </el-link>
      </div>

      <!-- Right: Unified Health & Quality Badge -->
      <div class="runbar__health">
        <el-popover
          placement="bottom-end"
          :width="260"
          trigger="hover"
          :show-arrow="false"
          popper-class="bench-pop runbar-health-pop"
        >
          <template #reference>
            <button
              type="button"
              class="health-btn"
              :class="`health-btn--${overallStatusTone}`"
            >
              <span class="health-btn__dot" aria-hidden="true" />
              <span class="health-btn__text">{{ overallStatusLabel }}</span>
              <el-icon class="health-btn__arrow"><ArrowDown /></el-icon>
            </button>
          </template>

          <div class="health-pop-card">
            <h4 class="health-pop-title">{{ t('run.healthTitle') }}</h4>
            <div class="health-pop-list">
              <div class="health-pop-item">
                <span class="health-pop-label">{{ t('run.archive') }}</span>
                <StatusChip kind="archive" :status="archive" />
              </div>
              <div class="health-pop-item">
                <span class="health-pop-label">{{ t('run.quality') }}</span>
                <StatusChip kind="quality" :status="quality" />
              </div>
              <div class="health-pop-item">
                <span class="health-pop-label">{{ t('run.evidence') }}</span>
                <StatusChip kind="evidence" :status="evidence" />
              </div>
            </div>
          </div>
        </el-popover>
      </div>
    </div>
  </GlassPanel>
</template>

<style scoped>
.runbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px 16px;
  flex-wrap: wrap;
}

.runbar__pick {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-shrink: 0;
}

.runbar__pick-label {
  font-size: var(--fs-13, 13px);
  font-weight: 650;
  color: var(--ink-1);
  white-space: nowrap;
}

.runbar__select {
  width: 250px;
}

:deep(.runbar__select .el-select__wrapper) {
  background: var(--glass-fill-1, rgba(255, 255, 255, 0.55)) !important;
  border: 1px solid var(--glass-edge, rgba(255, 255, 255, 0.75)) !important;
  border-radius: var(--radius-chip, 999px) !important;
  box-shadow: none !important;
  backdrop-filter: blur(8px);
  transition: all 0.16s ease;
  padding: 3px 12px;
}

:deep(.runbar__select .el-select__wrapper:hover),
:deep(.runbar__select .el-select__wrapper.is-focused) {
  background: var(--glass-fill-2, rgba(255, 255, 255, 0.85)) !important;
  border-color: color-mix(in srgb, var(--brand) 40%, transparent) !important;
}

.runbar__meta {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
  flex: 1 1 auto;
}

.meta-pill {
  display: inline-flex;
  align-items: center;
  gap: 5px;
  padding: 4px 10px;
  font-size: 12px;
  color: var(--ink-2);
  background: var(--glass-fill-1, rgba(255, 255, 255, 0.45));
  border: 1px solid var(--glass-edge, rgba(255, 255, 255, 0.65));
  border-radius: var(--radius-chip, 999px);
  backdrop-filter: blur(6px);
  user-select: none;
}

.meta-pill--link {
  color: var(--brand, #146c43);
  text-decoration: none;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.16s ease;
}

.meta-pill--link:hover {
  background: var(--brand-soft, rgba(20, 108, 67, 0.14));
  border-color: color-mix(in srgb, var(--brand) 30%, transparent);
}

.meta-pill__icon {
  font-size: 13px;
  color: var(--ink-3);
}

.meta-pill--link .meta-pill__icon {
  color: var(--brand, #146c43);
}

.meta-pill__sub {
  font-size: 11px;
  color: var(--ink-3);
  margin-left: 2px;
}

.runbar__health {
  display: flex;
  align-items: center;
  flex-shrink: 0;
}

.health-btn {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 4px 12px;
  font-size: 12px;
  font-family: inherit;
  font-weight: 600;
  border-radius: var(--radius-chip, 999px);
  cursor: pointer;
  transition: all 0.16s ease;
  user-select: none;
  backdrop-filter: blur(8px);
}

.health-btn--pass {
  color: var(--brand, #146c43);
  background: rgba(20, 108, 67, 0.1);
  border: 1px solid rgba(20, 108, 67, 0.25);
}

.health-btn--pass .health-btn__dot {
  background: var(--brand, #146c43);
}

.health-btn--noisy {
  color: var(--status-noisy, #d97706);
  background: rgba(217, 119, 6, 0.1);
  border: 1px solid rgba(217, 119, 6, 0.25);
}

.health-btn--noisy .health-btn__dot {
  background: var(--status-noisy, #d97706);
}

.health-btn--fail {
  color: var(--status-fail, #dc2626);
  background: rgba(220, 38, 38, 0.1);
  border: 1px solid rgba(220, 38, 38, 0.25);
}

.health-btn--fail .health-btn__dot {
  background: var(--status-fail, #dc2626);
}

.health-btn:hover {
  filter: brightness(0.95);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
}

.health-btn__dot {
  width: 7px;
  height: 7px;
  border-radius: 50%;
  flex-shrink: 0;
}

.health-btn__arrow {
  font-size: 11px;
  opacity: 0.7;
}

:global(.el-popover.runbar-health-pop) {
  padding: 12px 14px !important;
  border-radius: 12px !important;
  background: rgba(255, 255, 255, 0.88) !important;
  backdrop-filter: blur(20px) !important;
  border: 1px solid rgba(255, 255, 255, 0.8) !important;
  box-shadow: 0 12px 32px -6px rgba(0, 0, 0, 0.12) !important;
}

:global(html.dark .el-popover.runbar-health-pop) {
  background: rgba(22, 28, 34, 0.88) !important;
  border-color: rgba(255, 255, 255, 0.12) !important;
}

.health-pop-card {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.health-pop-title {
  margin: 0;
  font-size: 13px;
  font-weight: 700;
  color: var(--ink-1);
  padding-bottom: 6px;
  border-bottom: 1px solid var(--line-2, rgba(0, 0, 0, 0.06));
}

.health-pop-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.health-pop-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
}

.health-pop-label {
  font-size: 12px;
  font-weight: 600;
  color: var(--ink-2);
}

@media (max-width: 768px) {
  .runbar {
    flex-direction: column;
    align-items: stretch;
    gap: 10px;
  }

  .runbar__select {
    width: 100%;
  }

  .runbar__health {
    justify-content: flex-start;
  }
}
</style>
