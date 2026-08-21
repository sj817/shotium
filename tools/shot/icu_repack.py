#!/usr/bin/env python3
"""Rebuild an ICU .dat package with some of its items removed.

third_party/icu ships nine prebuilt data sets and shot uses the smallest one
that still carries what a renderer needs (`cast`). None of the nine is the set
shot actually wants: `cast` keeps 1.9 MB of locale display names, timezone
bundles and CJK converter tables that nothing in this build ever opens, and the
sets that drop those (`flutter`, `flutter_desktop`) also drop the single-byte
legacy converters, which are 78 KB and do get used.

Rebuilding the data properly means running ICU's own data build, which wants a
POSIX toolchain and a source build of genrb. Removing items from a finished
package needs neither: the container is a table of contents of
(name, offset) pairs followed by the items, and dropping an entry is a matter
of rewriting the table.

An item that is removed but then looked up makes the ICU call return
U_MISSING_RESOURCE_ERROR. That is a runtime failure, not a build one, so every
name dropped here has to be justified at its call site rather than by
inspection of the package.

Usage:
  icu_repack.py IN.dat OUT.dat [--preset shot] [--drop-prefix P]...
                               [--drop-name N]... [--list] [--verify]

Names are matched against the item name with the package prefix removed, so
"zone/" and "gb18030.cnv" rather than "icudt78l/zone/root.res".

--preset shot is the list the build uses. Keeping it here rather than in the
GN action or the CI workflow is what keeps a local build and a runner build
producing the same table.
"""

import argparse
import struct
import sys


class Package:
    def __init__(self, path):
        self.path = path
        self.buf = open(path, 'rb').read()

        header_size, = struct.unpack_from('<H', self.buf, 0)
        if (self.buf[2], self.buf[3]) != (0xda, 0x27):
            raise SystemExit('%s: not an ICU data file (magic %02x %02x)' %
                             (path, self.buf[2], self.buf[3]))
        # UDataInfo starts at 4: size(2) reservedWord(2) isBigEndian(1) ...
        if self.buf[8]:
            raise SystemExit('%s: big-endian packages are not supported' % path)

        self.header = self.buf[:header_size]
        self.base = header_size

        count, = struct.unpack_from('<I', self.buf, self.base)
        toc = [struct.unpack_from('<II', self.buf, self.base + 4 + 8 * i)
               for i in range(count)]

        self.items = []  # (full_name, short_name, payload)
        for i, (name_off, data_off) in enumerate(toc):
            end = toc[i + 1][1] if i + 1 < count else len(self.buf) - self.base
            start = self.base + name_off
            full = self.buf[start:self.buf.index(b'\0', start)].decode('ascii')
            short = full.split('/', 1)[1] if '/' in full else full
            payload = self.buf[self.base + data_off:self.base + end]
            self.items.append((full, short, payload))

    def write(self, path, items):
        # Items must stay sorted by name: ICU binary-searches the table.
        items = sorted(items, key=lambda it: it[0].encode('ascii'))

        names = bytearray()
        name_offsets = []
        toc_size = 4 + 8 * len(items)
        for full, _short, _payload in items:
            name_offsets.append(toc_size + len(names))
            names += full.encode('ascii') + b'\0'

        data_start = _align16(toc_size + len(names))

        # Each payload already carries the input's own trailing padding, so
        # this only re-aligns if an input ever failed to.
        body = bytearray()
        data_offsets = []
        for _full, _short, payload in items:
            body += _FILLER * (_align16(len(body)) - len(body))
            data_offsets.append(data_start + len(body))
            body += payload

        out = bytearray(self.header)
        out += struct.pack('<I', len(items))
        for name_off, data_off in zip(name_offsets, data_offsets):
            out += struct.pack('<II', name_off, data_off)
        out += names
        out += _FILLER * (data_start - (toc_size + len(names)))
        out += body

        open(path, 'wb').write(bytes(out))
        return len(out)


# icupkg fills alignment gaps with 0xaa rather than zeroes. Nothing reads the
# filler, but matching it lets --verify prove the round-trip is byte-exact.
_FILLER = b'\xaa'

