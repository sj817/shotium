<script setup lang="ts">
import {computed} from 'vue';
import {Trophy, TrendCharts, Medal, WarningFilled} from '@element-plus/icons-vue';
import {useI18n} from '../lib/i18n';
import type {MessageKey} from '../lib/messages';
import {formatRatio} from '../lib/format';
import type {RunReport} from '../lib/report';
import type {PlatformId} from '../lib/types';
import GlassPanel from './GlassPanel.vue';

const props = defineProps<{
  report: RunReport | null;
  phase: 'index' | 'run' | 'ready' | 'error' | 'empty';
  error: string;
}>();

const emit = defineEmits<{retry: []; open: [platform: PlatformId]}>();

const {t, locale, platformName, engineName} = useI18n();

const verdict = computed(() => props.report?.verdict ?? null);

const leadHeadline = computed(() => {
  const v = verdict.value;
  if (!v) return '';
  const ranked = v.ranked.length;
  if (ranked === 0) return t('verdict.leadNoneRanked', {tested: v.tested});
  if (v.shotiumFirst === ranked) {
    return t('verdict.leadAllFirst', {tested: v.tested, ranked});
  }
  return t('verdict.leadSomeFirst', {tested: v.tested, ranked, first: v.shotiumFirst});
});

const leadSubtitle = computed(() => {
  const v = verdict.value;
  if (!v || v.ratioMin === null || v.ratioMax === null) return '';
  const min = formatRatio(v.ratioMin, locale.value);
  const max = formatRatio(v.ratioMax, locale.value);
  if (min === max) {
    return t('verdict.subtitleSingle', {min});
  }
  return t('verdict.subtitleRange', {min, max});
});

const ratioMinFormatted = computed(() => {
  const v = verdict.value;
  if (!v || v.ratioMin === null) return '—';
  return formatRatio(v.ratioMin, locale.value);
});

const ratioMaxFormatted = computed(() => {
  const v = verdict.value;
  if (!v || v.ratioMax === null) return '—';
  return formatRatio(v.ratioMax, locale.value);
});

const closestEngineText = computed(() => {
  const c = verdict.value?.closest;
  if (!c) return '—';
  return `${engineName(c.engine)} · ${platformName(c.platform)}`;
});

const closestRatioText = computed(() => {
  const c = verdict.value?.closest;
  if (!c) return '—';
  return formatRatio(c.ratio, locale.value);
});

const unrankedPlatforms = computed(() => {
  return (verdict.value?.unranked ?? []).map((platform) => {
    const reason = platform.noRankingReason ?? 'not-archived';
    return {
      id: platform.id,
      name: platformName(platform.id),
      reasonCode: reason,
      reasonText: t(`reason.${reason}` as MessageKey),
      help: t(`reasonHelp.${reason}` as MessageKey),
    };
  });
});
</script>

