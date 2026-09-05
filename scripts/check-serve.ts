// Exercises shotium --serve over its own protocol.
//
// The worker is the half of shotium that has no other test: the CLI path is
// covered by accept.ts comparing pixels against an oracle, but that says
// nothing about framing, about whether a second request on one process renders
// the same as the first, or about what happens when a request is refused.
//
// What it checks, in order:
//
//   1. two requests on one process produce byte-identical PNGs -- which is the
//      claim that made a resident worker possible in the first place, since
//      ShotRenderer used to overwrite its page without detaching the old one
//   2. the worker's PNG is byte-identical to the CLI's, so the two entry points
//      really do share one path
//   3. allowFileAccess actually gates subresources: the same document without it
//      must not come back the same, because the corpus loads fonts and bitmaps
//      over file:, and --allow-file-access moves what silence means without
//      overriding a request that states the field either way
//   4. the capture geometry -- fullPage, clip, selector, scale -- lands on the
//      exact pixels shot/testdata/features.html states it should
//   5. omitBackground really keeps the alpha channel, checked by decoding the
//      PNG rather than by comparing sizes
//   6. jpeg and webp come back as jpeg and webp
//   7. a malformed request, an impossible combination and an unknown field are
//      answered and the stream survives them, rather than taking the worker down
//   8. closing stdin exits cleanly
//
//   pnpm verify:serve out/Shot/shotium.exe
//
// Relative paths are resolved against the repository root.

