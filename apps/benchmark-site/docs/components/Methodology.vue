<script setup lang="ts">
import {withBase} from 'vitepress';
import {
  ScaleToOriginal,
  Timer,
  Check,
  TrendCharts,
  Warning,
  FolderChecked,
  Document,
  Link,
} from '@element-plus/icons-vue';
import {useI18n} from '../lib/i18n';
import type {MessageKey} from '../lib/messages';
import {REPO_URL} from '../lib/labels';
import GlassPanel from './GlassPanel.vue';

const {t} = useI18n();

interface MethodCard {
  icon: typeof ScaleToOriginal;
  tone: 'green' | 'blue' | 'purple' | 'orange' | 'cyan' | 'brand';
  title: MessageKey;
  body: MessageKey;
}

const CARDS: MethodCard[] = [
  {icon: ScaleToOriginal, tone: 'green', title: 'method.fair.title', body: 'method.fair.body'},
  {icon: Timer, tone: 'blue', title: 'method.p50.title', body: 'method.p50.body'},
  {icon: Check, tone: 'green', title: 'method.comparable.title', body: 'method.comparable.body'},
  {icon: TrendCharts, tone: 'purple', title: 'method.geomean.title', body: 'method.geomean.body'},
  {icon: Warning, tone: 'orange', title: 'method.noisy.title', body: 'method.noisy.body'},
  {icon: FolderChecked, tone: 'cyan', title: 'method.evidence.title', body: 'method.evidence.body'},
];
</script>

<template>
  <GlassPanel id="method" class="method-panel">
    <!-- Header -->
    <div class="method-header">
      <div class="method-header__main">
        <h2 class="method-header__title">{{ t('method.title') }}</h2>
        <span class="method-header__badge">{{ t('method.summary') }}</span>
      </div>

      <div class="method-header__links">
        <el-link
          :href="withBase('/benchmark-results/README.md')"
          target="_blank"
          underline="never"
          class="method-link mono"
        >
          <el-icon class="method-link__icon"><Document /></el-icon>
          <span>benchmark-results/README.md</span>
        </el-link>
        <el-link
          :href="`${REPO_URL}/tree/main/apps/benchmark`"
          target="_blank"
          underline="never"
          class="method-link mono"
        >
          <el-icon class="method-link__icon"><Link /></el-icon>
          <span>apps/benchmark</span>
        </el-link>
      </div>
    </div>

    <!-- 6 Methodology Cards Matrix -->
    <div class="method-grid">
      <article
        v-for="card in CARDS"
        :key="card.title"
        class="method-card"
        :class="`method-card--${card.tone}`"
      >
        <div class="method-card__head">
          <div class="method-card__icon-wrap">
            <el-icon class="method-card__icon"><component :is="card.icon" /></el-icon>
          </div>
          <h3 class="method-card__title">{{ t(card.title) }}</h3>
        </div>
        <p class="method-card__body">{{ t(card.body) }}</p>
      </article>
    </div>
  </GlassPanel>
</template>

<style scoped>
.method-panel {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.method-header {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: space-between;
  gap: 12px 20px;
  padding-bottom: 14px;
  border-bottom: 1px solid var(--line-2, rgba(0, 0, 0, 0.06));
}

.method-header__main {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 10px 14px;
}

.method-header__title {
  margin: 0;
  font-size: var(--fs-18, 18px);
  font-weight: 750;
  color: var(--ink-1);
  letter-spacing: -0.01em;
}

.method-header__badge {
  font-size: 12px;
  color: var(--ink-3);
  background: var(--line-2, rgba(0, 0, 0, 0.05));
  padding: 3px 10px;
  border-radius: var(--radius-chip, 999px);
  font-weight: 500;
}

.method-header__links {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px 12px;
}

.method-link {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 4px 12px;
  font-size: 12px;
  font-weight: 600;
  color: var(--brand, #146c43);
  background: var(--glass-fill-1, rgba(255, 255, 255, 0.5));
  border: 1px solid var(--glass-edge, rgba(255, 255, 255, 0.7));
  border-radius: var(--radius-chip, 999px);
  backdrop-filter: blur(8px);
  transition: all 0.16s ease;
}

.method-link:hover {
  background: var(--brand-soft, rgba(20, 108, 67, 0.12));
  border-color: color-mix(in srgb, var(--brand) 30%, transparent);
}

.method-link__icon {
  font-size: 13px;
}

/* 6 Cards Matrix Grid */
.method-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(min(320px, 100%), 1fr));
  gap: 14px;
}

.method-card {
  display: flex;
  flex-direction: column;
  gap: 10px;
  padding: 16px 18px;
  background: var(--glass-fill-1, rgba(255, 255, 255, 0.5));
  border: 1px solid var(--glass-edge, rgba(255, 255, 255, 0.7));
  border-radius: var(--radius-inner, 14px);
  backdrop-filter: blur(14px);
  transition: all 0.2s cubic-bezier(0.2, 0.8, 0.2, 1);
}

.method-card:hover {
  transform: translateY(-2px);
  background: var(--glass-fill-2, rgba(255, 255, 255, 0.85));
  box-shadow: 0 8px 24px -4px rgba(0, 0, 0, 0.08);
}

.method-card__head {
  display: flex;
  align-items: center;
  gap: 10px;
}

.method-card__icon-wrap {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 30px;
  height: 30px;
  border-radius: 8px;
  flex-shrink: 0;
  transition: all 0.2s ease;
}

.method-card__title {
  margin: 0;
  font-size: var(--fs-14, 14px);
  font-weight: 700;
  color: var(--ink-1);
  letter-spacing: -0.01em;
}

.method-card__body {
  margin: 0;
  font-size: 12.5px;
  line-height: 1.6;
  color: var(--ink-2);
}

/* Tone Color Variations */
.method-card--green .method-card__icon-wrap {
  background: rgba(20, 108, 67, 0.12);
  color: var(--brand, #146c43);
}

.method-card--green:hover {
  border-color: rgba(20, 108, 67, 0.35);
  box-shadow: 0 8px 24px -4px rgba(20, 108, 67, 0.15);
}

.method-card--blue .method-card__icon-wrap {
  background: rgba(2, 132, 199, 0.12);
  color: #0284c7;
}

.method-card--blue:hover {
  border-color: rgba(2, 132, 199, 0.35);
  box-shadow: 0 8px 24px -4px rgba(2, 132, 199, 0.15);
}

.method-card--purple .method-card__icon-wrap {
  background: rgba(124, 58, 237, 0.12);
  color: #7c3aed;
}

.method-card--purple:hover {
  border-color: rgba(124, 58, 237, 0.35);
  box-shadow: 0 8px 24px -4px rgba(124, 58, 237, 0.15);
}

.method-card--orange .method-card__icon-wrap {
  background: rgba(217, 119, 6, 0.12);
  color: #d97706;
}

.method-card--orange:hover {
  border-color: rgba(217, 119, 6, 0.35);
  box-shadow: 0 8px 24px -4px rgba(217, 119, 6, 0.15);
}

.method-card--cyan .method-card__icon-wrap {
  background: rgba(14, 165, 233, 0.12);
  color: #0ea5e9;
}

.method-card--cyan:hover {
  border-color: rgba(14, 165, 233, 0.35);
  box-shadow: 0 8px 24px -4px rgba(14, 165, 233, 0.15);
}

@media (max-width: 768px) {
  .method-header {
    flex-direction: column;
    align-items: flex-start;
  }
}
</style>
