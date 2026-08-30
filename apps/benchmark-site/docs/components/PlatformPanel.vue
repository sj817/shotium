<script setup lang="ts">
import {computed, ref, watch} from 'vue';
import {useI18n} from '../lib/i18n';
import type {MessageKey} from '../lib/messages';
import {statusColorVar} from '../lib/labels';
import type {PlatformReport, RunReport} from '../lib/report';
import type {PlatformId} from '../lib/types';
import GlassPanel from './GlassPanel.vue';
import StatusChip from './StatusChip.vue';
import RankingSection from './RankingSection.vue';
import ScenarioTable from './ScenarioTable.vue';
import QualitySection from './QualitySection.vue';

const props = defineProps<{report: RunReport}>();

const {t, platformName} = useI18n();

const selectedPlatformId = ref<PlatformId>('linux-x64');

watch(() => props.report, (report) => {
  if (!report) return;
  if (report.byId[selectedPlatformId.value]?.data.summary) return;
  const preferred = report.platforms.find((platform) => platform.ranked) ??
    report.platforms.find((platform) => platform.data.summary !== null);
  if (preferred) selectedPlatformId.value = preferred.id;
}, {immediate: true});

const currentPlatform = computed<PlatformReport | null>(() => {
  return props.report.byId[selectedPlatformId.value] ?? props.report.platforms[0] ?? null;
});

function reasonText(platform: PlatformReport): string {
  const code = platform.noRankingReason;
  return code ? t(`reason.${code}` as MessageKey) : '';
}

function warningsOf(platform: PlatformReport): Array<{tone: 'warning' | 'error'; text: string}> {
  const summary = platform.data.summary;
  const out: Array<{tone: 'warning' | 'error'; text: string}> = [];
  if (!summary) {
    out.push({tone: 'error', text: t('quality.notArchived')});
    if (platform.data.loadError) out.push({tone: 'error', text: t('quality.loadError', {error: platform.data.loadError})});
    return out;
  }
  if (summary.shards_complete === false) {
    out.push({tone: 'warning', text: t('quality.missingShards', {shards: (summary.missing_shards ?? []).join(', ') || '?'})});
  }
  if (summary.error) out.push({tone: 'warning', text: t('quality.summaryError', {error: summary.error})});
  if (platform.manifest?.evidence_complete === false) out.push({tone: 'warning', text: t('quality.evidenceIncomplete')});
  if (platform.manifest?.artifact_error) {
    out.push({tone: 'error', text: t('quality.artifactError', {error: platform.manifest.artifact_error})});
  }
  return out;
}
</script>

<template>
  <GlassPanel id="platform" class="platform-detail-panel" dense>
    <div class="platform-container">
      <!-- Decoupled Frosted Glass Platform Tabs -->
      <nav class="platform-tabs" role="tablist" :aria-label="t('platform.select')">
        <button
          v-for="platform in report.platforms"
          :key="platform.id"
          type="button"
          role="tab"
          :aria-selected="selectedPlatformId === platform.id"
          class="platform-tab"
          :class="{'platform-tab--active': selectedPlatformId === platform.id}"
          @click="selectedPlatformId = platform.id"
        >
          <span class="tabdot" :style="{background: statusColorVar(platform.status)}" aria-hidden="true" />
          <span>{{ platformName(platform.id) }}</span>
        </button>
      </nav>

      <!-- Active Platform Deep Dive Content -->
      <div v-if="currentPlatform" :key="currentPlatform.id" class="platform-content">
        <header class="platform__head">
          <h2>{{ platformName(currentPlatform.id) }}</h2>
          <StatusChip :status="currentPlatform.status" />
          <span v-if="currentPlatform.data.summary" class="hint">
            <el-tooltip :content="t('overview.comparableHelp')" placement="top" popper-class="bench-pop">
              <span class="comparable-badge">{{ t('overview.comparable', {n: currentPlatform.comparable}) }}</span>
            </el-tooltip>
            <template v-if="!currentPlatform.ranked"> · {{ t('overview.noRanking') }}{{ t('common.colon') }}{{ reasonText(currentPlatform) }}</template>
          </span>
        </header>

        <div v-if="warningsOf(currentPlatform).length" class="platform__warnings">
          <el-alert
            v-for="(warning, index) in warningsOf(currentPlatform)"
            :key="index"
            :title="warning.text"
            :type="warning.tone"
            show-icon
            :closable="false"
          />
        </div>

        <template v-if="currentPlatform.data.summary">
          <RankingSection :platform="currentPlatform" />
          <ScenarioTable :summary="currentPlatform.data.summary" />
        </template>
        <QualitySection :platform="currentPlatform" :run-path="report.run.entry.path" />
      </div>
    </div>
  </GlassPanel>
</template>

<style scoped>
.platform-container {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.platform-tabs {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 6px;
  padding-bottom: 12px;
  border-bottom: 1px solid var(--line-2, rgba(0, 0, 0, 0.06));
}

.platform-tab {
  display: inline-flex;
  align-items: center;
  gap: 7px;
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

.platform-tab:hover {
  color: var(--ink-1);
  background: var(--glass-fill-2, rgba(255, 255, 255, 0.85));
  border-color: var(--line-1, rgba(0, 0, 0, 0.14));
}

.platform-tab--active {
  color: var(--brand, #146c43) !important;
  background: var(--brand-soft, rgba(20, 108, 67, 0.14)) !important;
  border-color: color-mix(in srgb, var(--brand) 35%, transparent) !important;
  font-weight: 650;
}

.comparable-badge {
  font-size: 11px;
  font-weight: 600;
  color: var(--ink-3);
  background: var(--line-2, rgba(0, 0, 0, 0.06));
  padding: 2px 7px;
  border-radius: 999px;
  cursor: help;
}

.platform__head {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px 14px;
}

.platform__warnings {
  display: flex;
  flex-direction: column;
  gap: 8px;
  margin-top: 6px;
}
</style>