import {chmodSync, existsSync, mkdirSync, readFileSync, rmSync, rmdirSync, statSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';

import {decodePng, pixel, pngChannels, pngSize, rgb, row, sameRgb} from './lib/png.ts';
import {resolve} from './lib/repo.ts';
import {Checks, sha256} from './lib/report.ts';
import {ServeWorker} from './lib/serve.ts';

// A document taller than blink paints from one scroll position (32767 CSS
// pixels): white all the way down, and a red strip as the last 10px, so that
// "did the bottom get painted" is one pixel read. A file rather than a data:
// URL, which the worker does not load.
const TALL_HEIGHT = 36000;

function fixture(name: string, contents: string): string {
  const file = resolve('shot/testdata/out', name);
  mkdirSync(path.dirname(file), {recursive: true});
  writeFileSync(file, contents);
  return file;
}

function tallDocument(): string {
  return fixture(
      'tall_check.html',
      `<body style='margin:0'><div style='height:${TALL_HEIGHT - 10}px;background:#fff'></div>` +
          `<div style='height:10px;background:#f00'></div></body>`);
}

const RED = [255, 0, 0], WHITE = [255, 255, 255], BLUE = [0x00, 0x66, 0xcc], GREEN = [0, 255, 0];
const size = (png: Buffer) => `(${pngSize(png).join(', ')})`;
const sizeIs = (png: Buffer, w: number, h: number) => pngSize(png)[0] === w && pngSize(png)[1] === h;

async function main(exeArg: string, opts: {corpus: string; features: string; width: number; height: number}): Promise<number> {
  const exe = resolve(exeArg);
  const corpus = resolve(opts.corpus);
  const features = resolve(opts.features);
  const checks = new Checks();
  const err = (h: {error?: string}) => h.error ?? '';

  // The CLI's answer, to compare the worker against.
  const cliPng = resolve('shot/testdata/out/shot_cli.png');
  mkdirSync(path.dirname(cliPng), {recursive: true});
  await execa(exe, ['--file', corpus, '--width', String(opts.width), '--height', String(opts.height), '--output', cliPng], {stdio: 'inherit'});
  const invalidCli = await execa(exe, ['--file', corpus, '--tile-height', '32001', '--output', cliPng], {reject: false});
  checks.check(invalidCli.exitCode === 2 && invalidCli.stderr.includes('1 to 32000'),
               'the CLI rejects tile heights above the paint limit', invalidCli.stderr.trim());
  const cliDigest = sha256(readFileSync(cliPng));
  console.log(`\nCLI     sha256 ${cliDigest.slice(0, 32)}`);

  const proc = new ServeWorker(exe);
  const request = {file: corpus, width: opts.width, height: opts.height, allowFileAccess: true};
  const ask = (extra: Record<string, unknown>, base: Record<string, unknown> = request) => proc.ask({...base, ...extra});

  checks.section('two requests on one process');
  const digests: string[] = [];
  for (let i = 0; i < 2; i++) {
    const [header, payload] = await ask({});
    checks.check(header.ok === true, `request ${i + 1} succeeded`, err(header));
    checks.check(payload.length === header.bytes, `request ${i + 1} payload length matches header`, `${payload.length} vs ${header.bytes}`);
    digests.push(sha256(payload));
    console.log(`        sha256 ${digests[i].slice(0, 32)}`);
  }
  checks.check(digests[0] === digests[1], 'the second render is byte-identical to the first');
  checks.check(digests[0] === cliDigest, "the worker's PNG is byte-identical to the CLI's");

  checks.section('allowFileAccess actually gates subresources');
  const {allowFileAccess: _dropped, ...deniedRequest} = request;
  void _dropped;
  proc.send(deniedRequest);
  let [header, payload] = await proc.recv();
  checks.check(header.ok === true, 'request without file access still renders', err(header));
  const denied = sha256(payload);
  checks.check(denied !== digests[0], 'and renders differently, because fonts and images were refused');

  // The same question asked of a worker started with --allow-file-access.
  // The point of the flag is that the answer to silence belongs to whoever
  // launched the process rather than to whoever sends the request, so both
  // halves matter: silence now means yes, and an explicit no is still obeyed.
  checks.section('--allow-file-access moves the default, not the decision');
  const permissive = new ServeWorker(exe, ['--allow-file-access']);
  [header, payload] = await permissive.ask(deniedRequest);
  checks.check(header.ok === true, 'a silent request renders on a permissive worker', err(header));
  checks.check(sha256(payload) === digests[0], 'and matches the render that asked for file access');
  [header, payload] = await permissive.ask({...deniedRequest, allowFileAccess: false});
  checks.check(header.ok === true, 'an explicit allowFileAccess:false still renders', err(header));
  checks.check(sha256(payload) === denied, 'and matches the refused render, so the request still wins');
  await permissive.close(30_000);

  // features.html states its own geometry, so these are exact.
  checks.section('capture geometry');
  const geometry = {file: features, width: 400, height: 300, allowFileAccess: true};
  [header, payload] = await ask({}, geometry);
  checks.check(header.ok === true, 'the feature page renders', err(header));
  checks.check(sizeIs(payload, 400, 300), 'the viewport shot is 400x300', size(payload));

  let full: Buffer;
  [header, full] = await ask({fullPage: true}, geometry);
  checks.check(header.ok === true, 'fullPage renders', err(header));
  checks.check(sizeIs(full, 400, 2000), 'fullPage reaches the bottom of a 2000px document', size(full));

  let clipped: Buffer;
  [header, clipped] = await ask({clip: {x: 40, y: 60, width: 200, height: 120}}, geometry);
  checks.check(header.ok === true, 'clip renders', err(header));
  checks.check(sizeIs(clipped, 200, 120), 'clip is exactly 200x120', size(clipped));

  let selected: Buffer;
  [header, selected] = await ask({selector: '#box'}, geometry);
  checks.check(header.ok === true, 'selector renders', err(header));
  checks.check(sizeIs(selected, 200, 120), 'the selected element is exactly 200x120', size(selected));

  checks.section('ordinary output paths stay literal');
  const literalPath = resolve(`shot/testdata/out/literal-{n}-${process.pid}.png`);
  const expandedPath = literalPath.replace('{n}', '1');
  for (const candidate of [literalPath, expandedPath]) rmSync(candidate, {force: true});
  [header, payload] = await ask({path: literalPath}, geometry);
  checks.check(header.ok === true, 'an ordinary path containing {n} renders', err(header));
  checks.check(header.path === literalPath && existsSync(literalPath) && !existsSync(expandedPath) && payload.length === 0,
               'and reports and writes the literal path only', String(header.path));
  rmSync(literalPath, {force: true});

  if (process.platform !== 'win32') {
    const modePath = resolve(`shot/testdata/out/mode-${process.pid}.png`);
    writeFileSync(modePath, 'old screenshot');
    chmodSync(modePath, 0o640);
    [header] = await ask({path: modePath}, geometry);
    checks.check(header.ok === true, 'an existing POSIX output is replaced', err(header));
    const mode = statSync(modePath).mode & 0o777;
    checks.check(mode === 0o640, 'without changing its permission bits', '0o' + mode.toString(8));
    rmSync(modePath, {force: true});
  }

  checks.section('a document taller than one paint reaches its bottom');
  const tall = {file: tallDocument(), width: 400, height: 300};
  let banded: Buffer;
  [header, banded] = await ask({fullPage: true, scale: 0.25}, tall);
  checks.check(header.ok === true, 'fullPage renders a 36000px page', err(header));
  checks.check(sizeIs(banded, 100, TALL_HEIGHT / 4), 'as one image of the whole height', size(banded));
  {
    const image = decodePng(banded);
    const bottom = rgb(image, 50, TALL_HEIGHT / 4 - 2);
    checks.check(sameRgb(bottom, RED), 'and the last rows are painted, not left blank', `(${bottom.join(', ')})`);
    const above = rgb(image, 50, Math.floor(32767 / 4) - 2);
    checks.check(sameRgb(above, WHITE), 'with the rows either side of the paint limit intact', `(${above.join(', ')})`);
  }

  checks.section('tiles');
  proc.send({...tall, fullPage: true, tile: {height: 8000}});
  let tiles: Buffer[];
  [header, tiles] = await proc.recvTiles();
  checks.check(header.ok === true, 'a tiles request renders', err(header));
  const listed = header.tiles ?? [];
  checks.check(listed.length === 5 && tiles.length === 5, '36000px in 8000px tiles is five tiles', `${listed.length} listed, ${tiles.length} frames`);
  checks.check(listed.map((t) => t.y).join() === '0,8000,16000,24000,32000', 'stacked top to bottom', `[${listed.map((t) => t.y).join(', ')}]`);
  checks.check(listed.map((t) => t.height).join() === '8000,8000,8000,8000,4000', 'the last tile being what was left', `[${listed.map((t) => t.height).join(', ')}]`);
  const sizes = tiles.map((t) => pngSize(t));
  checks.check(sizes.map((s) => s.join('x')).join() === '400x8000,400x8000,400x8000,400x8000,400x4000',
               "and each frame is a PNG of its tile's size", sizes.map((s) => `(${s.join(', ')})`).join(', '));
  checks.check(tiles.every((t, i) => t.length === listed[i]?.bytes), 'whose lengths match the header');
  {
    const last = decodePng(tiles[tiles.length - 1]);
    const bottom = rgb(last, 200, 3998);
    checks.check(sameRgb(bottom, RED), 'the last tile ends in the red strip', `(${bottom.join(', ')})`);
  }

  proc.send({...tall, fullPage: true, tile: {height: 8000}, path: 'tiles.png'});
  [header] = await proc.recvTiles();
  checks.check(header.ok === false && err(header).includes('{n}'), 'a tiles path without {n} is refused by name', err(header));
  [header, payload] = await ask({}, geometry);
  checks.check(header.ok === true && sizeIs(payload, 400, 300), 'and the stream is still in step afterwards');
  checks.check(selected.equals(clipped), 'selector and clip found the same box, to the byte');

  checks.section("fractional tiles share the whole image's device grid");
  const fractionalPath = fixture(
      'fractional_tiles.html',
      '<style>html,body{margin:0;width:20px}body{height:1001px;' +
          'background:repeating-linear-gradient(to bottom,#123456 0 1px,#abcdef 1px 2px)}</style>');
  const fractional = {file: fractionalPath, width: 20, height: 100, allowFileAccess: true, fullPage: true, scale: 1.5};
  let fractionalWhole: Buffer;
  [header, fractionalWhole] = await ask({}, fractional);
  checks.check(header.ok === true, 'the fractional whole image renders', err(header));
  proc.send({...fractional, tile: {height: 333}});
  let fractionalTiles: Buffer[];
  [header, fractionalTiles] = await proc.recvTiles();
  checks.check(header.ok === true, 'the fractional tiles render', err(header));
  {
    const whole = decodePng(fractionalWhole);
    const wholeChannels = pngChannels(fractionalWhole);
    const images = fractionalTiles.map((t) => decodePng(t));
    const heights = images.map((i) => i.height);
    checks.check(heights.reduce((a, b) => a + b, 0) === whole.height, 'their heights add up to the whole image', `[${heights.join(', ')}] vs ${whole.height}`);
    let y = 0, same = true;
    for (const [i, image] of images.entries()) {
      if (image.width !== whole.width || pngChannels(fractionalTiles[i]) !== wholeChannels) same = false;
      for (let r = 0; r < image.height && same; r++, y++) {
        if (!row(image, r).equals(row(whole, y))) same = false;
      }
    }
    checks.check(same && y === whole.height, 'and concatenating them reproduces every whole-image pixel');
  }

  checks.section('scrolled capture windows do not repeat viewport content');
  const fixedPath = fixture(
      'fixed_clip_window.html',
      '<style>html,body{margin:0;width:100px}body{height:34000px;background:#06c}' +
          '.fixed{position:fixed;inset:0 0 auto;height:20px;background:#f00}</style><div class=fixed></div>');
  const fixed = {file: fixedPath, width: 100, height: 720, allowFileAccess: true, scale: 0.25};
  let shortClip: Buffer, longClip: Buffer;
  [header, shortClip] = await ask({clip: {x: 0, y: 1000, width: 100, height: 100}}, fixed);
  checks.check(header.ok === true, 'the short offset clip renders', err(header));
  [header, longClip] = await ask({clip: {x: 0, y: 1000, width: 100, height: 32000}}, fixed);
  checks.check(header.ok === true, 'the windowed offset clip renders', err(header));
  checks.check(sameRgb(rgb(decodePng(shortClip), 12, 2), BLUE) && sameRgb(rgb(decodePng(longClip), 12, 2), BLUE),
               'the same document rows stay clear of the fixed header');

  const stickyPage = (name: string, before: number, sticky: string, after: number) => fixture(
      name,
      `<style>html,body{margin:0;width:100px}body{background:#06c}.before{height:${before}px}` +
          `.sticky{position:sticky;${sticky};height:20px;background:#0f0}.after{height:${after}px}</style>` +
          '<div class=before></div><div class=sticky></div><div class=after></div>');
  const topStickyPath = stickyPage('top_sticky_window.html', 31990, 'top:0', 3990);
  const insetStickyPath = stickyPage('inset_sticky_window.html', 31990, 'top:10px', 3990);
  const bottomStickyPath = stickyPage('bottom_sticky_window.html', 33000, 'bottom:0', 2980);

  const stickyRows = async (file: string): Promise<[number[], number[]]> => {
    const [h, image] = await ask({file, width: 100, height: 720, allowFileAccess: true, fullPage: true, scale: 0.25}, {});
    checks.check(h.ok === true, `${path.basename(file)} renders`, err(h));
    const decoded = decodePng(image);
    const painted: number[] = [], green: number[] = [];
    for (let y = 0; y < decoded.height; y++) {
      const colour = rgb(decoded, 12, y);
      if (!sameRgb(colour, BLUE)) painted.push(y);
      if (sameRgb(colour, GREEN)) green.push(y);
    }
    return [painted, green];
  };
  const range = (a: number, b: number) => Array.from({length: b - a}, (_, i) => a + i).join();
  const [topPainted, topGreen] = await stickyRows(topStickyPath);
  const [insetPainted, insetGreen] = await stickyRows(insetStickyPath);
  const [, bottomGreen] = await stickyRows(bottomStickyPath);
  checks.check(topPainted.join() === range(7997, 8003) && topGreen.join() === range(7998, 8002),
               'a top-sticky box crossing the seam keeps exactly its flow rows', `painted [${topPainted.join(', ')}], solid [${topGreen.join(', ')}]`);
  checks.check(insetPainted.join() === range(7997, 8003) && insetGreen.join() === range(7998, 8002),
               'a sticky inset does not hide the suffix beyond the seam', `painted [${insetPainted.join(', ')}], solid [${insetGreen.join(', ')}]`);
  checks.check(bottomGreen.length === 5 && bottomGreen.join() === range(175, 180),
               'a bottom-sticky box is kept in only the requested viewport', `[${bottomGreen.join(', ')}]`);

  checks.section('tile files commit as one result');
  const atomicDir = resolve('shot/testdata/out/atomic_tiles');
  mkdirSync(atomicDir, {recursive: true});
  const firstTile = path.join(atomicDir, 'tile-1.png');
  const blockedTile = path.join(atomicDir, 'tile-2.png');
  const sentinel = Buffer.from('the old first tile');
  writeFileSync(firstTile, sentinel);
  if (existsSync(blockedTile) && statSync(blockedTile).isFile()) rmSync(blockedTile);
  mkdirSync(blockedTile, {recursive: true});
  proc.send({...fractional, scale: 1, tile: {height: 600}, path: path.join(atomicDir, 'tile-{n}.png')});
  [header] = await proc.recvTiles();
  checks.check(header.ok === false, 'a tile set reports a failed final commit');
  checks.check(readFileSync(firstTile).equals(sentinel), 'and rolls an earlier destination back to its old bytes');
  rmdirSync(blockedTile);

  if (process.platform !== 'win32') {
    const modeTemplate = path.join(atomicDir, 'mode-{n}.png');
    const modeTile = modeTemplate.replace('{n}', '1');
    writeFileSync(modeTile, 'old tile');
    chmodSync(modeTile, 0o640);
    proc.send({...fractional, scale: 1, tile: {height: 600}, path: modeTemplate});
    let modeTiles: Buffer[];
    [header, modeTiles] = await proc.recvTiles();
    checks.check(header.ok === true && modeTiles.every((t) => t.length === 0), 'an existing POSIX tile is replaced', err(header));
    const mode = statSync(modeTile).mode & 0o777;
    checks.check(mode === 0o640, "without changing the tile's permission bits", '0o' + mode.toString(8));
    for (const tile of header.tiles ?? []) {
      if (tile.path && existsSync(tile.path)) rmSync(tile.path);
    }
  }

  {
    const image = decodePng(clipped);
    const corner = pixel(image, 0, 0);
    checks.check(sameRgb(corner, [0xcc, 0x00, 0x00]), 'and the box really is #cc0000', `(${corner.join(', ')})`);
  }

  checks.section('selectors beyond the viewport');
  const oversizedGeometry = {file: path.join(path.dirname(features), 'selector_oversized.html'), width: 400, height: 300, allowFileAccess: true};
  let oversized: Buffer;
  [header, oversized] = await ask({selector: '#oversized'}, oversizedGeometry);
  checks.check(header.ok === true, 'an oversized selector renders', err(header));
  checks.check(sizeIs(oversized, 800, 600), "and uses the element's full 800x600 box", size(oversized));
  {
    const image = decodePng(oversized);
    checks.check(sameRgb(rgb(image, 100, 100), [0x00, 0xcc, 0x00]), 'content inside the original viewport is present', `(${pixel(image, 100, 100).join(', ')})`);
    checks.check(sameRgb(rgb(image, 300, 100), [0xcc, 0x00, 0x00]), 'and 50vw stayed 200px instead of reflowing to 400px', `(${pixel(image, 300, 100).join(', ')})`);
  }

  const centeredGeometry = {file: path.join(path.dirname(features), 'selector_centered.html'), width: 400, height: 300, allowFileAccess: true};
  let centered: Buffer;
  [header, centered] = await ask({selector: '#centered'}, centeredGeometry);
  checks.check(header.ok === true, 'a centered selector larger than the viewport renders', err(header));
  checks.check(sizeIs(centered, 800, 600), 'and negative initial bounds settle to the full 800x600 box', size(centered));
  {
    const image = decodePng(centered);
    checks.check(sameRgb(rgb(image, 0, 0), BLUE) && sameRgb(rgb(image, 799, 599), BLUE), 'and both far corners were painted',
                 `(${pixel(image, 0, 0).join(', ')}) / (${pixel(image, 799, 599).join(', ')})`);
  }

  let scaled: Buffer;
  [header, scaled] = await ask({scale: 2}, geometry);
  checks.check(header.ok === true, 'scale 2 renders', err(header));
  checks.check(sizeIs(scaled, 800, 600), 'scale 2 doubles the pixels, not the layout', size(scaled));

  checks.section('omitBackground keeps the alpha channel');
  let opaque: Buffer;
  [header, opaque] = await ask({}, geometry);
  {
    const channels = pngChannels(opaque);
    const corner = rgb(decodePng(opaque), 399, 299);
    // Three channels, not four: the encoder drops an alpha channel that is
    // opaque everywhere, so "no alpha channel at all" is the stronger form of
    // what this is asserting.
    checks.check(channels === 3, 'without it the PNG has no alpha channel', `${channels} channels`);
    checks.check(sameRgb(corner, WHITE), 'and the page sits on white', `(${corner.join(', ')})`);
  }
  let transparent: Buffer;
  [header, transparent] = await ask({omitBackground: true}, geometry);
  checks.check(header.ok === true, 'omitBackground renders', err(header));
  {
    const channels = pngChannels(transparent);
    const image = decodePng(transparent);
    const corner = pixel(image, 399, 299);
    checks.check(channels === 4, 'with it the PNG keeps its alpha channel', `${channels} channels`);
    checks.check(corner[3] === 0, 'and the uncovered corner is transparent', `(${corner.join(', ')})`);
    checks.check(sameRgb(rgb(image, 100, 100), [0xcc, 0x00, 0x00]), 'and the painted box is still there', `(${pixel(image, 100, 100).join(', ')})`);
  }

  checks.section('the other encoders');
  let jpeg: Buffer;
  [header, jpeg] = await ask({type: 'jpeg', quality: 90}, geometry);
  checks.check(header.ok === true, 'jpeg renders', err(header));
  checks.check(jpeg.subarray(0, 3).equals(Buffer.from('ffd8ff', 'hex')), 'and is a JPEG', jpeg.subarray(0, 3).toString('hex'));
  let webp: Buffer;
  [header, webp] = await ask({type: 'webp', quality: 90}, geometry);
  checks.check(header.ok === true, 'webp renders', err(header));
  checks.check(webp.subarray(0, 4).toString() === 'RIFF' && webp.subarray(8, 12).toString() === 'WEBP', 'and is a WebP', webp.subarray(0, 12).toString('hex'));

  checks.section('bad input is answered, not fatal');
  [header, payload] = await proc.ask({file: corpus, tile: {height: 32001}});
  checks.check(header.ok === false && payload.length === 0, 'a tile taller than the paint limit is rejected');
  checks.check(err(header).includes('tile.height') && err(header).includes('32000'), 'and the error names the supported tile.height range', err(header));

  [header] = await proc.ask({file: corpus, width: '1248'});
  checks.check(header.ok === false, 'a string where a number belongs is rejected');
  checks.check(err(header).includes('width'), 'the error names the field', err(header));

  [header] = await proc.ask({file: features, fullPage: true, clip: {x: 0, y: 0, width: 10, height: 10}});
  checks.check(header.ok === false, 'fullPage and clip together are rejected');
  checks.check(err(header).includes('fullPage'), 'and the error says why', err(header));

  [header] = await proc.ask({file: features, type: 'jpeg', omitBackground: true});
  checks.check(header.ok === false, 'omitBackground on a jpeg is rejected, not silently dropped');
  checks.check(err(header).includes('alpha'), 'and the error says why', err(header));

  [header] = await proc.ask({file: features, pngCompression: 'balanced'});
  checks.check(header.ok === false, 'the removed PNG compression mode is rejected');
  checks.check(err(header).includes('removed'), 'and the error explains that it is gone', err(header));

  [header] = await proc.ask({file: features, selector: '!!not a selector'});
  checks.check(header.ok === false, 'an invalid selector is rejected');
  checks.check(err(header).includes('selector'), 'and named', err(header));

  checks.section('the stream survived all of that');
  [header, payload] = await proc.ask(request);
  checks.check(header.ok === true, 'a good request after five bad ones still works');
  checks.check(sha256(payload) === digests[0], 'and produces the same bytes as the first');

  checks.section('closing stdin exits cleanly');
  const code = await proc.close(60_000);
  checks.check(code === 0, 'exit code is 0', String(code));

  return checks.finish();
}

const cli = cac('check-serve');
cli.command('<exe>', 'exercise shotium --serve over its own protocol')
    .option('--corpus <file>', 'the document both entry points render', {default: 'shot/testdata/render_corpus.html'})
    .option('--features <file>', 'the page whose geometry is asserted', {default: 'shot/testdata/features.html'})
    .option('--width <px>', 'corpus viewport width', {default: 1248})
    .option('--height <px>', 'corpus viewport height', {default: 1320})
    .action(async (exe: string, options: {corpus: string; features: string; width: number; height: number}) => {
      process.exitCode = await main(exe, {...options, width: Number(options.width), height: Number(options.height)});
    });
cli.help();
cli.parse();
