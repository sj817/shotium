import {computed, type ComputedRef} from 'vue';
import {useData} from 'vitepress';
import {
  hasMessage,
  resolveLocale,
  segments,
  translate,
  type Locale,
  type MessageKey,
  type Params,
  type Segment,
} from './messages';
import type {ExclusionCode} from './ranking';
import type {ResultStatus} from './types';

export type {Locale, MessageKey, Params, Segment};

export interface I18n {
  locale: ComputedRef<Locale>;
  t: (key: MessageKey, params?: Params) => string;
  seg: (key: MessageKey, params: Params) => Segment[];
  engineName: (engine: string) => string;
  engineHelp: (engine: string) => string;
  platformName: (platform: string) => string;
  scenarioName: (scenario: string, concurrency?: number) => string;
  scenarioHelp: (scenario: string) => string | null;
  shardName: (shard: string) => string;
  statusLabel: (status: ResultStatus | string | null | undefined) => string;
  statusHelp: (status: ResultStatus | string | null | undefined) => string;
  exclusionLabel: (code: ExclusionCode, count: number) => string;
}

const STATUS_KEY: Record<string, string> = {
  'pass': 'pass',
  'fail': 'fail',
  'noisy': 'noisy',
  'n/a': 'na',
  'infra-error': 'infra',
  'missing': 'missing',
};

export function createI18n(locale: ComputedRef<Locale>): I18n {
  const t: I18n['t'] = (key, params) => translate(locale.value, key, params);
  const fallback = (key: string, raw: string): string => hasMessage(key) ? t(key) : raw;
  return {
    locale,
    t,
    seg: (key, params) => segments(locale.value, key, params),
    engineName: (engine) => fallback(`engine.${engine}`, engine),
    engineHelp: (engine) => {
      if (engine === 'shotium') return t('engineHelp.shotium');
      if (engine.endsWith('-shell')) return t('engineHelp.shell');
      if (engine.endsWith('-chrome')) return t('engineHelp.chrome');
      return engine;
    },
    platformName: (platform) => fallback(`platform.${platform}`, platform),
    scenarioName: (scenario, concurrency) => {
      const name = fallback(`scenario.${scenario}`, scenario);
      return concurrency && concurrency > 1 ? `${name} ×${concurrency}` : name;
    },
    scenarioHelp: (scenario) => {
      const key = `scenarioHelp.${scenario}`;
      return hasMessage(key) ? t(key) : null;
    },
    shardName: (shard) => fallback(`shard.${shard}`, shard),
    statusLabel: (status) => {
      const key = `status.${STATUS_KEY[status ?? ''] ?? ''}`;
      return hasMessage(key) ? t(key) : String(status ?? t('runStatus.unknown'));
    },
    statusHelp: (status) => {
      const key = `statusHelp.${STATUS_KEY[status ?? ''] ?? ''}`;
      return hasMessage(key) ? t(key) : t('runHelp.unknown');
    },
    exclusionLabel: (code, count) => t(`exclusion.${code}`, {count}),
  };
}

/** Locale comes from VitePress; the language switch in the nav bar is the only way to change it. */
export function useI18n(): I18n {
  const {lang} = useData();
  const locale = computed(() => resolveLocale(lang.value));
  return createI18n(locale);
}
