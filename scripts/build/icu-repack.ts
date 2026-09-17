// Rebuild an ICU .dat package with some of its items removed.
//
// third_party/icu ships nine prebuilt data sets and shot uses the smallest one
// that still carries what a renderer needs (`cast`). None of the nine is the
// set shot actually wants: `cast` keeps 1.95 MB of locale, region, currency
// and timezone display names and collation tables that nothing in this build
// ever opens, and the sets that drop those (`flutter`, `flutter_desktop`) also
// drop the legacy converters, which do get used.
//
// Rebuilding the data properly means running ICU's own data build, which
// wants a POSIX toolchain and a source build of genrb. Removing items from a
// finished package needs neither: the container is a table of contents of
// (name, offset) pairs followed by the items, and dropping an entry is a
// matter of rewriting the table.
//
// An item that is removed but then looked up makes the ICU call return
// U_MISSING_RESOURCE_ERROR. That is a runtime failure, not a build one, so
// every name dropped here has to be justified at its call site rather than by
// inspection of the package.
//
//   pnpm icu:repack IN.dat OUT.dat [--preset shot] [--drop-prefix P]...
//                                  [--drop-name N]... [--list] [--verify]
//
// Names are matched against the item name with the package prefix removed,
// so "zone/" and "gb18030.cnv" rather than "icudt78l/zone/root.res".
//
// --preset shot is the list the build uses. Keeping it here rather than in
// the GN action or the CI workflow is what keeps a local build and a runner
// build producing the same table. build-engine calls repack() in-process.
// Relative paths are resolved against the repository root.

import {readFileSync, writeFileSync} from 'node:fs';

import {cac} from 'cac';

import {resolve} from '../lib/repo.ts';

interface Item {
  full: string;
  short: string;
  payload: Buffer;
}

// icupkg fills alignment gaps with 0xaa rather than zeroes. Nothing reads the
// filler, but matching it lets --verify prove the round-trip is byte-exact.
const FILLER = 0xaa;
const align16 = (n: number) => (n + 15) & ~15;

export class Package {
  readonly path: string;
  readonly buf: Buffer;
  readonly header: Buffer;
  readonly items: Item[] = [];

  constructor(path: string) {
    this.path = path;
    this.buf = readFileSync(path);
    const headerSize = this.buf.readUInt16LE(0);
    if (this.buf[2] !== 0xda || this.buf[3] !== 0x27) {
      throw new Error(`${path}: not an ICU data file (magic ${this.buf[2].toString(16).padStart(2, '0')} ${this.buf[3].toString(16).padStart(2, '0')})`);
    }
    // UDataInfo starts at 4: size(2) reservedWord(2) isBigEndian(1) ...
    if (this.buf[8]) throw new Error(`${path}: big-endian packages are not supported`);
    this.header = this.buf.subarray(0, headerSize);
    const base = headerSize;
    const count = this.buf.readUInt32LE(base);
    const toc: Array<[number, number]> = [];
    for (let i = 0; i < count; i++) toc.push([this.buf.readUInt32LE(base + 4 + 8 * i), this.buf.readUInt32LE(base + 8 + 8 * i)]);
    for (let i = 0; i < count; i++) {
      const [nameOff, dataOff] = toc[i];
      const end = i + 1 < count ? toc[i + 1][1] : this.buf.length - base;
      const start = base + nameOff;
      const full = this.buf.subarray(start, this.buf.indexOf(0, start)).toString('ascii');
      const short = full.includes('/') ? full.slice(full.indexOf('/') + 1) : full;
      this.items.push({full, short, payload: this.buf.subarray(base + dataOff, base + end)});
    }
  }

