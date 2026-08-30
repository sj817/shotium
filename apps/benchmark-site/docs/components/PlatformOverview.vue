<script setup lang="ts">
import {computed, ref, watch} from 'vue';
import {
  Timer,
  ScaleToOriginal,
  TrendCharts,
  Platform,
  DataAnalysis,
  InfoFilled,
} from '@element-plus/icons-vue';
import {useI18n} from '../lib/i18n';
import type {MessageKey} from '../lib/messages';
import {statusColorVar} from '../lib/labels';
import type {PlatformReport, RunReport} from '../lib/report';
import {PLATFORM_IDS, type PlatformId} from '../lib/types';
import GlassPanel from './GlassPanel.vue';
import ScenarioCardGrid from './ScenarioCardGrid.vue';
import StatusChip from './StatusChip.vue';

const props = defineProps<{
  report: RunReport | null;
  modelValue?: PlatformId;
}>();

const emit = defineEmits<{'update:modelValue': [platform: PlatformId]}>();

const {t, platformName} = useI18n();

const internalSelected = ref<PlatformId>('linux-x64');
const mode = ref<'ms' | 'ratio' | 'throughput'>('ms');

function isPlatformId(value: string | undefined): value is PlatformId {
  return typeof value === 'string' && (PLATFORM_IDS as readonly string[]).includes(value);
}

const selectedPlatformId = computed<PlatformId>({
  get: () => (props.modelValue && isPlatformId(props.modelValue)) ? props.modelValue : internalSelected.value,
  set: (value: PlatformId) => {
    internalSelected.value = value;
    emit('update:modelValue', value);
  },
});

const currentSubtitle = computed(() => {
  if (mode.value === 'ratio') return t('overview.subtitleRatio');
  if (mode.value === 'throughput') return t('overview.subtitleThroughput');
  return t('overview.subtitle');
});

const currentPlatform = computed<PlatformReport | null>(() => {
  if (!props.report) return null;
  return props.report.byId[selectedPlatformId.value] ?? props.report.platforms[0] ?? null;
});

watch(() => props.report, (report) => {
  if (!report) return;
  if (report.byId[selectedPlatformId.value]?.data.summary) return;
  const preferred = report.platforms.find((platform) => platform.ranked) ??
    report.platforms.find((platform) => platform.data.summary !== null);
  if (preferred) selectedPlatformId.value = preferred.id;
}, {immediate: true});

function noRankingOf(platform: PlatformReport): {text: string; help: string} | null {
  const code = platform.noRankingReason;
  if (!code || platform.ranked) return null;
  return {
    text: t('overview.noRankingWith', {reason: t(`reasonShort.${code}` as MessageKey)}),
    help: `${t(`reason.${code}` as MessageKey)}。${t(`reasonHelp.${code}` as MessageKey)}`,
  };
}

function warningsOf(platform: PlatformReport): string[] {
  const summary = platform.data.summary;
  const out: string[] = [];
  if (!summary) {
    if (platform.data.loadError) out.push(t('quality.loadError', {error: platform.data.loadError}));
    return out;
  }
  if (summary.shards_complete === false) {
    out.push(t('quality.missingShards', {shards: (summary.missing_shards ?? []).join(', ') || '?'}));
  }
  if (summary.error) out.push(t('quality.summaryError', {error: summary.error}));
  if (platform.manifest?.evidence_complete === false) out.push(t('quality.evidenceIncomplete'));
  if (platform.manifest?.artifact_error) out.push(t('quality.artifactError', {error: platform.manifest.artifact_error}));
  return out;
}
</script>

