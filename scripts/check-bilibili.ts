// Offline regressions for the two original Bilibili article reports.
//
//   pnpm verify:bilibili --fixtures-only        # the fixtures reference nothing on the network
//   pnpm verify:bilibili --package shotium      # and the engine renders them whole
//
// No browser, server or network is needed. The engine half loads the package
// in this process and exercises screenshot() and screenshotTiles() on two
// articles taller than Blink paints from one scroll position, then compares
// every tile with the full page and every article photo with its source file.
//
// Relative paths are resolved against the repository root.

import {existsSync, mkdirSync, mkdtempSync, readFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import sharp from 'sharp';

import {capture, FIXTURES, readManifest, type CapturedPage} from './bilibili-capture.ts';
import {crop, decodePng, type Image} from './lib/png.ts';
import {resolve} from './lib/repo.ts';
import {Checks, sha256} from './lib/report.ts';

const ARTICLES = new Set(['1788403871415', '1788434008828']);

// The src, href and poster attributes of every tag, and every <img> tag's
// attributes, the way Python's HTMLParser reported them (entities decoded).
function resources(html: string): {urls: string[]; images: Array<Record<string, string>>} {
  const unescape = (s: string) => s.replace(/&amp;/g, '&').replace(/&lt;/g, '<').replace(/&gt;/g, '>').replace(/&quot;/g, '"').replace(/&#39;/g, "'");
  const urls: string[] = [];
  const images: Array<Record<string, string>> = [];
  for (const [, tag, attrText] of html.matchAll(/<([a-zA-Z][\w-]*)\b([^>]*)>/g)) {
    const attrs: Record<string, string> = {};
    for (const m of attrText.matchAll(/([\w:-]+)\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s"'>]+))/g)) {
      attrs[m[1].toLowerCase()] = unescape(m[2] ?? m[3] ?? m[4] ?? '');
    }
    for (const key of ['src', 'href', 'poster']) {
      if (attrs[key]) urls.push(attrs[key]);
    }
    if (tag.toLowerCase() === 'img') images.push(attrs);
  }
  return {urls, images};
}

const hasScheme = (url: string) => /^[a-zA-Z][a-zA-Z0-9+.-]*:/.test(url);
const hasHost = (url: string) => /^(?:[a-zA-Z][a-zA-Z0-9+.-]*:)?\/\//.test(url);

function fixtureChecks(checks: Checks): void {
  const manifest = readManifest();
  checks.section('every resource is local and accounted for');
  const sources = new Set(Object.keys(manifest.sources));
  checks.check(sources.size === ARTICLES.size && [...ARTICLES].every((a) => sources.has(a)), 'the manifest lists both articles', [...sources].join(', '));
  const assets = new Set(manifest.assets.map((a) => a.path));
  const referenced = new Set<string>();
  let bad = 0;
  for (const article of sources) {
    const text = readFileSync(path.join(FIXTURES, `${article}.html`), 'utf8');
    const parsed = resources(text);
    const urls = [...parsed.urls, ...[...text.matchAll(/url\(["']?([^)"']+)/g)].map((m) => m[1])];
    for (const url of urls) {
      if (url.startsWith('#')) continue;
      const ok = !hasScheme(url) && !hasHost(url) && assets.has(url) && existsSync(path.join(FIXTURES, decodeURIComponent(url)));
      if (!ok) {
        bad++;
        checks.check(false, `${article}: ${url.slice(0, 100)}`, hasScheme(url) || hasHost(url) ? 'external or embedded resource' : !assets.has(url) ? 'not in the manifest' : 'file missing');
      }
      referenced.add(url);
    }
  }
  checks.check(bad === 0, 'every referenced resource is a local fixture in the manifest', `${referenced.size} references`);
  const unreferenced = [...assets].filter((a) => !referenced.has(a));
  checks.check(unreferenced.length === 0 && referenced.size === assets.size, 'and every fixture asset is referenced', unreferenced.join(', '));

  checks.section('the original resource bytes are pinned');
  let mismatched = 0;
  for (const asset of manifest.assets) {
    const data = readFileSync(path.join(FIXTURES, asset.path));
    const ok = data.length === asset.bytes && sha256(data) === asset.sha256 && (!asset.path.endsWith('.woff2') || data.subarray(0, 4).toString() === 'wOF2');
    if (!ok) {
      mismatched++;
      checks.check(false, asset.path, `${data.length} bytes, ${sha256(data).slice(0, 16)}`);
    }
  }
  checks.check(mismatched === 0, `all ${manifest.assets.length} assets match their pinned length and sha256`);
}

// Per-channel mean absolute difference, and the share of channel values that
// differ by 5 or more, over two RGBA images of one size (alpha ignored).
function difference(a: Image, b: Image): {maxMean: number; changedShare: number; maxDelta: number} {
  const sums = [0, 0, 0];
  let changed = 0, maxDelta = 0;
  const pixels = a.width * a.height;
  for (let i = 0; i < pixels; i++) {
    for (let c = 0; c < 3; c++) {
      const d = Math.abs(a.data[i * 4 + c] - b.data[i * 4 + c]);
      sums[c] += d;
      if (d >= 5) changed++;
      if (d > maxDelta) maxDelta = d;
    }
  }
  return {maxMean: Math.max(...sums) / pixels, changedShare: changed / (pixels * 3), maxDelta};
}

// Area-averaging downscale (Pillow's BOX filter) to w x h.
function boxResize(image: Image, w: number, h: number): Image {
  const data = Buffer.alloc(w * h * 4);
  for (let oy = 0; oy < h; oy++) {
    const y0 = (oy * image.height) / h, y1 = ((oy + 1) * image.height) / h;
    for (let ox = 0; ox < w; ox++) {
      const x0 = (ox * image.width) / w, x1 = ((ox + 1) * image.width) / w;
      const acc = [0, 0, 0, 0];
      let weight = 0;
      for (let y = Math.floor(y0); y < Math.ceil(y1); y++) {
        const wy = Math.min(y + 1, y1) - Math.max(y, y0);
        for (let x = Math.floor(x0); x < Math.ceil(x1); x++) {
          const wx = Math.min(x + 1, x1) - Math.max(x, x0);
          const i = (y * image.width + x) * 4;
          for (let c = 0; c < 4; c++) acc[c] += image.data[i + c] * wx * wy;
          weight += wx * wy;
        }
      }
      const o = (oy * w + ox) * 4;
      for (let c = 0; c < 4; c++) data[o + c] = Math.round(acc[c] / weight);
    }
  }
  return {width: w, height: h, data};
}

async function renderChecks(checks: Checks, packageDir: string): Promise<void> {
  const manifest = readManifest();
  const outputRoot = resolve('shot/testdata/out');
  mkdirSync(outputRoot, {recursive: true});
  const output = mkdtempSync(path.join(outputRoot, 'bilibili-'));
  const captures = await capture(packageDir, output);
  const ids = new Set(captures.pages.map((p) => p.id));
  if (ids.size !== ARTICLES.size || ![...ARTICLES].every((a) => ids.has(a))) throw new Error('did not capture both original articles');
  console.log(`Bilibili captures: ${output}`);
  console.log(`Engine SHA256: ${captures.librarySha256}`);

  checks.section('full page and tiles have no gaps or missing pixels');
  for (const page of captures.pages) {
    checks.check(page.stats.failed === 0 && page.tileStats.failed === 0, `${page.id}: no failed subresources`, `${page.stats.failed} / ${page.tileStats.failed}`);
    const full = decodePng(readFileSync(page.fullPath));
    checks.check(full.width === 1440, `${page.id}: the full page is 1440 wide`, `${full.width}`);
    // Both articles exceed the old 32767 CSS-pixel boundary.
    checks.check(full.height > 32767, `${page.id}: and taller than one paint`, `${full.height}`);
    checks.check(page.tiles.length > 1, `${page.id}: more than one tile`, `${page.tiles.length}`);
    let y = 0, geometry = true, pixels = true, worst = '';
    for (const tile of page.tiles) {
      if (tile.x !== 0 || tile.y !== y || tile.width !== 1440 || tile.height <= 0 || tile.height > 8000) geometry = false;
      const image = decodePng(readFileSync(tile.path));
      if (image.width !== 1440 || image.height !== tile.height) geometry = false;
      // Raster strips may move a few antialiased rounded-corner pixels. Bound
      // both aggregate error and its coverage; even one missing row across an
      // 8000px tile exceeds the latter bound.
      const d = difference(image, crop(full, 0, y, 1440, tile.height));
      if (!(d.maxMean < 0.01 && d.changedShare < 0.0001)) {
        pixels = false;
        worst = `tile at ${y}: mean ${d.maxMean.toFixed(4)}, changed ${(d.changedShare * 100).toFixed(4)}%`;
      }
      y += tile.height;
    }
    checks.check(geometry, `${page.id}: tiles stack from 0 to the bottom in 1440-wide slices of at most 8000px`);
    checks.check(y === full.height, `${page.id}: and their heights add up to the full page`, `${y} vs ${full.height}`);
    checks.check(pixels, `${page.id}: every tile matches the full page pixel for pixel`, worst);
  }

  checks.section('article images match their source and the full page');
  for (const page of captures.pages as CapturedPage[]) {
    const parsed = resources(readFileSync(path.join(FIXTURES, `${page.id}.html`), 'utf8'));
    const urls = new Set(parsed.urls);
    const photos = new Set(manifest.assets.filter((a) => a.source.includes('/new_dyn/')).map((a) => a.path).filter((p) => urls.has(p)));
    const expected = new Set([...photos, ...parsed.images.filter((i) => i.alt === '二维码').map((i) => i.src)]);
    checks.check(photos.size > 0, `${page.id}: article photos found`, `${photos.size}`);
    const probed = new Set(page.probes.map((p) => p.source));
    checks.check(probed.size === expected.size && [...expected].every((e) => probed.has(e)) && page.probes.length === expected.size,
                 `${page.id}: one probe per photo and the QR code`, `${page.probes.length} probes`);
    const full = decodePng(readFileSync(page.fullPath));
    checks.check(page.probes[page.probes.length - 1].y > 32767, `${page.id}: the last probe lies beyond the first paint window`, `${page.probes[page.probes.length - 1].y}`);
    for (const probe of page.probes) {
      const label = `${page.id}: ${probe.source}`;
      if (!checks.check(probe.stats.failed === 0, `${label} rendered without failed subresources`, `${probe.stats.failed}`)) continue;
      const selected = decodePng(readFileSync(probe.path!));
      const {x, y, width: w, height: h} = probe;
      // Stay clear of rounded corners and shadows. This independent source
      // check also rejects matching blank output from the full and tiled
      // paths.
      const box = [Math.floor(w / 4), Math.floor(h / 4), Math.floor((w * 3) / 4), Math.floor((h * 3) / 4)] as const;
      const bw = box[2] - box[0], bh = box[3] - box[1];
      const center = crop(selected, box[0], box[1], bw, bh);
      const bg = selected.data.subarray((Math.floor(w / 2)) * 4, (Math.floor(w / 2)) * 4 + 3);
      const source = await sharp(path.join(FIXTURES, decodeURIComponent(probe.source))).ensureAlpha().resize(w, h, {kernel: 'linear', fit: 'fill'}).raw().toBuffer();
      const reference = await sharp({create: {width: w, height: h, channels: 4, background: {r: bg[0], g: bg[1], b: bg[2], alpha: 1}}})
                            .composite([{input: source, raw: {width: w, height: h, channels: 4}}])
                            .raw().toBuffer();
      const referenceCenter = crop({width: w, height: h, data: reference}, box[0], box[1], bw, bh);
      // Different resamplers/subpixel positions can vary at edges. Compare
      // low-frequency photo content.
      const d = difference(boxResize(center, 16, 16), boxResize(referenceCenter, 16, 16));
      checks.check(d.maxMean < 12, `${label} looks like its source file`, `mean delta ${d.maxMean.toFixed(2)}`);
      const fromFull = crop(full, x + box[0], y + box[1], bw, bh);
      const e = difference(center, fromFull);
      checks.check(e.maxDelta <= 4, `${label} matches the same region of the full page`, `max delta ${e.maxDelta}`);
    }
  }
}

async function main(opts: {fixturesOnly: boolean; package: string}): Promise<number> {
  const checks = new Checks();
  fixtureChecks(checks);
  if (!opts.fixturesOnly) await renderChecks(checks, resolve(opts.package));
  return checks.finish();
}

const cli = cac('check-bilibili');
cli.command('', 'offline regressions for the two Bilibili article fixtures')
    .option('--fixtures-only', 'only check that the fixtures are complete and offline')
    .option('--package <dir>', 'the package directory to load', {default: 'shotium'})
    .action(async (options: {fixturesOnly?: boolean; package: string}) => {
      try {
        process.exitCode = await main({fixturesOnly: options.fixturesOnly === true, package: options.package});
      } catch (error) {
        console.error(error);
        process.exitCode = 1;
      }
      process.exit(process.exitCode);
    });
cli.help();
cli.parse();