<template>
  <GlassPanel>
    <div class="verdict" aria-live="polite">
      <template v-if="phase === 'ready' && report && verdict">
        <!-- Top Hero Headline -->
        <div class="verdict__hero">
          <div class="verdict__hero-badge">
            <el-icon class="verdict__hero-icon"><Trophy /></el-icon>
          </div>
          <div class="verdict__hero-text">
            <h2 class="verdict__title">{{ leadHeadline }}</h2>
            <p class="verdict__desc">{{ leadSubtitle }}</p>
          </div>
        </div>

        <!-- 3 Flat Frosted Glass Cards with Distinct Metric Accents -->
        <div class="verdict__cards">
          <!-- Card 1: Platform Win Rate (Emerald Green Accent) -->
          <div class="verdict-card verdict-card--green">
            <div class="verdict-card__header">
              <span class="verdict-card__label">{{ t('verdict.cardWinRate') }}</span>
              <span class="verdict-card__badge verdict-card__badge--green">{{ verdict.shotiumFirst === verdict.ranked.length ? t('verdict.badgeRankFirst') : t('verdict.badgeWinRate') }}</span>
            </div>
            <div class="verdict-card__body">
              <div class="verdict-card__val">
                <span class="verdict-card__num verdict-card__num--green">{{ verdict.shotiumFirst }}</span>
                <span class="verdict-card__denom">/ {{ verdict.ranked.length }} {{ t('noun.platform.other') }}</span>
              </div>
              <p class="verdict-card__desc">{{ t('verdict.cardWinRateDesc', {tested: verdict.tested}) }}</p>
            </div>
          </div>

          <!-- Card 2: Speedup Ratio Range (Electric Blue Accent) -->
          <div class="verdict-card verdict-card--blue">
            <div class="verdict-card__header">
              <span class="verdict-card__label">{{ t('verdict.cardRatio') }}</span>
              <span class="verdict-card__badge verdict-card__badge--blue">{{ ratioMinFormatted === ratioMaxFormatted ? t('verdict.badgeGeomean') : t('verdict.badgeRatioRange') }}</span>
            </div>
            <div class="verdict-card__body">
              <div class="verdict-card__val">
                <span class="verdict-card__num verdict-card__num--blue">{{ ratioMinFormatted }} ~ {{ ratioMaxFormatted }}</span>
              </div>
              <p class="verdict-card__desc">{{ t('verdict.cardRatioDesc') }}</p>
            </div>
          </div>

          <!-- Card 3: Closest Competitor (Purple/Indigo Accent) -->
          <div class="verdict-card verdict-card--purple">
            <div class="verdict-card__header">
              <span class="verdict-card__label">{{ t('verdict.cardClosest') }}</span>
              <span class="verdict-card__badge verdict-card__badge--purple">{{ t('verdict.badgeBestComp') }}</span>
            </div>
            <div class="verdict-card__body">
              <div class="verdict-card__val">
                <span class="verdict-card__num verdict-card__num--purple">{{ closestRatioText }}</span>
              </div>
              <p class="verdict-card__desc">{{ closestEngineText }}</p>
            </div>
          </div>
        </div>

        <!-- Bottom Meta: Clean Unranked Links & Rule Footnote -->
        <div class="verdict__meta">
          <div v-if="unrankedPlatforms.length" class="verdict__unranked">
            <span class="verdict__meta-label">{{ t('verdict.unrankedLabel') }}</span>
            <div class="verdict__unranked-list">
              <template v-for="(item, idx) in unrankedPlatforms" :key="item.id">
                <span v-if="idx > 0" class="verdict__meta-sep">/</span>
                <button
                  type="button"
                  class="unranked-link"
                  @click="emit('open', item.id)"
                >
                  <span class="unranked-link__name">{{ item.name }}</span>
                  <span class="unranked-link__reason">（{{ item.reasonText }}）</span>
                </button>
              </template>
            </div>
          </div>

          <div class="verdict__rule">
            * {{ t('verdict.rule') }}
          </div>
        </div>
      </template>

      <template v-else-if="phase === 'error'">
        <el-alert :title="t('app.loadError', {error})" type="error" show-icon :closable="false" />
        <p><el-button type="primary" size="small" @click="emit('retry')">{{ t('app.retry') }}</el-button></p>
      </template>

      <el-empty v-else-if="phase === 'empty'" :description="t('app.noRuns')" :image-size="72" />

      <p v-else class="verdict__lead verdict__lead--state verdict__lead--loading">
        {{ phase === 'run' ? t('app.loadingRun') : t('app.loading') }}
      </p>
    </div>
  </GlassPanel>
</template>

