<script setup lang="ts">
import {computed} from 'vue';
import {useI18n} from '../lib/i18n';
import type {MessageKey} from '../lib/messages';
import {formatRatio} from '../lib/format';
import {compareScenarios, engineColorVar} from '../lib/labels';
import {comparableBaselines, type RankingRow} from '../lib/ranking';
import type {PlatformReport} from '../lib/report';
import RankingChart from './RankingChart.vue';
import StatusChip from './StatusChip.vue';
import Tip from './Tip.vue';

/*
 * Ranking for one platform: an ECharts bar chart of the scores next to an
 * Element Plus table with rank, coverage, wins, status and exclusion reasons.
 */
const props = defineProps<{platform: PlatformReport}>();

const {t, locale, engineName, exclusionLabel, scenarioName} = useI18n();

const rows = computed(() => props.platform.rows);

const comparable = computed(() => {
  const summary = props.platform.data.summary;
  if (!summary) return [];
  return [...comparableBaselines(summary).values()].sort(compareScenarios);
});

// Element Plus types table-row slot props as a loose record; the rows are RankingRow.
function isOfficial(input: unknown): boolean {
  const row = input as RankingRow;
  return row.official && props.platform.ranked;
}

function rankOf(input: unknown): number | null {
  const row = input as RankingRow;
  return isOfficial(row) ? props.platform.ranks.get(row.engine) ?? null : null;
}

function reasonText(): string {
  const code = props.platform.noRankingReason;
  return code ? t(`reason.${code}` as MessageKey) : '';
}
</script>

<template>
  <section class="section ranking">
    <div class="section__head">
      <h3>{{ t('ranking.title') }}</h3>
      <span class="hint">{{ t('ranking.subtitle') }}</span>
      <span class="hint">
        <Tip :text="t('overview.comparableHelp')">
          <span class="tip-trigger">{{ t('ranking.basedOn', {n: platform.comparable}) }}</span>
        </Tip>
      </span>
    </div>

    <el-alert
      v-if="!platform.ranked"
      class="ranking__notice"
      type="warning"
      show-icon
      :closable="false"
      :title="t('ranking.notProduced', {reason: reasonText()})"
      :description="t('ranking.notProducedHelp')"
    />

    <p v-if="!rows.length" class="hint">{{ t('ranking.empty') }}</p>

    <div v-else class="ranking__grid">
      <RankingChart :rows="rows" :ranked="platform.ranked" />

      <el-table :data="rows" size="small" table-layout="auto" class="ranking__table">
        <el-table-column :label="t('ranking.rank')" width="72">
          <template #default="{row}">
            <el-tag v-if="rankOf(row) !== null" :type="rankOf(row) === 1 ? 'primary' : 'info'" :effect="rankOf(row) === 1 ? 'dark' : 'light'" round size="small">{{ rankOf(row) }}</el-tag>
            <span v-else class="hint">{{ row.geometricMean !== null ? t('ranking.reference') : '—' }}</span>
          </template>
        </el-table-column>
        <el-table-column :label="t('ranking.engine')" min-width="150">
          <template #default="{row}">
            <span class="engine" :class="{'engine--shotium': row.engine === 'shotium'}" :style="{'--dot-color': engineColorVar(row.engine)}">
              <span class="dot" aria-hidden="true" />{{ engineName(row.engine) }}
            </span>
          </template>
        </el-table-column>
        <el-table-column align="right" width="92">
          <template #header>{{ t('ranking.score') }}<Tip :text="t('ranking.scoreHelp')" /></template>
          <template #default="{row}">
            <strong v-if="row.geometricMean !== null" class="num">{{ isOfficial(row) ? '' : '≈' }}{{ formatRatio(row.geometricMean, locale) }}</strong>
            <span v-else class="hint">—</span>
          </template>
        </el-table-column>
        <el-table-column align="right" width="92">
          <template #header>{{ t('ranking.compared') }}<Tip :text="t('ranking.comparedHelp')" /></template>
          <template #default="{row}"><span class="num">{{ row.compared }}<span class="hint">/{{ row.totalBaselines }}</span></span></template>
        </el-table-column>
        <el-table-column align="right" width="80">
          <template #header>{{ t('ranking.champions') }}<Tip :text="t('ranking.championsHelp')" /></template>
          <template #default="{row}"><span class="num">{{ row.champions }}</span></template>
        </el-table-column>
        <el-table-column :label="t('ranking.status')" width="110">
          <template #default="{row}"><StatusChip :status="row.status" /></template>
        </el-table-column>
        <el-table-column :label="t('ranking.exclusions')" min-width="200">
          <template #default="{row}">
            <template v-if="row.exclusions.length">
              <div v-for="exclusion in row.exclusions" :key="exclusion.code" class="ranking__exclusion">
                <Tip v-if="exclusion.detail" :text="exclusion.detail">
                  <span class="tip-trigger">{{ exclusionLabel(exclusion.code, exclusion.count) }}</span>
                </Tip>
                <template v-else>{{ exclusionLabel(exclusion.code, exclusion.count) }}</template>
              </div>
            </template>
            <span v-else class="hint">{{ t('common.none') }}</span>
          </template>
        </el-table-column>
      </el-table>
    </div>

    <p v-if="comparable.length" class="ranking__items hint">
      <span>{{ t('ranking.comparableList') }}{{ t('common.colon') }}</span>
      <el-tag v-for="row in comparable" :key="`${row.scenario}|${row.concurrency}`" type="info" effect="plain" round size="small">
        {{ scenarioName(row.scenario, row.concurrency) }}
      </el-tag>
    </p>
  </section>
</template>

<style scoped>
.ranking__notice {
  margin-bottom: 14px;
}

.ranking__grid {
  display: grid;
  grid-template-columns: minmax(0, 1fr);
  gap: 16px;
  align-items: start;
}

.ranking__exclusion {
  white-space: normal;
  color: var(--ink-2);
  line-height: 1.4;
}

.ranking__items {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 6px;
  margin-top: 16px;
}

</style>
