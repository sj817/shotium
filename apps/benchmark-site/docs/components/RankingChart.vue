<script setup lang="ts">
import {computed, defineAsyncComponent} from 'vue';
import type {EChartsOption} from 'echarts';
import {useI18n} from '../lib/i18n';
import {formatRatio} from '../lib/format';
import {isKnownEngine} from '../lib/labels';
import type {RankingRow} from '../lib/ranking';
import {chartPalette, cssVar, useThemeTick} from '../lib/theme';

/*
 * Stock ECharts horizontal bar chart of the geometric-mean scores of one
 * platform: one bar per engine in rank order, engine colours, value labels;
 * reference scores (partial coverage) are faded and prefixed with "≈".
 */
const VChart = defineAsyncComponent(async () => {
  const [chart] = await Promise.all([import('vue-echarts'), import('../lib/echarts')]);
  return chart.default;
});

const props = defineProps<{rows: RankingRow[]; ranked: boolean}>();

const {locale, engineName} = useI18n();
const tick = useThemeTick();

const list = computed(() => props.rows.filter((row) => row.geometricMean !== null));
const height = computed(() => 36 + list.value.length * 36);

function isOfficial(row: RankingRow): boolean {
  return row.official && props.ranked;
}

const option = computed<EChartsOption>(() => {
  void tick.value;
  const colors = chartPalette();
  const rows = list.value;
  return {
    textStyle: {fontFamily: colors.font},
    grid: {
      top: 4,
      left: 8,
      right: 56,
      bottom: 4,
      outerBoundsMode: 'same',
      outerBoundsContain: 'axisLabel',
    },
    tooltip: {
      trigger: 'item',
      backgroundColor: colors.pop,
      borderColor: colors.edge,
      textStyle: {color: colors.ink1},
      valueFormatter: (value: unknown) => formatRatio(Number(value), locale.value),
    },
    xAxis: {
      type: 'value',
      axisLabel: {formatter: '{value}×', color: colors.ink3},
      splitLine: {lineStyle: {color: colors.line2}},
    },
    yAxis: {
      type: 'category',
      inverse: true,
      data: rows.map((row) => engineName(row.engine)),
      axisLabel: {color: colors.ink2},
      axisTick: {show: false},
      axisLine: {show: false},
    },
    series: [{
      type: 'bar',
      barWidth: 16,
      data: rows.map((row) => ({
        value: Math.round((row.geometricMean as number) * 100) / 100,
        itemStyle: {
          color: isKnownEngine(row.engine) ? cssVar(`--engine-${row.engine}`) : cssVar('--engine-other'),
          opacity: isOfficial(row) ? 1 : 0.45,
          borderRadius: 4,
        },
      })),
      label: {
        show: true,
        position: 'right',
        color: colors.ink1,
        formatter: (params: {dataIndex: number; value: unknown}) => {
          const row = rows[params.dataIndex];
          return `${row && !isOfficial(row) ? '≈' : ''}${formatRatio(Number(params.value), locale.value)}`;
        },
      },
    }],
  } as EChartsOption;
});
</script>

<template>
  <div class="rankchart" :style="{height: `${height}px`}">
    <VChart class="rankchart__canvas" :option="option" :init-options="{renderer: 'svg'}" :update-options="{notMerge: true}" autoresize />
  </div>
</template>

<style scoped>
.rankchart {
  position: relative;
  width: 100%;
  min-width: 0;
}

.rankchart__canvas {
  width: 100%;
  height: 100%;
}
</style>
