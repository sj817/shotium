// Check that a document's declared encoding is the one it is decoded with.
//
// shot installs the top-level document with
// LocalFrame::ForceSynchronousDocumentInstall, which has no DocumentLoader to
// carry the Content-Type charset down, so the charset is handed over
// explicitly (shot_renderer.cc) and the decoder ranks it a browser's way:
//
//   byte order mark  >  Content-Type charset  >  <meta charset>  >  sniffing
//
// Each case below is a document whose bytes are in one legacy encoding, with
// one of those three declarations, and the pixels it renders are compared
// with two controls that need no golden image:
//
//   decoded     the same pixels as the same text in UTF-8: the declaration won
//   undecoded   the same pixels as those bytes read the way they would be if
//               the declaration lost (as UTF-8, or as the lying <meta> in the
//               precedence cases): the declaration was ignored, mojibake
//   neither     something else, which is what a broken converter looks like
//
// Every case must come out `decoded`. Until 2026-09-18 every one of them was
// `undecoded`, because the install path hardcoded UTF-8 as the header
// encoding; the CJK cases additionally depend on the six ICU converter tables
// that TextCodecCjk builds its index tables from (scripts/build/icu-repack.ts
// explains why those stay in the data set), so a `neither` on a CJK case and a
// `decoded` on the single-byte ones points at the ICU data.
//
//   pnpm verify:charset out/Shot/shotium.exe
//
// The legacy bytes come from iconv-lite, which encodes every charset here;
// Node's own TextEncoder only writes UTF-8. The header cases are served from
// a loopback http server. Relative paths are resolved against the repository
// root.

import {existsSync, mkdtempSync, readFileSync, rmSync, writeFileSync} from 'node:fs';
import http from 'node:http';
import type {AddressInfo} from 'node:net';
import os from 'node:os';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';
import iconv from 'iconv-lite';

import {resolve} from '../lib/repo.ts';

// How the document says what it is:
//   meta    <meta charset> and nothing else, from disk
//   header  Content-Type charset over http, no meta tag at all
//   both    Content-Type says the truth, <meta> lies: the header must win
//   bom     a UTF-16LE byte order mark and a lying <meta>: the BOM must win
type Declaration = 'meta' | 'header' | 'both' | 'bom';

// (label, iconv-lite codec, charset name, sample text, declaration)
const CASES: Array<[string, string, string, string, Declaration]> = [
  ['shift_jis', 'shift_jis', 'shift_jis', 'ひらがなカタカナ漢字', 'meta'],
  ['euc-jp', 'euc-jp', 'euc-jp', 'ひらがなカタカナ漢字', 'meta'],
  ['gbk', 'gbk', 'gbk', '简体中文测试文本', 'meta'],
  ['gb18030', 'gb18030', 'gb18030', '简体中文测试文本', 'meta'],
  ['big5', 'big5', 'big5', '繁體中文測試文字', 'meta'],
  ['euc-kr', 'euc-kr', 'euc-kr', '한국어시험문장', 'meta'],
  ['windows-1251', 'win1251', 'windows-1251', 'Проверка кириллицы', 'meta'],
  ['iso-8859-2', 'iso-8859-2', 'iso-8859-2', 'Zażółć gęślą jaźń', 'meta'],
  ['iso-8859-7', 'iso-8859-7', 'iso-8859-7', 'Ελληνικό κείμενο', 'meta'],
  ['koi8-r', 'koi8-r', 'koi8-r', 'Проверка кодировки', 'meta'],
  ['windows-1256', 'win1256', 'windows-1256', 'نص عربي', 'meta'],
  ['gbk (header)', 'gbk', 'gbk', '简体中文测试文本', 'header'],
  ['windows-1251 (header)', 'win1251', 'windows-1251', 'Проверка кириллицы', 'header'],
  ['shift_jis (header+meta)', 'shift_jis', 'shift_jis', 'ひらがなカタカナ漢字', 'both'],
  ['utf-16le (bom+meta)', 'utf16le', 'utf-16le', 'ひらがな Проверка 简体', 'bom'],
];

// No font-family and no web fonts: whatever the system picks it picks the
// same way for all three renders, and 40px is large enough that a wrong
// codepoint cannot land on the same pixels as a right one.
const page = (meta: string | null, text: string) =>
    `<!doctype html><html><head>${meta === null ? '' : `<meta charset="${meta}">`}<style>body{margin:0;background:#fff;font-size:40px;line-height:1.4}</style></head><body>${text}</body></html>`;