# The six ICU converter tables for the encodings Blink decodes itself.
#
# TextCodecIcu enumerates ICU's converters at startup and registers what it
# finds, but text_codec_icu.cc's ShouldSkipEncoding() drops every name that
# TextCodecCjk::IsSupported() claims first, and text_codec_cjk.cc's
# kSupportedCanonicalNames is exactly EUC-JP, Shift_JIS, EUC-KR, ISO-2022-JP,
# GBK, gb18030, Big5 and Big5-HKSCS. So these tables are loaded by nothing:
# Blink has its own decoder for each of them.
#
# They are 853 KB of the 932 KB of converters in the `cast` data set. The 27
# single-byte tables that remain -- ISO-8859-2..16, windows-1250..1258, KOI8,
# IBM866, macintosh -- are 78 KB together and are the ones TextCodecIcu really
# does serve, which is why swapping to a filter that drops conversion_mappings
# wholesale (`flutter_desktop`) is the wrong trade.
_CJK_CONVERTERS = [
    'big5-html.cnv',
    'euc-jp-html.cnv',
    'euc-kr-html.cnv',
    'gb18030.cnv',
    'shift_jis-html.cnv',
    'windows-936-2000.cnv',
]

PRESETS = {
    'shot': {
        'names': _CJK_CONVERTERS,
        'prefixes': [],
    },
}


def _align16(n):
    return (n + 15) & ~15


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument('input')
    ap.add_argument('output', nargs='?')
    ap.add_argument('--preset', choices=sorted(PRESETS),
                    help='apply a named drop list documented in this file')
    ap.add_argument('--drop-prefix', action='append', default=[])
    ap.add_argument('--drop-name', action='append', default=[])
    ap.add_argument('--list', action='store_true',
                    help='print every item and its size, then exit')
    ap.add_argument('--verify', action='store_true',
                    help='repack with nothing dropped and require the result '
                         'to be byte-identical to the input')
    args = ap.parse_args(argv)

    pkg = Package(args.input)

    if args.list:
        for _full, short, payload in sorted(pkg.items,
                                            key=lambda it: -len(it[2])):
            print('%9d  %s' % (len(payload), short))
        return 0

    if args.verify:
        if not args.output:
            ap.error('--verify needs an output path to write to')
        pkg.write(args.output, pkg.items)
        if open(args.output, 'rb').read() != pkg.buf:
            print('VERIFY FAILED: round-trip is not byte-identical')
            return 1
        print('verify ok: %d items round-trip byte-identically' %
              len(pkg.items))
        return 0

    if not args.output:
        ap.error('an output path is required')

    if args.preset:
        preset = PRESETS[args.preset]
        args.drop_name = args.drop_name + preset['names']
        args.drop_prefix = args.drop_prefix + preset['prefixes']
    if not args.drop_name and not args.drop_prefix:
        ap.error('nothing to drop: pass --preset, --drop-name or --drop-prefix')

    kept, dropped = [], []
    for item in pkg.items:
        short = item[1]
        if (short in args.drop_name or
                any(short.startswith(p) for p in args.drop_prefix)):
            dropped.append(item)
        else:
            kept.append(item)

    unmatched = [n for n in args.drop_name
                 if not any(it[1] == n for it in pkg.items)]
    if unmatched:
        # A name that matches nothing is a typo, not a no-op: it would leave
        # the item in the package while the caller believes it is gone.
        raise SystemExit('these --drop-name values matched no item: %s' %
                         ', '.join(unmatched))
    unmatched = [p for p in args.drop_prefix
                 if not any(it[1].startswith(p) for it in pkg.items)]
    if unmatched:
        raise SystemExit('these --drop-prefix values matched no item: %s' %
                         ', '.join(unmatched))

    size = pkg.write(args.output, kept)
    freed = len(pkg.buf) - size
    print('%s -> %s' % (args.input, args.output))
    print('  %d items (%d bytes) -> %d items (%d bytes)' %
          (len(pkg.items), len(pkg.buf), len(kept), size))
    print('  dropped %d items, %d bytes (%.2f MB)' %
          (len(dropped), freed, freed / 1048576.0))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
