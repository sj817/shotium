import {describe, expect, it} from 'vitest';
import {hasMessage, resolveLocale, segments, translate} from '../docs/lib/messages';

describe('messages', () => {
  it('resolves the VitePress lang to a dictionary', () => {
    expect(resolveLocale('zh-CN')).toBe('zh-CN');
    expect(resolveLocale('en')).toBe('en');
    expect(resolveLocale('en-US')).toBe('en');
    expect(resolveLocale(undefined)).toBe('zh-CN');
  });

  it('substitutes parameters and keeps unknown placeholders visible', () => {
    expect(translate('zh-CN', 'overview.comparable', {n: 10})).toBe('10 项可比');
    expect(translate('en', 'ranking.basedOn', {n: 1})).toBe('Based on 1 comparable items');
    expect(translate('en', 'verdict.closest', {platform: 'P'})).toContain('{engine}');
  });

  it('splits a template into text and parameter segments', () => {
    const parts = segments('zh-CN', 'verdict.someFirst', {ranked: 4, first: 3});
    expect(parts.map((part) => part.text).join('')).toBe('Shotium 在 4 个产生排名的平台中的 3 个第一。');
    expect(parts.filter((part) => part.param).map((part) => part.param)).toEqual(['ranked', 'first']);
  });

  it('knows every status word and reason in both languages', () => {
    for (const key of ['status.pass', 'status.noisy', 'status.na', 'status.infra', 'reason.partial-coverage', 'reasonHelp.no-competitor']) {
      expect(hasMessage(key)).toBe(true);
      expect(translate('en', key as never)).not.toBe(translate('zh-CN', key as never));
    }
    expect(hasMessage('status.nope')).toBe(false);
  });
});