  // Items must stay sorted by name: ICU binary-searches the table.
  write(path: string, items: Item[]): number {
    const sorted = [...items].sort((a, b) => Buffer.compare(Buffer.from(a.full, 'ascii'), Buffer.from(b.full, 'ascii')));
    const tocSize = 4 + 8 * sorted.length;
    const nameChunks: Buffer[] = [];
    const nameOffsets: number[] = [];
    let namesLength = 0;
    for (const it of sorted) {
      nameOffsets.push(tocSize + namesLength);
      const chunk = Buffer.from(it.full + '\0', 'ascii');
      nameChunks.push(chunk);
      namesLength += chunk.length;
    }
    const dataStart = align16(tocSize + namesLength);
    // Each payload already carries the input's own trailing padding, so this
    // only re-aligns if an input ever failed to.
    const bodyChunks: Buffer[] = [];
    const dataOffsets: number[] = [];
    let bodyLength = 0;
    for (const it of sorted) {
      const pad = align16(bodyLength) - bodyLength;
      if (pad) {
        bodyChunks.push(Buffer.alloc(pad, FILLER));
        bodyLength += pad;
      }
      dataOffsets.push(dataStart + bodyLength);
      bodyChunks.push(it.payload);
      bodyLength += it.payload.length;
    }
    const toc = Buffer.alloc(tocSize);
    toc.writeUInt32LE(sorted.length, 0);
    for (let i = 0; i < sorted.length; i++) {
      toc.writeUInt32LE(nameOffsets[i], 4 + 8 * i);
      toc.writeUInt32LE(dataOffsets[i], 8 + 8 * i);
    }
    const out = Buffer.concat([
      this.header, toc, ...nameChunks,
      Buffer.alloc(dataStart - (tocSize + namesLength), FILLER),
      ...bodyChunks,
    ]);
    writeFileSync(path, out);
    return out.length;
  }
}

// Every converter table stays. The 27 single-byte ones (ISO-8859-2..16,
// windows-1250..1258, KOI8, IBM866, macintosh; 78 KB together) are what
// TextCodecIcu decodes with. The six CJK ones (big5-html, euc-jp-html,
// euc-kr-html, gb18030, shift_jis-html, windows-936-2000; 853 KB) look unused
// -- text_codec_icu.cc's ShouldSkipEncoding() yields every name that
// TextCodecCjk::IsSupported() claims -- but TextCodecCjk is built on them:
// wtf/text/encoding_tables.cc fills its index tables at first use with
// ucnv_open("EUC-JP"), ucnv_open("windows-949"), ucnv_open("gb18030") and
// ucnv_open("big5-html"). In a release build a missing converter there is not
// a U+FFFD: the DCHECK on ucnv_open is compiled out, ucnv_toUnicode on a null
// converter writes nothing, and the loop copies an uninitialised UChar into a
// fixed-size array past its end. Dropping them was survivable only while
// ForceSynchronousDocumentInstall hardcoded UTF-8 and no legacy charset ever
// reached a decoder; since the Content-Type charset, the BOM and <meta
// charset> are honoured (scripts/verify/charset.ts proves it), the tables are
// load-bearing. The smaller alternative -- compiling the WHATWG index tables
// into Blink, about 210 KB -- needs a generator the tree no longer carries
// the .ucm sources for.

// Five resource trees, 1.95 MB together, that no code path in shotium opens.
// Each is reached through exactly one ICU service, and the final-link section
// graph of shotium.exe (out/size-analysis, 2026-09-18) shows that service has
// no live caller; the same holds for Linux and macOS, whose Blink uses
// LocaleICU (udat/unum against the `<locale>.res` bundles kept here).
//
//   zone/    time zone display names: TimeZoneNames, used by SimpleDateFormat
//            for the z/v/V pattern fields only. Blink's date/time form
//            controls format with short and medium patterns, which have no
//            zone field, and blink::ConvertToLocalTime, the last caller of
//            TimeZone::createDefault, now asks base::Time for the offset.
//            Zone rules (zoneinfo64.res, metaZones, timezoneTypes) stay in
//            the package.
//   lang/    language, script and variant display names: uldn_* and
//            Locale::getDisplayName. No caller outside ICU's own service
//            registry, whose getDisplayName is never invoked.
//   region/  country display names, same consumer.
//   curr/    currency names and symbols: ucurr_* and NumberFormat currency
//            styles. No caller.
//   coll/    root collation tables: ucol_open / usearch_open, whose only
//            reachable user was <select> keyboard type-ahead
//            (StringImpl::StartsWithIgnoringCaseAndAccents), removed with it.
//
// A dropped item that is looked up after all returns U_MISSING_RESOURCE_ERROR
// at run time, so this list changes when one of those callers comes back.
const UNOPENED_TREES = ['zone/', 'lang/', 'region/', 'curr/', 'coll/'];

export const PRESETS: Record<string, {names: string[]; prefixes: string[]}> = {
  shot: {names: [], prefixes: UNOPENED_TREES},
};

export interface RepackResult {
  items: number;
  bytes: number;
  kept: number;
  size: number;
  dropped: number;
  freed: number;
}

