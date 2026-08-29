import assert from 'node:assert/strict';
import test from 'node:test';
import {browserVersionFromLockedMetadata} from '../src/engines.ts';

const puppeteerRevisions = {
  chrome: '152.0.7977.42',
  'chrome-headless-shell': '152.0.7977.42',
};

const playwrightBrowsers = [
  {name: 'chromium', revision: '1234', browserVersion: '151.0.7922.34'},
  {name: 'chromium-headless-shell', revision: '1234', browserVersion: '151.0.7922.34'},
];

test('matches Puppeteer locked revisions against the executable cache path', () => {
  assert.equal(browserVersionFromLockedMetadata(
      'puppeteer-chrome',
      '/cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome',
      {puppeteerRevisions}), '152.0.7977.42');
  assert.equal(browserVersionFromLockedMetadata(
      'puppeteer-shell',
      'C:\\cache\\puppeteer\\chrome-headless-shell\\win64-152.0.7977.42\\headless_shell.exe',
      {puppeteerRevisions}), '152.0.7977.42');
});

test('matches Playwright revision directories to browsers.json browser versions', () => {
  assert.equal(browserVersionFromLockedMetadata(
      'playwright-chrome',
      'C:\\cache\\ms-playwright\\chromium-1234\\chrome-win64\\chrome.exe',
      {playwrightBrowsers}), '151.0.7922.34');
  assert.equal(browserVersionFromLockedMetadata(
      'playwright-shell',
      '/cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/headless_shell',
      {playwrightBrowsers}), '151.0.7922.34');
});

test('does not invent a version for a missing or mismatched package browser', () => {
  assert.equal(browserVersionFromLockedMetadata(
      'playwright-shell', '/custom/headless_shell', {playwrightBrowsers}), null);
  assert.equal(browserVersionFromLockedMetadata(
      'playwright-shell', '/cache/ms-playwright/chromium-1234/chrome-linux/chrome',
      {playwrightBrowsers}), null);
  assert.equal(browserVersionFromLockedMetadata(
      'puppeteer-chrome', '/cache/puppeteer/chrome/linux-older/chrome',
      {puppeteerRevisions}), null);
});
