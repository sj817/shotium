// Check that removing ICU's CJK converter tables changed no encoding.
//
// icu-repack drops six converter tables from the ICU data on the grounds
// that Blink decodes those encodings itself in TextCodecCjk and never asks
// ICU for them. That is a claim about two files agreeing (ShouldSkipEncoding
// in text_codec_icu.cc and kSupportedCanonicalNames in text_codec_cjk.cc),
// so it deserves a test that reads neither.
//
// Each encoding is rendered three ways and the result is classified:
//
//   decoded     the legacy bytes render the same pixels as the same text in UTF-8
//   undecoded   they render the same pixels as those bytes read *as* UTF-8, i.e.
//               the charset declaration was ignored and the text is mojibake
//   neither     something else entirely, which is what a broken converter looks
//               like
//
// No golden images: the page is its own oracle in every case.
//
// *** What this build does today ***
//
// Every encoding except pure-ASCII cases comes out `undecoded`, and that is
// not this cut's doing -- it reproduces with the untrimmed `cast` data. The
// cause is LocalFrame::ForceSynchronousDocumentInstall, which shot uses to
// install a document without a DocumentLoader:
//
//     DocumentParser* parser = document->OpenForNavigation(
//         kForceSynchronousParsing, mime_type, AtomicString("UTF-8"));
//
// The encoding is hardcoded. An explicit encoding outranks `<meta charset>`
// in TextResourceDecoder, so no declaration a page makes is ever consulted,
// and shot_renderer.cc's SetDefaultTextEncodingName cannot reach it either.
// Whoever fixes that should re-run this, because it will move most cases to
// `decoded` and put the ICU converter tables genuinely in use for the first
// time.
//
// The invariant this enforces is therefore not "everything decodes" -- it is
// that the CJK encodings, whose ICU tables were removed, behave exactly like
// the single-byte ones, whose tables were kept. If the two groups ever
// disagree, the data cut is the reason.
//
//   pnpm verify:charset out/Shot/shotium.exe
//
// The legacy bytes come from iconv-lite, which encodes every charset here;
// Node's own TextEncoder only writes UTF-8. Relative paths are resolved
// against the repository root.

import {existsSync, mkdtempSync, readFileSync, rmSync, writeFileSync} from 'node:fs';
import os from 'node:os';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';
import iconv from 'iconv-lite';

import {resolve} from './lib/repo.ts';

// (label, iconv-lite codec, charset name for <meta>, sample text, group)
//
// "cjk" are the encodings whose ICU tables icu-repack removes; Blink decodes
// them in TextCodecCjk. "single" are the ones whose ICU tables stay.
const CASES: Array<[string, string, string, string, 'cjk' | 'single']> = [
  ['shift_jis', 'shift_jis', 'shift_jis', 'ひらがなカタカナ漢字', 'cjk'],
  ['euc-jp', 'euc-jp', 'euc-jp', 'ひらがなカタカナ漢字', 'cjk'],
  ['gbk', 'gbk', 'gbk', '简体中文测试文本', 'cjk'],
  ['gb18030', 'gb18030', 'gb18030', '简体中文测试文本', 'cjk'],
  ['big5', 'big5', 'big5', '繁體中文測試文字', 'cjk'],
  ['euc-kr', 'euc-kr', 'euc-kr', '한국어시험문장', 'cjk'],
  ['windows-1251', 'win1251', 'windows-1251', 'Проверка кириллицы', 'single'],
  ['iso-8859-2', 'iso-8859-2', 'iso-8859-2', 'Zażółć gęślą jaźń', 'single'],
  ['iso-8859-7', 'iso-8859-7', 'iso-8859-7', 'Ελληνικό κείμενο', 'single'],
  ['koi8-r', 'koi8-r', 'koi8-r', 'Проверка кодировки', 'single'],
  ['windows-1256', 'win1256', 'windows-1256', 'نص عربي', 'single'],
];

// No font-family and no web fonts: whatever the system picks it picks the
// same way for all three renders, and 40px is large enough that a wrong
// codepoint cannot land on the same pixels as a right one.
const page = (charset: string, text: string) =>
    `<!doctype html><html><head><meta charset="${charset}"><style>body{margin:0;background:#fff;font-size:40px;line-height:1.4}</style></head><body>${text}</body></html>`;

type Verdict = 'decoded' | 'undecoded' | 'neither' | 'norender';

