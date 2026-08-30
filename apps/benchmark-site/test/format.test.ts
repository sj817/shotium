import {describe, expect, it} from 'vitest';
import {firstLine, formatMs, formatMsTick, formatRatio, formatUtc, shortSha} from '../docs/lib/format';

describe('formatting', () => {
  it('formats ratios with one decimal under ten and none above', () => {
    expect(formatRatio(1)).toBe('1×');
    expect(formatRatio(4.83)).toBe('4.8×');
    expect(formatRatio(18.4)).toBe('18×');
    expect(formatRatio(null)).toBe('—');
  });

  it('formats milliseconds and seconds', () => {
    expect(formatMs(53.4)).toBe('53 ms');
    expect(formatMs(9.96)).toBe('10 ms');
    expect(formatMs(1061)).toBe('1.06 s');
    expect(formatMs(10491)).toBe('10.5 s');
    expect(formatMs(46803)).toBe('46.8 s');
    expect(formatMsTick(500)).toBe('500 ms');
    expect(formatMsTick(20000)).toBe('20 s');
  });

  it('formats UTC timestamps without the viewer time zone', () => {
    expect(formatUtc('2026-08-30T00:05:25.000Z')).toBe('2026-08-30 00:05 UTC');
    expect(formatUtc('2026-08-30T00:05:25.000Z', false)).toBe('2026-08-30');
    expect(formatUtc(null)).toBe('—');
  });

  it('shortens hashes and error text', () => {
    expect(shortSha('bac114e9f8c57fb04875ee8ceec7e8e7c11905d4')).toBe('bac114e9f8');
    expect(shortSha('01059e287ce7')).toBe('01059e287c');
    expect(firstLine('Error: first\n    at second')).toBe('Error: first');
    expect(firstLine('x'.repeat(200), 20)).toHaveLength(20);
  });
});
