<script setup lang="ts">
import {computed, onMounted, ref, watch} from 'vue';
import {useData} from 'vitepress';
import zhCn from 'element-plus/es/locale/lang/zh-cn';
import en from 'element-plus/es/locale/lang/en';
import {loadIndex, loadRun} from '../lib/data';
import {useI18n} from '../lib/i18n';
import {buildRunReport} from '../lib/report';
import {PLATFORM_IDS, type BenchmarkIndex, type LoadedRun, type PlatformId} from '../lib/types';
import RunBar from './RunBar.vue';
import VerdictPanel from './VerdictPanel.vue';
import PlatformOverview from './PlatformOverview.vue';
import PlatformPanel from './PlatformPanel.vue';
import Methodology from './Methodology.vue';

/*
 * Root of the report. Loads benchmark-results/index.json, then the selected
 * run (manifest + six platform summaries + failures), derives the run report
 * and keeps the selected run and platform in the URL (?run=…&platform=…) so
 * a view can be shared. Element Plus takes its locale from VitePress's.
 */
const {site} = useData();
const {locale} = useI18n();
const base = computed(() => site.value.base);
const epLocale = computed(() => (locale.value === 'en' ? en : zhCn));

const index = ref<BenchmarkIndex | null>(null);
const selectedPath = ref<string | null>(null);
const run = ref<LoadedRun | null>(null);
const phase = ref<'index' | 'run' | 'ready' | 'error' | 'empty'>('index');
const error = ref('');
const platformId = ref<PlatformId>('linux-x64');

const report = computed(() => (run.value ? buildRunReport(run.value) : null));

function isPlatformId(value: string | null): value is PlatformId {
  return value !== null && (PLATFORM_IDS as readonly string[]).includes(value);
}

function readUrl(): {run: string | null; platform: string | null} {
  const params = new URLSearchParams(window.location.search);
  return {run: params.get('run'), platform: params.get('platform')};
}

function writeUrl() {
  const params = new URLSearchParams(window.location.search);
  if (selectedPath.value && index.value?.results[0]?.path !== selectedPath.value) params.set('run', selectedPath.value);
  else params.delete('run');
  params.set('platform', platformId.value);
  const query = params.toString();
  const next = `${window.location.pathname}${query ? `?${query}` : ''}${window.location.hash}`;
  window.history.replaceState(window.history.state, '', next);
}

async function loadAll() {
  phase.value = 'index';
  error.value = '';
  try {
    const loaded = await loadIndex(base.value);
    index.value = loaded;
    if (!loaded.results.length) {
      phase.value = 'empty';
      return;
    }
    const wanted = readUrl();
    if (isPlatformId(wanted.platform)) platformId.value = wanted.platform;
    const entry = loaded.results.find((candidate) => candidate.path === wanted.run) ?? loaded.results[0];
    selectedPath.value = entry?.path ?? null;
  } catch (cause) {
    error.value = cause instanceof Error ? cause.message : String(cause);
    phase.value = 'error';
  }
}

watch(selectedPath, async (path) => {
  if (!path || !index.value) return;
  const entry = index.value.results.find((candidate) => candidate.path === path);
  if (!entry) return;
  phase.value = 'run';
  error.value = '';
  try {
    const loaded = await loadRun(base.value, entry);
    if (selectedPath.value !== path) return;
    run.value = loaded;
    phase.value = 'ready';
    if (!readUrl().platform) {
      const built = buildRunReport(loaded);
      const preferred = built.platforms.find((platform) => platform.ranked) ??
        built.platforms.find((platform) => platform.data.summary !== null);
      if (preferred) platformId.value = preferred.id;
    }
    writeUrl();
  } catch (cause) {
    if (selectedPath.value !== path) return;
    error.value = cause instanceof Error ? cause.message : String(cause);
    phase.value = 'error';
  }
});

function selectRun(path: string) {
  if (path !== selectedPath.value) selectedPath.value = path;
}

function selectPlatform(id: PlatformId) {
  platformId.value = id;
  writeUrl();
}

onMounted(loadAll);
</script>

<template>
  <el-config-provider :locale="epLocale">
    <div class="bench">
      <RunBar :index="index" :run="run" :selected="selectedPath" @select="selectRun" />
      <VerdictPanel :report="report" :phase="phase" :error="error" @retry="loadAll" @open="selectPlatform" />
      <PlatformOverview
        :report="report"
        :model-value="platformId"
        @update:model-value="selectPlatform"
      />
      <PlatformPanel
        v-if="report"
        :report="report"
      />
      <Methodology />
    </div>
  </el-config-provider>
</template>