<style scoped>
.verdict {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

/* Hero Headline */
.verdict__hero {
  display: flex;
  align-items: flex-start;
  gap: 14px;
}

.verdict__hero-badge {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 40px;
  height: 40px;
  border-radius: 10px;
  background: var(--brand-soft, rgba(20, 108, 67, 0.14));
  border: 1px solid color-mix(in srgb, var(--brand) 28%, transparent);
  color: var(--brand, #146c43);
  flex-shrink: 0;
  margin-top: 2px;
}

.verdict__hero-icon {
  font-size: 20px;
}

.verdict__hero-text {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.verdict__title {
  margin: 0;
  font-size: clamp(18px, 1.8vw, 22px);
  font-weight: 700;
  color: var(--ink-1);
  line-height: 1.35;
  letter-spacing: -0.015em;
  word-break: normal;
  overflow-wrap: break-word;
}

.verdict__desc {
  margin: 0;
  font-size: var(--fs-14, 14px);
  color: var(--ink-2);
  line-height: 1.45;
  word-break: normal;
  overflow-wrap: break-word;
}

/* 3 Flat Frosted Glass Cards with Translucent Tint */
.verdict__cards {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14px;
  width: 100%;
  box-sizing: border-box;
}

@media (max-width: 768px) {
  .verdict__cards {
    grid-template-columns: 1fr;
  }
}

.verdict-card {
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  gap: 10px;
  padding: 16px 18px;
  border-radius: var(--radius-inner, 14px);
  backdrop-filter: blur(16px);
  transition: all 0.2s cubic-bezier(0.2, 0.8, 0.2, 1);
  cursor: default;
  min-width: 0;
  min-height: 114px;
  box-sizing: border-box;
}

/* Green Card */
.verdict-card--green {
  background: rgba(20, 108, 67, 0.06);
  border: 1px solid rgba(20, 108, 67, 0.22);
  box-shadow: 0 2px 10px rgba(20, 108, 67, 0.04);
}

.verdict-card--green:hover {
  background: rgba(20, 108, 67, 0.12);
  border-color: rgba(20, 108, 67, 0.45);
  box-shadow: 0 8px 24px -4px rgba(20, 108, 67, 0.2);
  transform: translateY(-2px);
}

.verdict-card--green:hover .verdict-card__badge--green {
  background: var(--brand, #146c43);
  color: #ffffff;
}

/* Blue Card */
.verdict-card--blue {
  background: rgba(2, 132, 199, 0.06);
  border: 1px solid rgba(2, 132, 199, 0.22);
  box-shadow: 0 2px 10px rgba(2, 132, 199, 0.04);
}

.verdict-card--blue:hover {
  background: rgba(2, 132, 199, 0.12);
  border-color: rgba(2, 132, 199, 0.45);
  box-shadow: 0 8px 24px -4px rgba(2, 132, 199, 0.2);
  transform: translateY(-2px);
}

.verdict-card--blue:hover .verdict-card__badge--blue {
  background: #0284c7;
  color: #ffffff;
}

/* Purple Card */
.verdict-card--purple {
  background: rgba(124, 58, 237, 0.06);
  border: 1px solid rgba(124, 58, 237, 0.22);
  box-shadow: 0 2px 10px rgba(124, 58, 237, 0.04);
}

.verdict-card--purple:hover {
  background: rgba(124, 58, 237, 0.12);
  border-color: rgba(124, 58, 237, 0.45);
  box-shadow: 0 8px 24px -4px rgba(124, 58, 237, 0.2);
  transform: translateY(-2px);
}

.verdict-card--purple:hover .verdict-card__badge--purple {
  background: #7c3aed;
  color: #ffffff;
}

/* Dark mode adjustments */
:global(html.dark) .verdict-card--green {
  background: rgba(52, 211, 153, 0.08);
  border-color: rgba(52, 211, 153, 0.25);
}
:global(html.dark) .verdict-card--green:hover {
  background: rgba(52, 211, 153, 0.16);
  border-color: rgba(52, 211, 153, 0.5);
  box-shadow: 0 8px 24px -4px rgba(52, 211, 153, 0.3);
}

:global(html.dark) .verdict-card--blue {
  background: rgba(56, 189, 248, 0.08);
  border-color: rgba(56, 189, 248, 0.25);
}
:global(html.dark) .verdict-card--blue:hover {
  background: rgba(56, 189, 248, 0.16);
  border-color: rgba(56, 189, 248, 0.5);
  box-shadow: 0 8px 24px -4px rgba(56, 189, 248, 0.3);
}

:global(html.dark) .verdict-card--purple {
  background: rgba(167, 139, 250, 0.08);
  border-color: rgba(167, 139, 250, 0.25);
}
:global(html.dark) .verdict-card--purple:hover {
  background: rgba(167, 139, 250, 0.16);
  border-color: rgba(167, 139, 250, 0.5);
  box-shadow: 0 8px 24px -4px rgba(167, 139, 250, 0.3);
}

.verdict-card__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
  min-width: 0;
  width: 100%;
}

.verdict-card__label {
  font-size: 13px;
  font-weight: 650;
  color: var(--ink-2);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  min-width: 0;
}

.verdict-card__badge {
  font-size: 11px;
  font-weight: 600;
  padding: 2px 8px;
  border-radius: 999px;
  transition: all 0.2s ease;
  flex-shrink: 0;
  white-space: nowrap;
}

.verdict-card__badge--green {
  color: var(--brand, #146c43);
  background: rgba(20, 108, 67, 0.12);
}

.verdict-card__badge--blue {
  color: #0284c7;
  background: rgba(2, 132, 199, 0.12);
}

.verdict-card__badge--purple {
  color: #7c3aed;
  background: rgba(124, 58, 237, 0.12);
}

.verdict-card__body {
  display: flex;
  flex-direction: column;
  gap: 4px;
  min-width: 0;
  width: 100%;
}

.verdict-card__val {
  display: flex;
  align-items: baseline;
  gap: 6px;
  min-width: 0;
  flex-wrap: wrap;
}

.verdict-card__num {
  font-size: 24px;
  font-weight: 800;
  font-variant-numeric: tabular-nums;
  letter-spacing: -0.03em;
  line-height: 1.15;
}

.verdict-card__num--green {
  color: var(--brand, #146c43);
}

.verdict-card__num--blue {
  color: #0284c7;
}

.verdict-card__num--purple {
  color: #7c3aed;
}

:global(html.dark) .verdict-card__num--green {
  color: #34d399;
}

:global(html.dark) .verdict-card__num--blue {
  color: #38bdf8;
}

:global(html.dark) .verdict-card__num--purple {
  color: #a78bfa;
}

.verdict-card__denom {
  font-size: 13px;
  font-weight: 600;
  color: var(--ink-3);
}

.verdict-card__desc {
  margin: 0;
  font-size: 12px;
  color: var(--ink-3);
  line-height: 1.4;
  white-space: normal;
  word-break: normal;
  overflow-wrap: break-word;
}

/* Bottom Meta Area */
.verdict__meta {
  display: flex;
  flex-direction: column;
  gap: 8px;
  font-size: 12px;
  padding-top: 4px;
  width: 100%;
  box-sizing: border-box;
}

.verdict__unranked {
  display: flex;
  align-items: center;
  gap: 6px;
  flex-wrap: wrap;
  width: 100%;
  box-sizing: border-box;
}

.verdict__meta-label {
  color: var(--ink-3);
  font-weight: 600;
  flex-shrink: 0;
}

.verdict__unranked-list {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  flex-wrap: wrap;
  min-width: 0;
}

.verdict__meta-sep {
  color: var(--ink-3);
  opacity: 0.4;
}

.unranked-link {
  display: inline-flex;
  align-items: baseline;
  gap: 3px;
  background: transparent;
  border: none;
  padding: 2px 4px;
  font-size: 12px;
  font-family: inherit;
  cursor: pointer;
  border-radius: 4px;
  transition: all 0.15s ease;
  white-space: normal;
  text-align: left;
}

.unranked-link:hover {
  background: var(--brand-soft, rgba(20, 108, 67, 0.1));
}

.unranked-link__name {
  color: var(--brand, #146c43);
  font-weight: 600;
  text-decoration: underline;
  text-underline-offset: 3px;
  text-decoration-color: color-mix(in srgb, var(--brand) 40%, transparent);
}

.unranked-link:hover .unranked-link__name {
  text-decoration-color: var(--brand);
}

.unranked-link__reason {
  color: var(--ink-3);
  font-size: 11.5px;
}

.verdict__rule {
  color: var(--ink-3);
  font-size: 11.5px;
  line-height: 1.5;
  word-break: normal;
  overflow-wrap: break-word;
  white-space: normal;
}

.verdict__lead--state {
  font-weight: 600;
  color: var(--ink-2);
}

.verdict__lead--loading {
  animation: verdict-pulse 1.6s ease-in-out infinite;
}

@keyframes verdict-pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.55; }
}

@media (prefers-reduced-motion: reduce) {
  .verdict__lead--loading {
    animation: none;
  }
}

/* Responsive */
@media (max-width: 768px) {
  .verdict__stats {
    flex-direction: column;
    gap: 14px;
    padding: 14px;
  }

  .stat-col__divider {
    width: 100%;
    height: 1px;
    margin: 0;
  }

  .verdict__meta {
    flex-direction: column;
    align-items: flex-start;
    gap: 8px;
  }
}
</style>