// The <meta> a lying document carries: a single-byte charset whose every byte
// decodes to something, so the mojibake control is well defined.
const LIE = 'windows-1251';

type Verdict = 'decoded' | 'undecoded' | 'neither' | 'norender';

// A path goes through --file; a URL is the positional argument.
async function render(exe: string, source: string, png: string): Promise<Buffer | null> {
  const input = /^https?:/.test(source) ? [source] : ['--file', source];
  await execa(exe, [...input, '--width', '600', '--height', '160', '--output', png], {reject: false});
  return existsSync(png) ? readFileSync(png) : null;
}

interface Served {
  bytes: Buffer;
  contentType: string;
}

function serve(routes: Map<string, Served>): Promise<http.Server> {
  const server = http.createServer((req, res) => {
    const hit = routes.get(req.url ?? '/');
    if (!hit) {
      res.writeHead(404);
      res.end();
      return;
    }
    res.writeHead(200, {'Content-Type': hit.contentType, 'Content-Length': hit.bytes.length});
    res.end(hit.bytes);
  });
  return new Promise((done) => server.listen(0, '127.0.0.1', () => done(server)));
}

async function classify(exe: string, tmp: string, port: number, routes: Map<string, Served>, index: number,
                        codec: string, charset: string, text: string, how: Declaration): Promise<[Verdict, string | null]> {
  const slug = `case${index}`;
  const meta = how === 'meta' ? charset : how === 'header' ? null : LIE;
  const legacyBytes = how === 'bom'
      ? iconv.encode(page(meta, text), codec, {addBOM: true})
      : iconv.encode(page(meta, text), codec);

  // The control for "decoded correctly", and the control for "declaration
  // ignored": the same bytes read the way they would be if the declaration
  // under test lost -- as UTF-8 when nothing else claims them (what the
  // hardcoded install path did), as the lying <meta> when there is one --
  // written back out as UTF-8 so that the page renders those exact codepoints.
  const utf8 = path.join(tmp, `${slug}.utf8.html`);
  const mojibake = path.join(tmp, `${slug}.moji.html`);
  writeFileSync(utf8, Buffer.from(page('utf-8', text), 'utf8'));
  const ignoredAs = meta === LIE ? LIE : 'utf-8';
  const mojibakeText = iconv.decode(legacyBytes, ignoredAs).replace(/<meta charset="[^"]*">/, '').replace('<head>', '<head><meta charset="utf-8">');
  writeFileSync(mojibake, Buffer.from(mojibakeText, 'utf8'));

  let legacy: string;
  if (how === 'header' || how === 'both') {
    routes.set(`/${slug}.html`, {bytes: legacyBytes, contentType: `text/html; charset=${charset}`});
    legacy = `http://127.0.0.1:${port}/${slug}.html`;
  } else {
    legacy = path.join(tmp, `${slug}.html`);
    writeFileSync(legacy, legacyBytes);
  }

  const images: Record<string, Buffer> = {};
  for (const [kind, source] of Object.entries({legacy, utf8, mojibake})) {
    const image = await render(exe, source, path.join(tmp, `${slug}.${kind}.png`));
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
  const routes = new Map<string, Served>();
  const server = await serve(routes);
  const port = (server.address() as AddressInfo).port;
  const failures: string[] = [];
  const tmp = mkdtempSync(path.join(os.tmpdir(), 'shot-charset-'));
  try {
    for (const [index, [label, codec, charset, text, how]] of CASES.entries()) {
      const [verdict, detail] = await classify(exe, tmp, port, routes, index, codec, charset, text, how);
      const note = {
        decoded: 'decodes correctly',
        undecoded: 'declaration ignored, renders as mojibake',
        neither: 'matches NEITHER control',
        norender: `the ${detail} render produced no PNG`,
      }[verdict];
      console.log(`  ${verdict.toUpperCase().padEnd(9)} ${label.padEnd(26)} ${note}`);
      if (verdict !== 'decoded') failures.push(`${label}: ${note}`);
    }
  } finally {
    server.close();
    rmSync(tmp, {recursive: true, force: true});
  }

  console.log();
  if (failures.length) {
    for (const f of failures) console.log(`FAIL  ${f}`);
    return 1;
  }
  console.log(`ALL ${CASES.length} ENCODING CASES DECODED`);
  return 0;
}

const cli = cac('pnpm verify:charset');
cli.command('<exe>', 'check that a document is decoded with the encoding it declares')
    .action(async (exe: string) => {
      process.exitCode = await main(exe);
    });
cli.help();
cli.parse();
