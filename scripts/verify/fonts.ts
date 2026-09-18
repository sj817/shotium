// Check that a character the page's font lacks is drawn from a system font.
//
// System font fallback is the path a page never names: the font-family list
// has no glyph for the character, Blink asks the host for a font that does,
// opens it, and shapes with it. On Linux that is fontconfig through
// ShotSandboxSupport, and the font it names is opened by file path; on
// Windows it is DirectWrite, on macOS Core Text. When the opening step fails
// nothing reports it -- Blink falls through to the last-resort font, and the
// character comes out as that font's .notdef, or as nothing at all.
//
// Every case here renders one character in the checked-in Ahem font, whose
// glyphs are ASCII only, so the character must come from a system font. The
// render is compared with two controls that need no golden image:
//
//   notdef   the same page with U+0378, a code point no font has: what a
//            missing glyph looks like on this host
//   blank    the same page with no character at all
//
// A render equal to either is a fallback that found nothing. The emoji case
// must additionally contain colour: every colour emoji font the three CI
// hosts carry -- Segoe UI Emoji, Apple Color Emoji, Noto Color Emoji -- is a
// colour font, and the Linux one is CBDT, whose PNG strikes are decoded by
// a registry shot_runtime.cc has to fill itself. A monochrome glyph where a
// colour one was expected is the fallback picking a text font for an
// emoji-presentation character, which is a different failure.
//
// Until 2026-09-18 every case failed on Linux: ShotSandboxSupport (v0.10.0)
// made FontCache::CreateTypeface take Chrome's sandbox branch, which opens
// the fallback font by a fontconfig id the in-process lookup never assigns.
// CJK text on a host whose default font is Latin rendered as boxes and every
// emoji as a blank; no check saw it because the pixel checks compare renders
// with each other, and a box equals a box.
//
//   pnpm verify:fonts out/Shot/shotium.exe
//
// The host needs a CJK font and a colour emoji font: Windows and macOS ship
// them; on Debian and Ubuntu that is fonts-noto-cjk and
// fonts-noto-color-emoji. Relative paths are resolved against the repository
// root.

import {existsSync, mkdtempSync, readFileSync, rmSync, writeFileSync} from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

import {cac} from 'cac';
import {execa} from 'execa';

import {decodePng, type Image} from '../lib/png.ts';
import {resolve} from '../lib/repo.ts';

// (label, text, lang, must the glyph carry colour)
const CASES: Array<[string, string, string, boolean]> = [
  ['cjk ideograph', '完', 'zh-CN', false],  // 完
  ['hangul', '한', 'ko', false],            // 한
  // 🎉 has Emoji_Presentation, so Blink asks for a colour font first, and
  // DejaVu Sans, the usual Linux default, has no monochrome glyph for it.
  ['colour emoji', '\u{1f389}', 'en', true],
];

// U+0378 is unassigned in every Unicode version, so no font claims it.
const NOTDEF = '\u0378';

const AHEM = pathToFileURL(resolve('shot/testdata/ahem.ttf')).href;

// One large character on white. Ahem draws ASCII as solid squares, so a wrong
// fallback to it, or to the last-resort font's .notdef, is a different image
// from any real glyph.
const page = (text: string, lang: string) => `<!doctype html><html lang="${lang}"><head><meta charset="utf-8">
<style>@font-face{font-family:probe;src:url("${AHEM}") format("truetype")}
body{margin:0;background:#fff;color:#000;font:96px/1.2 probe}</style></head>
<body><span>${text}</span></body></html>`;

async function render(exe: string, html: string, png: string): Promise<Buffer | null> {
  await execa(exe, ['--file', html, '--width', '200', '--height', '140', '--output', png], {reject: false});
  return existsSync(png) ? readFileSync(png) : null;
}

// Largest gap between the channels of any pixel: 0 for a greyscale image.
function chroma(image: Image): number {
  let worst = 0;
  for (let i = 0; i < image.data.length; i += 4) {
    const r = image.data[i], g = image.data[i + 1], b = image.data[i + 2];
    const gap = Math.max(r, g, b) - Math.min(r, g, b);
    if (gap > worst) worst = gap;
  }
  return worst;
}

// Antialiasing can tint a grey edge by a few levels; a colour glyph is far
// beyond that.
const COLOUR_GAP = 64;

async function main(exeArg: string): Promise<number> {
  const exe = resolve(exeArg);
  if (!existsSync(exe)) {
    console.log(`no such binary: ${exe}`);
    return 2;
  }
  const failures: string[] = [];
  const tmp = mkdtempSync(path.join(os.tmpdir(), 'shot-fonts-'));
  try {
    const controls: Record<string, Buffer> = {};
    for (const [name, text] of [['notdef', NOTDEF], ['blank', '']] as const) {
      const html = path.join(tmp, `${name}.html`);
      writeFileSync(html, page(text, 'en'));
      const image = await render(exe, html, path.join(tmp, `${name}.png`));
      if (image === null) {
        console.log(`the ${name} control produced no PNG`);
        return 1;
      }
      controls[name] = image;
    }
    if (controls.notdef.equals(controls.blank)) {
      console.log('  note: a missing glyph draws nothing on this host, so the two controls coincide');
    }

    for (const [index, [label, text, lang, wantsColour]] of CASES.entries()) {
      const html = path.join(tmp, `case${index}.html`);
      writeFileSync(html, page(text, lang));
      const image = await render(exe, html, path.join(tmp, `case${index}.png`));
      let verdict: string;
      if (image === null) verdict = 'NORENDER  no PNG';
      else if (image.equals(controls.blank)) verdict = 'BLANK     nothing drawn';
      else if (image.equals(controls.notdef)) verdict = 'NOTDEF    the missing-glyph box';
      else if (wantsColour && chroma(decodePng(image)) < COLOUR_GAP) verdict = 'MONO      a glyph, but not a colour one';
      else verdict = 'PASS      drawn from a system font';
      console.log(`  ${verdict.slice(0, 9)} ${label.padEnd(16)} ${verdict.slice(10)}`);
      if (!verdict.startsWith('PASS')) failures.push(`${label}: ${verdict.slice(10)}`);
    }
  } finally {
    rmSync(tmp, {recursive: true, force: true});
  }

  console.log();
  if (failures.length) {
    for (const f of failures) console.log(`FAIL  ${f}`);
    return 1;
  }
  console.log(`ALL ${CASES.length} FALLBACK CASES DRAWN`);
  return 0;
}

const cli = cac('pnpm verify:fonts');
cli.command('<exe>', 'check that characters outside the page font come from a system font')
    .action(async (exe: string) => {
      process.exitCode = await main(exe);
    });
cli.help();
cli.parse();