<template>
  <div class="overview-layout">
    <!-- Main Benchmark Glass Card -->
    <GlassPanel
      id="overview"
      class="overview-layout__main"
      :title="t('overview.title')"
      :subtitle="currentSubtitle"
    >
      <!-- Top Right Rules Popover Button -->
      <template #head>
        <el-popover
          placement="bottom-end"
          :width="290"
          trigger="hover"
          :show-arrow="false"
          popper-class="bench-pop rules-popover"
        >
          <template #reference>
            <button type="button" class="rules-btn">
              <el-icon class="rules-btn__icon"><InfoFilled /></el-icon>
              <span>{{ t('overview.rulesBtn') }}</span>
            </button>
          </template>

          <div class="rules-card">
            <div class="rules-card__head">
              <span class="rules-card__title">{{ t('overview.rulesTitle') }}</span>
            </div>

            <!-- Group 1: Bar Visual Legend -->
            <div class="rules-group">
              <div class="rules-row">
                <span class="rules-swatch rules-swatch--hero" aria-hidden="true" />
                <div class="rules-row__text">
                  <span class="rules-row__label">Shotium</span>
                  <span class="rules-row__desc">{{ t('overview.baseline') }}</span>
                </div>
              </div>
              <div class="rules-row">
                <span class="rules-swatch rules-swatch--pass" aria-hidden="true" />
                <div class="rules-row__text">
                  <span class="rules-row__label">{{ t('overview.rulesPass') }}</span>
                  <span class="rules-row__desc">{{ t('overview.rulesPassDesc') }}</span>
                </div>
              </div>
              <div class="rules-row">
                <span class="rules-swatch rules-swatch--noisy" aria-hidden="true" />
                <div class="rules-row__text">
                  <span class="rules-row__label">{{ t('overview.rulesNoisy') }}</span>
                  <span class="rules-row__desc">{{ t('overview.rulesNoisyDesc') }}</span>
                </div>
              </div>
              <div class="rules-row">
                <span class="rules-swatch rules-swatch--fail" aria-hidden="true" />
                <div class="rules-row__text">
                  <span class="rules-row__label">{{ t('overview.rulesFail') }}</span>
                  <span class="rules-row__desc">{{ t('overview.rulesFailDesc') }}</span>
                </div>
              </div>
            </div>

            <div class="rules-divider" />

            <!-- Group 2: Key Notes -->
            <div class="rules-notes">
              <p class="rules-note">
                {{ t('overview.rulesDirection') }}
              </p>
              <p class="rules-note">
                {{ t('overview.rulesDifference') }}
              </p>
            </div>
          </div>
        </el-popover>
      </template>

      <p v-if="!report" class="hint">{{ t('app.loading') }}</p>

      <div v-else-if="currentPlatform">
        <!-- Optional Quality Warnings or Missing Ranking Reason -->
        <div v-if="warningsOf(currentPlatform).length || noRankingOf(currentPlatform)" class="overview-alerts">
          <span v-if="noRankingOf(currentPlatform)" class="overview-alert overview-alert--flag">
            {{ noRankingOf(currentPlatform)!.text }}：{{ noRankingOf(currentPlatform)!.help }}
          </span>
          <span v-for="(warning, index) in warningsOf(currentPlatform)" :key="index" class="overview-alert overview-alert--warning">
            ! {{ warning }}
          </span>
        </div>

        <ScenarioCardGrid
          v-if="currentPlatform.data.summary && currentPlatform.data.summary.scenarios.length"
          :key="`${currentPlatform.id}|${mode}`"
          :summary="currentPlatform.data.summary"
          :mode="mode"
          :label="t('overview.chartLabel', {platform: platformName(currentPlatform.id)})"
        />
        <el-empty v-else :description="currentPlatform.data.summary ? t('chart.empty') : t('quality.notArchived')" :image-size="64" />
      </div>
    </GlassPanel>

    <!-- Sticky Control Sidebar outside the card -->
    <aside v-if="report" class="overview-layout__sidebar">
      <div class="sidebar-panel glass">
        <!-- Metric Mode Section -->
        <div class="sidebar-section">
          <div class="sidebar-section__head">
            <el-icon class="sidebar-section__icon"><DataAnalysis /></el-icon>
            <span class="sidebar-section__title">{{ t('overview.modeLabel') }}</span>
          </div>
          <div class="sidebar-list" role="radiogroup" :aria-label="t('overview.modeLabel')">
            <button
              type="button"
              class="sidebar-item"
              :class="{'sidebar-item--active': mode === 'ms'}"
              @click="mode = 'ms'"
            >
              <el-icon class="sidebar-item__icon"><Timer /></el-icon>
              <span>{{ t('overview.modeMs') }}</span>
            </button>
            <button
              type="button"
              class="sidebar-item"
              :class="{'sidebar-item--active': mode === 'ratio'}"
              @click="mode = 'ratio'"
            >
              <el-icon class="sidebar-item__icon"><ScaleToOriginal /></el-icon>
              <span>{{ t('overview.modeRatio') }}</span>
            </button>
            <button
              type="button"
              class="sidebar-item"
              :class="{'sidebar-item--active': mode === 'throughput'}"
              @click="mode = 'throughput'"
            >
              <el-icon class="sidebar-item__icon"><TrendCharts /></el-icon>
              <span>{{ t('overview.modeThroughput') }}</span>
            </button>
          </div>
        </div>

        <div class="sidebar-divider" />

        <!-- Platform Switcher Section -->
        <div class="sidebar-section">
          <div class="sidebar-section__head">
            <el-icon class="sidebar-section__icon"><Platform /></el-icon>
            <span class="sidebar-section__title">{{ t('overview.platform') }}</span>
          </div>
          <nav class="sidebar-list" role="tablist" :aria-label="t('platform.select')">
            <button
              v-for="platform in report.platforms"
              :key="platform.id"
              type="button"
              role="tab"
              :aria-selected="selectedPlatformId === platform.id"
              class="sidebar-item"
              :class="{'sidebar-item--active': selectedPlatformId === platform.id}"
              @click="selectedPlatformId = platform.id"
            >
              <span class="tabdot" :style="{background: statusColorVar(platform.status)}" aria-hidden="true" />
              <span class="sidebar-item__name">{{ platformName(platform.id) }}</span>
              <span v-if="platform.data.summary && platform.comparable > 0" class="sidebar-item__badge">
                {{ platform.comparable }}
              </span>
            </button>
          </nav>
        </div>
      </div>
    </aside>
  </div>
