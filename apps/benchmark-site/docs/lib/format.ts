import type {Locale} from './messages';

const RATIO_TIMES = '×';

function numberFormat(locale: Locale, options: Intl.NumberFormatOptions): Intl.NumberFormat {
  return new Intl.NumberFormat(locale === 'zh-CN' ? 'zh-CN' : 'en-US', options);
}

/** `4.8×`, `18×`, `1×`; ratios below 10 keep one decimal, larger ones none. */
export function formatRatio(value: number | null | undefined, locale: Locale = 'en'): string {
  if (value === null || value === undefined || !Number.isFinite(value)) return '—';
  const digits = value < 10 ? 1 : 0;
  return `${numberFormat(locale, {maximumFractionDigits: digits, minimumFractionDigits: 0}).format(value)}${RATIO_TIMES}`;
}

/** Milliseconds as `53 ms`, `1.06 s`, `10.5 s`, `46.8 s`. */
export function formatMs(value: number | null | undefined, locale: Locale = 'en'): string {
  if (value === null || value === undefined || !Number.isFinite(value)) return '—';
  if (value < 1000) {
    const digits = value < 10 ? 1 : 0;
    return `${numberFormat(locale, {maximumFractionDigits: digits}).format(value)} ms`;
  }
  const seconds = value / 1000;
  const digits = seconds < 10 ? 2 : 1;
  return `${numberFormat(locale, {maximumFractionDigits: digits}).format(seconds)} s`;
}

/** Axis ticks: `50 ms`, `1 s`, `10 s`. */
export function formatMsTick(value: number, locale: Locale = 'en'): string {
  if (value < 1000) return `${numberFormat(locale, {maximumFractionDigits: 0}).format(value)} ms`;
  return `${numberFormat(locale, {maximumFractionDigits: 1}).format(value / 1000)} s`;
}

export function formatInt(value: number | null | undefined, locale: Locale = 'en'): string {
  if (value === null || value === undefined || !Number.isFinite(value)) return '—';
  return numberFormat(locale, {maximumFractionDigits: 0}).format(value);
}

export function formatThroughput(value: number | null | undefined, locale: Locale = 'en'): string {
  if (value === null || value === undefined || !Number.isFinite(value)) return '—';
  const digits = value < 10 ? 1 : 0;
  return `${numberFormat(locale, {maximumFractionDigits: digits}).format(value)}/s`;
}

/** ISO timestamp → `2026-08-30 00:05 UTC`; falls back to the raw string. */
export function formatUtc(iso: string | null | undefined, withTime = true): string {
  if (!iso) return '—';
  const date = new Date(iso);
  if (Number.isNaN(date.getTime())) return iso;
  const pad = (n: number) => String(n).padStart(2, '0');
  const day = `${date.getUTCFullYear()}-${pad(date.getUTCMonth() + 1)}-${pad(date.getUTCDate())}`;
  if (!withTime) return day;
  return `${day} ${pad(date.getUTCHours())}:${pad(date.getUTCMinutes())} UTC`;
}

export function shortSha(sha: string | null | undefined, length = 10): string {
  if (!sha) return '—';
  return sha.length > length ? sha.slice(0, length) : sha;
}

/** First line of a multi-line error, trimmed for list display. */
export function firstLine(text: string, max = 160): string {
  const line = text.split(/\r?\n/, 1)[0] ?? '';
  return line.length > max ? `${line.slice(0, max - 1)}…` : line;
}