async function render(exe: string, html: string, png: string): Promise<Buffer | null> {
  await execa(exe, ['--file', html, '--width', '600', '--height', '160', '--output', png], {reject: false});
  return existsSync(png) ? readFileSync(png) : null;
}

async function classify(exe: string, tmp: string, label: string, codec: string, charset: string, text: string): Promise<[Verdict, string | null]> {
  const legacyBytes = iconv.encode(page(charset, text), codec);
  const paths: Record<string, string> = {
    legacy: path.join(tmp, `${label}.html`),
    // The control for "decoded correctly".
    utf8: path.join(tmp, `${label}.utf8.html`),
    // The control for "charset ignored": the same bytes read as UTF-8.
    mojibake: path.join(tmp, `${label}.moji.html`),
  };
  writeFileSync(paths.legacy, legacyBytes);
  writeFileSync(paths.utf8, Buffer.from(page('utf-8', text), 'utf8'));
  const mojibake = new TextDecoder('utf-8').decode(legacyBytes);
  writeFileSync(paths.mojibake, Buffer.from(mojibake.replace(charset, 'utf-8'), 'utf8'));

  const images: Record<string, Buffer> = {};
  for (const [kind, file] of Object.entries(paths)) {
    const image = await render(exe, file, path.join(tmp, `${label}${kind}.png`));
    if (image === null) return ['norender', kind];
    images[kind] = image;
  }
  if (images.legacy.equals(images.utf8)) return ['decoded', null];
  if (images.legacy.equals(images.mojibake)) return ['undecoded', null];
  return ['neither', null];
}

async function main(exeArg: string): Promise<number> {
  const exe = resolve(exeArg);
  if (!existsSync(exe)) {
    console.log(`no such binary: ${exe}`);
    return 2;
  }
  const results: Record<string, [Verdict, 'cjk' | 'single']> = {};
  const tmp = mkdtempSync(path.join(os.tmpdir(), 'shot-charset-'));
  try {
    for (const [label, codec, charset, text, group] of CASES) {
      const [verdict, detail] = await classify(exe, tmp, label, codec, charset, text);
      results[label] = [verdict, group];
      const note = {
        decoded: 'decodes correctly',
        undecoded: 'charset ignored, renders as mojibake',
        neither: 'matches NEITHER control',
        norender: `the ${detail} render produced no PNG`,
      }[verdict];
      console.log(`  ${verdict.toUpperCase().padEnd(9)} ${label.padEnd(14)} ${group.padEnd(7)} ${note}`);
    }
  } finally {
    rmSync(tmp, {recursive: true, force: true});
  }

  console.log();
  const failures: string[] = [];
  const broken = Object.entries(results).filter(([, [v]]) => v === 'neither' || v === 'norender').map(([k]) => k);
  if (broken.length) failures.push(`these matched neither control: ${broken.join(', ')}`);

  // The invariant: the group whose ICU tables were removed must behave the
  // same as the group whose tables were kept. Either both decode or neither
  // does; a split means the data cut caused it.
  const verdicts: Record<string, Set<Verdict>> = {};
  for (const group of ['cjk', 'single'] as const) {
    const got = new Set(Object.values(results).filter(([, g]) => g === group).map(([v]) => v));
    verdicts[group] = got;
    if (got.size > 1) failures.push(`the ${group} encodings disagree with each other: ${[...got].sort().join(', ')}`);
  }
  if (verdicts.cjk.size === 1 && verdicts.single.size === 1 && [...verdicts.cjk][0] !== [...verdicts.single][0]) {
    failures.push(`the CJK encodings are ${[...verdicts.cjk][0]} but the single-byte ones are ${[...verdicts.single][0]}; the removed ICU converter tables are the difference between them`);
  }
  if (failures.length) {
    for (const f of failures) console.log(`FAIL: ${f}`);
    return 1;
  }
  const state = results[CASES[0][0]][0];
  if (state === 'undecoded') {
    console.log("ALL ENCODINGS CONSISTENT (all undecoded -- see this file's header: ForceSynchronousDocumentInstall hardcodes UTF-8)");
  } else {
    console.log(`ALL ENCODINGS CONSISTENT (${state})`);
  }
  return 0;
}

const cli = cac('check-charset');
cli.command('<exe>', 'check that the CJK and single-byte legacy encodings behave alike')
    .action(async (exe: string) => {
      process.exitCode = await main(exe);
    });
cli.help();
cli.parse();