</template>

<style scoped>
.overview-layout {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 200px;
  gap: 20px;
  align-items: start;
  width: 100%;
}

.overview-layout__main {
  min-width: 0;
}

.overview-layout__sidebar {
  position: sticky;
  top: 76px;
  z-index: 10;
}

.rules-btn {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 5px 13px;
  font-size: var(--fs-13, 13px);
  font-family: inherit;
  font-weight: 600;
  color: var(--brand, #146c43);
  background: var(--brand-soft, rgba(20, 108, 67, 0.12));
  border: 1px solid color-mix(in srgb, var(--brand) 28%, transparent);
  border-radius: var(--radius-chip, 999px);
  backdrop-filter: blur(8px);
  cursor: pointer;
  transition: all 0.16s ease;
  user-select: none;
}

.rules-btn:hover,
.rules-btn:focus-visible {
  color: #ffffff;
  background: var(--brand, #146c43);
  border-color: var(--brand, #146c43);
  box-shadow: 0 2px 8px rgba(20, 108, 67, 0.28);
}

.rules-btn:hover .rules-btn__icon,
.rules-btn:focus-visible .rules-btn__icon {
  color: #ffffff;
}

.rules-btn__icon {
  font-size: 14px;
  color: var(--brand, #146c43);
  transition: color 0.16s ease;
}

:global(.el-popover.rules-popover) {
  padding: 12px 14px !important;
  border-radius: 12px !important;
  background: rgba(255, 255, 255, 0.88) !important;
  backdrop-filter: blur(20px) saturate(180%) !important;
  -webkit-backdrop-filter: blur(20px) saturate(180%) !important;
  border: 1px solid rgba(255, 255, 255, 0.8) !important;
  box-shadow: 0 16px 36px -8px rgba(0, 0, 0, 0.12), 0 2px 8px rgba(0, 0, 0, 0.04) !important;
}

:global(html.dark .el-popover.rules-popover) {
  background: rgba(22, 28, 34, 0.88) !important;
  border-color: rgba(255, 255, 255, 0.12) !important;
  box-shadow: 0 16px 36px -8px rgba(0, 0, 0, 0.5) !important;
}

.rules-card {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.rules-card__head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding-bottom: 6px;
  border-bottom: 1px solid var(--line-2, rgba(0, 0, 0, 0.06));
}

.rules-card__title {
  font-size: 13px;
  font-weight: 700;
  color: var(--ink-1);
  letter-spacing: -0.01em;
}

.rules-group {
  display: flex;
  flex-direction: column;
  gap: 7px;
}

.rules-row {
  display: flex;
  align-items: center;
  gap: 10px;
}

.rules-swatch {
  width: 20px;
  height: 7px;
  border-radius: 999px;
  flex-shrink: 0;
}

.rules-swatch--hero {
  background: var(--brand, #146c43);
  box-shadow: 0 0 4px rgba(20, 108, 67, 0.35);
}

.rules-swatch--pass {
  background: #57626e;
}

.rules-swatch--noisy {
  background: repeating-linear-gradient(
    45deg,
    var(--status-noisy, #d97706),
    var(--status-noisy, #d97706) 3px,
    rgba(217, 119, 6, 0.3) 3px,
    rgba(217, 119, 6, 0.3) 6px
  );
}

.rules-swatch--fail {
  background: var(--status-fail, #dc2626);
}

.rules-row__text {
  display: flex;
  align-items: baseline;
  gap: 6px;
  font-size: 12px;
  line-height: 1.35;
}

.rules-row__label {
  font-weight: 600;
  color: var(--ink-1);
  white-space: nowrap;
}

.rules-row__desc {
  color: var(--ink-3);
  font-size: 11px;
}

.rules-divider {
  height: 1px;
  background: var(--line-2, rgba(0, 0, 0, 0.06));
}

.rules-notes {
  display: flex;
  flex-direction: column;
  gap: 5px;
}

.rules-note {
  margin: 0;
  font-size: 11.5px;
  line-height: 1.45;
  color: var(--ink-3);
}

.rules-note strong {
  color: var(--ink-2);
  font-weight: 600;
}

.overview-alerts {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px;
  margin-bottom: 14px;
}

.overview-alert {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 4px 10px;
  border-radius: 6px;
  font-size: var(--fs-12, 12px);
  font-weight: 600;
}

.overview-alert--flag {
  color: var(--status-noisy, #d97706);
  background: rgba(217, 119, 6, 0.1);
  border: 1px solid rgba(217, 119, 6, 0.2);
}

.overview-alert--warning {
  color: var(--status-fail, #dc2626);
  background: rgba(220, 38, 38, 0.1);
  border: 1px solid rgba(220, 38, 38, 0.2);
}

.sidebar-panel {
  padding: 14px;
  border-radius: var(--radius-inner, 14px);
  background: var(--glass-fill-1, rgba(255, 255, 255, 0.6));
  border: 1px solid var(--glass-edge, rgba(255, 255, 255, 0.7));
  box-shadow: var(--glass-shadow, 0 4px 16px rgba(0, 0, 0, 0.05));
  display: flex;
  flex-direction: column;
  gap: 12px;
  backdrop-filter: blur(12px);
}

.sidebar-section {
  display: flex;
  flex-direction: column;
  gap: 5px;
}

.sidebar-section__head {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 2px 4px 4px;
  font-size: 11px;
  font-weight: 700;
  color: var(--ink-3);
  text-transform: uppercase;
  letter-spacing: 0.04em;
}

.sidebar-section__icon {
  font-size: 13px;
  color: var(--brand, #146c43);
}

.sidebar-divider {
  height: 1px;
  background: var(--line-2, rgba(0, 0, 0, 0.06));
}

.sidebar-list {
  display: flex;
  flex-direction: column;
  gap: 3px;
}

.sidebar-item {
  display: flex;
  align-items: center;
  gap: 8px;
  width: 100%;
  padding: 7px 11px;
  font-size: var(--fs-13, 13px);
  font-family: inherit;
  font-weight: 550;
  color: var(--ink-2);
  background: transparent;
  border: 1px solid transparent;
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.16s ease;
  text-align: left;
  user-select: none;
}

.sidebar-item:hover {
  color: var(--ink-1);
  background: var(--control-fill-hover, rgba(255, 255, 255, 0.8));
}

.sidebar-item--active {
  color: var(--brand, #146c43) !important;
  background: var(--brand-soft, rgba(20, 108, 67, 0.14)) !important;
  border-color: color-mix(in srgb, var(--brand) 30%, transparent) !important;
  font-weight: 650;
}

.sidebar-item__icon {
  font-size: 14px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.sidebar-item__name {
  flex: 1;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.sidebar-item__badge {
  font-size: 11px;
  font-weight: 600;
  color: var(--ink-3);
  background: var(--line-2, rgba(0, 0, 0, 0.05));
  padding: 1px 6px;
  border-radius: 999px;
  font-variant-numeric: tabular-nums;
}

.sidebar-item--active .sidebar-item__badge {
  color: var(--brand, #146c43);
  background: rgba(20, 108, 67, 0.15);
}

@media (max-width: 992px) {
  .overview-layout {
    grid-template-columns: 1fr;
    gap: 16px;
  }

  .overview-layout__sidebar {
    position: static;
  }

  .sidebar-panel {
    flex-direction: row;
    flex-wrap: wrap;
    gap: 12px 20px;
  }

  .sidebar-section {
    flex: 1 1 auto;
  }

  .sidebar-list {
    flex-direction: row;
    flex-wrap: wrap;
    gap: 6px;
  }

  .sidebar-item {
    width: auto;
    padding: 6px 12px;
  }

  .sidebar-divider {
    display: none;
  }
}
</style>
