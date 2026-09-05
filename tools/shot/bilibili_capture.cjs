'use strict';

// The real npm-facing API, driven by bilibili_check.py. Keep binary payloads
// in files so a failed check leaves inspectable evidence, not a giant JSON log.
const fs = require('node:fs');
const path = require('node:path');
const {createHash} = require('node:crypto');
const {Runtime} = require(path.resolve(process.argv[2]));
const output = path.resolve(process.argv[3]);
const root = path.resolve(__dirname, '../../shot/testdata/bilibili');
const manifest = JSON.parse(fs.readFileSync(path.join(root, 'manifest.json')));

async function main() {
  const runtime = new Runtime();
  const started = await runtime.start({cacheDir: null, allowFileAccess: true});
  const results = {enginePath: started.enginePath, pages: []};
  try {
    for (const id of Object.keys(manifest.sources)) {
      const file = path.join(root, `${id}.html`);
      const common = {file, viewport: {width: 1440, height: 900}, allowFileAccess: true};
      const fullPath = path.join(output, `${id}.png`);
      const full = await runtime.screenshot({...common, fullPage: true, path: fullPath});
      const tiled = await runtime.screenshotTiles({...common, fullPage: true, tile: {height: 8000}});
      const tiles = tiled.tiles.map(({image, ...region}, i) => {
        if (!Buffer.isBuffer(image)) throw new Error('screenshotTiles did not return a Buffer');
        const tilePath = path.join(output, `${id}-tile-${i}.png`);
        fs.writeFileSync(tilePath, image);
        return {...region, path: tilePath};
      });
      const html = fs.readFileSync(file, 'utf8');
      const images = [...html.matchAll(/<img\b[^>]*>/g)]
          .map(([tag]) => /\bsrc="([^"]+)"/.exec(tag)?.[1])
          .filter((src) => src && manifest.assets.some((a) => a.path === src && a.source.includes('/new_dyn/')));
      const qr = [...html.matchAll(/<img\b[^>]*>/g)]
          .find(([tag]) => tag.includes('alt="二维码"'))?.[0];
      const qrSource = qr && /\bsrc="([^"]+)"/.exec(qr)?.[1];
      if (!qrSource) throw new Error('footer QR image missing');
      // Check every article photo, including images beyond the first paint
      // window. Each selector also provides its document geometry.
      const probes = [];
      if (!images.length) throw new Error('article images missing');
      for (const src of new Set([...images, qrSource])) {
        if (!src) throw new Error('article images missing');
        const selected = await runtime.screenshotTiles({
          ...common, selector: `img[src="${src}"]`, tile: {height: 32000},
          path: path.join(output, `${id}-probe-${probes.length}-{n}.png`),
        });
        if (selected.tiles.length !== 1) throw new Error('unexpected image geometry');
        probes.push({source: src, ...selected.tiles[0], image: undefined, stats: selected.stats});
      }
      results.pages.push({id, fullPath, stats: full.stats, tileStats: tiled.stats, tiles, probes});
    }
    const library = path.join(started.enginePath,
        process.platform === 'win32' ? 'shotium.dll' : process.platform === 'darwin' ? 'libshotium.dylib' : 'libshotium.so');
    results.librarySha256 = createHash('sha256').update(fs.readFileSync(library)).digest('hex');
    fs.writeFileSync(path.join(output, 'captures.json'), JSON.stringify(results, null, 2));
  } finally {
    await runtime.stop();
  }
}

main().catch((error) => { console.error(error); process.exitCode = 1; });
