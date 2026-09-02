import fs from 'node:fs';
import path from 'node:path';
import {ShotiumEngine, competitorChromiumPolicy, waitForVisualReady} from './engines.ts';
import {BROWSER_OPERATION_TIMEOUT_MS, VIEWPORT} from './constants.ts';

function writeEvidence(file, image) {
  if (!file) return;
  fs.mkdirSync(path.dirname(file), {recursive: true});
  fs.writeFileSync(file, image);
}

async function warmPuppeteer(name, url, evidenceFile) {
  const puppeteerModule = await import('puppeteer');
  const puppeteer = puppeteerModule.default || puppeteerModule;
  const browser = await puppeteer.launch({
    headless: name === 'puppeteer-shell' ? 'shell' : true,
    args: competitorChromiumPolicy().puppeteerArgs,
    defaultViewport: {...VIEWPORT, deviceScaleFactor: 1},
    protocolTimeout: BROWSER_OPERATION_TIMEOUT_MS,
  });
  const page = await browser.newPage();
  await page.goto(url, {waitUntil: 'load', timeout: BROWSER_OPERATION_TIMEOUT_MS});
  await waitForVisualReady(page);
  writeEvidence(evidenceFile, await page.screenshot({type: 'png'}));
  await page.close();
  return {
    endpoint: {wsEndpoint: browser.wsEndpoint()},
    rootPids: browser.process()?.pid ? [browser.process().pid] : [],
    close: () => browser.close(),
  };
}

async function warmPlaywright(name, url, evidenceFile) {
  const {chromium} = await import('playwright');
  const channel = name === 'playwright-shell' ? 'chromium-headless-shell' : 'chromium';
  const policy = competitorChromiumPolicy();
  const server = await chromium.launchServer({
    headless: true,
    channel,
    args: policy.playwrightArgs,
    chromiumSandbox: policy.playwrightChromiumSandbox,
  });
  const browser = await chromium.connect(server.wsEndpoint());
  const context = await browser.newContext({viewport: VIEWPORT, deviceScaleFactor: 1});
  const page = await context.newPage();
  await page.goto(url, {waitUntil: 'load', timeout: BROWSER_OPERATION_TIMEOUT_MS});
  await waitForVisualReady(page);
  writeEvidence(evidenceFile, await page.screenshot({
    type: 'png',
    timeout: BROWSER_OPERATION_TIMEOUT_MS,
  }));
  await context.close();
  return {
    endpoint: {wsEndpoint: server.wsEndpoint()},
    rootPids: server.process()?.pid ? [server.process().pid] : [],
    close: async () => {
      await browser.close().catch(() => {});
      await server.close().catch(() => {});
    },
  };
}

export async function startResident(name, url, {
  workers = 4,
  daemonName = 'benchmark',
  evidenceFile = null,
} = {}) {
  if (name === 'shotium') {
    const engine = new ShotiumEngine({mode: 'daemon', workers, daemonName});
    await engine.launch();
    const warm = await engine.shot(url);
    writeEvidence(evidenceFile, warm.image);
    const status = await engine.status();
    return {
      endpoint: {daemonName},
      rootPids: status?.pid ? [status.pid] : [],
      close: () => engine.close({stopDaemon: true}),
    };
  }
  if (name.startsWith('puppeteer-')) return warmPuppeteer(name, url, evidenceFile);
  if (name.startsWith('playwright-')) return warmPlaywright(name, url, evidenceFile);
  throw new Error(`resident mode is unavailable for ${name}`);
}
