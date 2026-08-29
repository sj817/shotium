import assert from 'node:assert/strict';
import test from 'node:test';
import {
  browserVersionFromLockedMetadata,
  isPlaywrightHeadlessShellExecutable,
  playwrightCacheRootFromChromiumExecutable,
} from '../src/engines.ts';

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

test('recognizes current CFT and legacy Playwright shell executable names', () => {
  assert.equal(isPlaywrightHeadlessShellExecutable(
      '/cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell'), true);
  assert.equal(isPlaywrightHeadlessShellExecutable(
      'C:\\cache\\ms-playwright\\chromium_headless_shell-1234\\chrome-headless-shell-win64\\chrome-headless-shell.exe'), true);
  assert.equal(isPlaywrightHeadlessShellExecutable(
      '/cache/ms-playwright/chromium_headless_shell-1234/headless-shell/headless_shell'), true);
  assert.equal(isPlaywrightHeadlessShellExecutable(
      'C:\\cache\\ms-playwright\\chromium_headless_shell-1234\\headless_shell.exe'), true);
});

test('does not pick similarly named executables outside the Playwright shell cache', () => {
  assert.equal(isPlaywrightHeadlessShellExecutable(
      '/cache/puppeteer/chrome-headless-shell/linux-152/chrome-headless-shell'), false);
  assert.equal(isPlaywrightHeadlessShellExecutable(
      '/cache/ms-playwright/chromium-1234/chrome-linux/chrome-headless-shell'), false);
  assert.equal(isPlaywrightHeadlessShellExecutable(
      '/cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/chrome'), false);
});

test('derives the bounded Playwright cache root on Windows, Linux and macOS', () => {
  assert.equal(playwrightCacheRootFromChromiumExecutable(
      'C:\\Users\\runner\\AppData\\Local\\ms-playwright\\chromium-1234\\chrome-win64\\chrome.exe'),
  'C:\\Users\\runner\\AppData\\Local\\ms-playwright');
  assert.equal(playwrightCacheRootFromChromiumExecutable(
      '/home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome'),
  '/home/runner/.cache/ms-playwright');
  assert.equal(playwrightCacheRootFromChromiumExecutable(
      '/Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac/Chromium.app/Contents/MacOS/Chromium'),
  '/Users/runner/Library/Caches/ms-playwright');
});

test('refuses to search above an executable without a valid Chromium revision ancestor', () => {
  assert.throws(() => playwrightCacheRootFromChromiumExecutable(
      '/Users/runner/Library/Caches/ms-playwright/chromium-latest/Chromium.app/Contents/MacOS/Chromium'),
  /not inside a chromium-REV cache directory/);
  assert.throws(() => playwrightCacheRootFromChromiumExecutable('relative/chrome'),
      /path is not absolute/);
});