// Writes `input` minus the named items to `output`; throws on a name that
// matches nothing, because a typo there would leave the item in the package
// while the caller believes it is gone.
export function repack(input: string, output: string, dropNames: string[], dropPrefixes: string[]): RepackResult {
  const pkg = new Package(input);
  const kept: Item[] = [], dropped: Item[] = [];
  for (const item of pkg.items) {
    if (dropNames.includes(item.short) || dropPrefixes.some((p) => item.short.startsWith(p))) dropped.push(item);
    else kept.push(item);
  }
  const unmatchedNames = dropNames.filter((n) => !pkg.items.some((it) => it.short === n));
  if (unmatchedNames.length) throw new Error(`these --drop-name values matched no item: ${unmatchedNames.join(', ')}`);
  const unmatchedPrefixes = dropPrefixes.filter((p) => !pkg.items.some((it) => it.short.startsWith(p)));
  if (unmatchedPrefixes.length) throw new Error(`these --drop-prefix values matched no item: ${unmatchedPrefixes.join(', ')}`);
  const size = pkg.write(output, kept);
  return {items: pkg.items.length, bytes: pkg.buf.length, kept: kept.length, size, dropped: dropped.length, freed: pkg.buf.length - size};
}

function main(inputArg: string, outputArg: string | undefined, opts: {preset?: string; dropPrefix: string[]; dropName: string[]; list?: boolean; verify?: boolean}): number {
  const input = resolve(inputArg);
  const output = outputArg ? resolve(outputArg) : undefined;
  if (opts.list) {
    const pkg = new Package(input);
    for (const it of [...pkg.items].sort((a, b) => b.payload.length - a.payload.length)) {
      console.log(`${String(it.payload.length).padStart(9)}  ${it.short}`);
    }
    return 0;
  }
  if (opts.verify) {
    if (!output) throw new Error('--verify needs an output path to write to');
    const pkg = new Package(input);
    pkg.write(output, pkg.items);
    if (!readFileSync(output).equals(pkg.buf)) {
      console.log('VERIFY FAILED: round-trip is not byte-identical');
      return 1;
    }
    console.log(`verify ok: ${pkg.items.length} items round-trip byte-identically`);
    return 0;
  }
  if (!output) throw new Error('an output path is required');
  let names = [...opts.dropName], prefixes = [...opts.dropPrefix];
  if (opts.preset) {
    const preset = PRESETS[opts.preset];
    if (!preset) throw new Error(`--preset must be one of ${Object.keys(PRESETS).sort().join(', ')}`);
    names = [...names, ...preset.names];
    prefixes = [...prefixes, ...preset.prefixes];
  }
  if (!names.length && !prefixes.length) throw new Error('nothing to drop: pass --preset, --drop-name or --drop-prefix');
  const r = repack(input, output, names, prefixes);
  console.log(`${inputArg} -> ${outputArg}`);
  console.log(`  ${r.items} items (${r.bytes} bytes) -> ${r.kept} items (${r.size} bytes)`);
  console.log(`  dropped ${r.dropped} items, ${r.freed} bytes (${(r.freed / 1048576).toFixed(2)} MB)`);
  return 0;
}

if (process.argv[1] && resolve(process.argv[1]) === import.meta.filename) {
  const cli = cac('pnpm icu:repack');
  cli.command('<input> [output]', 'rebuild an ICU .dat package with some items removed')
      .option('--preset <name>', 'apply a named drop list documented in this file')
      .option('--drop-prefix <prefix>', 'drop every item whose short name starts with this; repeatable')
      .option('--drop-name <name>', 'drop the item with this short name; repeatable')
      .option('--list', 'print every item and its size, then exit')
      .option('--verify', 'repack with nothing dropped and require the result to be byte-identical to the input')
      .action((input: string, output: string | undefined, options: {preset?: string; dropPrefix?: string | string[]; dropName?: string | string[]; list?: boolean; verify?: boolean}) => {
        const list = (v: string | string[] | undefined) => v === undefined ? [] : Array.isArray(v) ? v : [v];
        try {
          process.exitCode = main(input, output, {...options, dropPrefix: list(options.dropPrefix), dropName: list(options.dropName)});
        } catch (error) {
          console.error(error instanceof Error ? error.message : String(error));
          process.exitCode = 2;
        }
      });
  cli.help();
  cli.parse();
}
