<script setup lang="ts">
import {computed} from 'vue';
import {useI18n} from '../lib/i18n';
import {hasMessage} from '../lib/messages';
import {statusTone, type StatusTone} from '../lib/labels';

/*
 * A status word as an Element Plus tag, with a plain-language explanation in
 * a tooltip on hover and on keyboard focus. `result` chips describe engines,
 * scenarios and platforms; `archive` / `quality` / `evidence` chips describe a
 * whole run.
 */
const props = withDefaults(defineProps<{
  status: string | null | undefined;
  kind?: 'result' | 'archive' | 'quality' | 'evidence';
}>(), {kind: 'result'});

const {t, statusLabel, statusHelp} = useI18n();

const TYPE: Record<StatusTone, 'success' | 'warning' | 'danger' | 'info' | 'primary'> = {
  pass: 'success',
  noisy: 'warning',
  fail: 'danger',
  na: 'info',
  infra: 'primary',
  neutral: 'info',
};

const tone = computed(() => statusTone(props.status));

const style = computed(() => (tone.value === 'neutral'
  ? {'--el-tag-bg-color': 'var(--control-fill)', '--el-tag-text-color': 'var(--ink-2)', '--el-tag-border-color': 'var(--line-1)'}
  : {
    '--el-tag-bg-color': `var(--status-${tone.value}-soft)`,
    '--el-tag-text-color': `var(--status-${tone.value})`,
    '--el-tag-border-color': `color-mix(in srgb, var(--status-${tone.value}) 30%, transparent)`,
  }));

const label = computed(() => {
  if (props.kind === 'result') return statusLabel(props.status);
  const key = `runStatus.${props.status ?? ''}`;
  return hasMessage(key) ? t(key) : t('runStatus.unknown');
});

const help = computed(() => {
  if (props.kind === 'result') return statusHelp(props.status);
  const key = `runHelp.${props.kind}.${props.status ?? ''}`;
  return hasMessage(key) ? t(key) : t('runHelp.unknown');
});
</script>

<template>
  <el-tooltip :content="help" placement="top" popper-class="bench-pop" :trigger="['hover', 'focus']" :show-after="100">
    <el-tag class="chip" :type="TYPE[tone]" effect="light" round size="small" :style="style" tabindex="0">{{ label }}</el-tag>
  </el-tooltip>
</template>

<style scoped>
.chip {
  font-weight: 600;
  cursor: default;
}
</style>
