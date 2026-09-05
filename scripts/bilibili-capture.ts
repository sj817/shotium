// Captures the two Bilibili fixture articles through the real npm-facing API,
// for check-bilibili. Binary payloads go to files so a failed check leaves
// inspectable evidence, not a giant JSON log.
//
//   pnpm bilibili:capture shotium out/bilibili
//
// The first argument is the package directory to load (a checkout's shotium/,
// or an installed @shotkit/shotium); the second is where the images and
// captures.json go. Relative paths are resolved against the repository root.

import {createHash} from 'node:crypto';
import {mkdirSync, readFileSync, writeFileSync} from 'node:fs';
import {createRequire} from 'node:module';
import path from 'node:path';

import {cac} from 'cac';

import {libraryName, resolve} from './lib/repo.ts';

import type * as Shotium from '../shotium/src/index.ts';
import type {CaptureStats, ScreenshotTile} from '../shotium/src/types.ts';

export const FIXTURES = resolve('shot/testdata/bilibili');

export interface Manifest {
  sources: Record<string, unknown>;
  assets: Array<{path: string; source: string; bytes: number; sha256: string}>;
}

export interface Probe extends Omit<ScreenshotTile, 'image'> {
  source: string;
  stats: CaptureStats;
}

export interface CapturedPage {
  id: string;
  fullPath: string;
  stats: CaptureStats;
  tileStats: CaptureStats;
  tiles: Array<Omit<ScreenshotTile, 'image' | 'path'> & {path: string}>;
  probes: Probe[];
}

export interface Captures {
  enginePath: string;
  librarySha256: string;
  pages: CapturedPage[];
}

export function readManifest(): Manifest {
  return JSON.parse(readFileSync(path.join(FIXTURES, 'manifest.json'), 'utf8')) as Manifest;
}

export async function capture(packageDir: string, output: string): Promise<Captures> {
  const {Runtime} = createRequire(import.meta.url)(packageDir) as typeof Shotium;
  const manifest = readManifest();
  mkdirSync(output, {recursive: true});

  const runtime = new Runtime();
  const started = runtime.start({cacheDir: null});
  if (!started.enginePath) throw new Error('the engine did not report where it was loaded from');
  const results: Captures = {enginePath: started.enginePath, librarySha256: '', pages: []};
  try {
    for (const id of Object.keys(manifest.sources)) {
      const file = path.join(FIXTURES, `${id}.html`);
      const common = {file, viewport: {width: 1440, height: 900}, allowFileAccess: true};
      const fullPath = path.join(output, `${id}.png`);
      const full = await runtime.screenshot({...common, fullPage: true, path: fullPath});
      const tiled = await runtime.screenshotTiles({...common, fullPage: true, tile: {height: 8000}});
      const tiles = tiled.tiles.map(({image, ...region}, i) => {
        if (!Buffer.isBuffer(image)) throw new Error('screenshotTiles did not return a Buffer');
        const tilePath = path.join(output, `${id}-tile-${i}.png`);
        writeFileSync(tilePath, image);
        return {...region, path: tilePath};
      });
      const html = readFileSync(file, 'utf8');
      const tags = [...html.matchAll(/<img\b[^>]*>/g)].map(([tag]) => tag);
      const images = tags.map((tag) => /\bsrc="([^"]+)"/.exec(tag)?.[1])
                         .filter((src): src is string => !!src && manifest.assets.some((a) => a.path === src && a.source.includes('/new_dyn/')));
      const qr = tags.find((tag) => tag.includes('alt="二维码"'));
      const qrSource = qr && /\bsrc="([^"]+)"/.exec(qr)?.[1];
      if (!qrSource) throw new Error('footer QR image missing');
      // Check every article photo, including images beyond the first paint
      // window. Each selector also provides its document geometry.
      const probes: Probe[] = [];
      if (images.length === 0) throw new Error('article images missing');
      for (const src of new Set([...images, qrSource])) {
        const selected = await runtime.screenshotTiles({
          ...common,
          selector: `img[src="${src}"]`,
          tile: {height: 32000},
          path: path.join(output, `${id}-probe-${probes.length}-{n}.png`),
        });
        if (selected.tiles.length !== 1) throw new Error('unexpected image geometry');
        const {image: _image, ...region} = selected.tiles[0];
        void _image;
        probes.push({source: src, ...region, stats: selected.stats});
      }
      results.pages.push({id, fullPath, stats: full.stats, tileStats: tiled.stats, tiles, probes});
    }
    const library = path.join(results.enginePath, libraryName);
    results.librarySha256 = createHash('sha256').update(readFileSync(library)).digest('hex');
    writeFileSync(path.join(output, 'captures.json'), JSON.stringify(results, null, 2));
  } finally {
    await runtime.stop();
  }
  return results;
}

if (process.argv[1] && path.resolve(process.argv[1]) === import.meta.filename) {
  const cli = cac('bilibili-capture');
  cli.command('<package> <output>', 'capture the Bilibili fixtures through the package API')
      .action(async (pkg: string, output: string) => {
        try {
          await capture(resolve(pkg), resolve(output));
        } catch (error) {
          console.error(error);
          process.exitCode = 1;
        }
      });
  cli.help();
  cli.parse();
}
