#!/usr/bin/env python3
"""Check that removing ICU's CJK converter tables changed no encoding.

tools/shot/icu_repack.py drops six converter tables from the ICU data on the
grounds that Blink decodes those encodings itself in TextCodecCjk and never
asks ICU for them. That is a claim about two files agreeing (ShouldSkipEncoding
in text_codec_icu.cc and kSupportedCanonicalNames in text_codec_cjk.cc), so it
deserves a test that reads neither.

Each encoding is rendered three ways and the result is classified:

  decoded     the legacy bytes render the same pixels as the same text in UTF-8
  undecoded   they render the same pixels as those bytes read *as* UTF-8, i.e.
              the charset declaration was ignored and the text is mojibake
  neither     something else entirely, which is what a broken converter looks
              like

No golden images: the page is its own oracle in every case.

*** What this build does today ***

Every encoding except pure-ASCII cases comes out `undecoded`, and that is not
this cut's doing -- it reproduces with the untrimmed `cast` data. The cause is
LocalFrame::ForceSynchronousDocumentInstall, which shot uses to install a
document without a DocumentLoader:

    DocumentParser* parser = document->OpenForNavigation(
        kForceSynchronousParsing, mime_type, AtomicString("UTF-8"));

The encoding is hardcoded. An explicit encoding outranks `<meta charset>` in
TextResourceDecoder, so no declaration a page makes is ever consulted, and
shot_renderer.cc's SetDefaultTextEncodingName cannot reach it either. Whoever
fixes that should re-run this, because it will move most cases to `decoded` and
put the ICU converter tables genuinely in use for the first time.

The invariant this enforces is therefore not "everything decodes" -- it is that
the CJK encodings, whose ICU tables were removed, behave exactly like the
single-byte ones, whose tables were kept. If the two groups ever disagree, the
data cut is the reason.

Usage:
    python tools/shot/charset_check.py out/ShotWip/shotium.exe
"""

import os
import subprocess
import sys
import tempfile

# (label, python codec, charset name for <meta>, sample text, group)
#
# "cjk" are the encodings whose ICU tables icu_repack.py removes; Blink decodes
# them in TextCodecCjk. "single" are the ones whose ICU tables stay.
CASES = [
    ('shift_jis', 'shift_jis', 'shift_jis', 'ひらがなカタカナ漢字', 'cjk'),
    ('euc-jp', 'euc_jp', 'euc-jp', 'ひらがなカタカナ漢字', 'cjk'),
    ('gbk', 'gbk', 'gbk', '简体中文测试文本', 'cjk'),
    ('gb18030', 'gb18030', 'gb18030', '简体中文测试文本', 'cjk'),
    ('big5', 'big5', 'big5', '繁體中文測試文字', 'cjk'),
    ('euc-kr', 'euc_kr', 'euc-kr', '한국어시험문장', 'cjk'),
    ('windows-1251', 'cp1251', 'windows-1251', 'Проверка кириллицы', 'single'),
    ('iso-8859-2', 'iso8859_2', 'iso-8859-2', 'Zażółć gęślą jaźń', 'single'),
    ('iso-8859-7', 'iso8859_7', 'iso-8859-7', 'Ελληνικό κείμενο', 'single'),
    ('koi8-r', 'koi8_r', 'koi8-r', 'Проверка кодировки', 'single'),
    ('windows-1256', 'cp1256', 'windows-1256', 'نص عربي', 'single'),
]

# No font-family and no web fonts: whatever the system picks it picks the same
# way for all three renders, and 40px is large enough that a wrong codepoint
# cannot land on the same pixels as a right one.
PAGE = ('<!doctype html><html><head><meta charset="{charset}">'
        '<style>body{{margin:0;background:#fff;font-size:40px;'
        'line-height:1.4}}</style></head><body>{text}</body></html>')


def render(exe, html_path, png_path):
    subprocess.run(
        [exe, '--file', html_path, '--width', '600', '--height', '160',
         '--output', png_path],
        capture_output=True, text=True)
    if not os.path.exists(png_path):
        return None
    return open(png_path, 'rb').read()


def classify(exe, tmp, label, codec, charset, text):
    legacy_bytes = PAGE.format(charset=charset, text=text).encode(codec)

    paths = {}
    paths['legacy'] = os.path.join(tmp, label + '.html')
    with open(paths['legacy'], 'wb') as f:
        f.write(legacy_bytes)

    # The control for "decoded correctly".
    paths['utf8'] = os.path.join(tmp, label + '.utf8.html')
    with open(paths['utf8'], 'wb') as f:
        f.write(PAGE.format(charset='utf-8', text=text).encode('utf-8'))

    # The control for "charset ignored": the same bytes read as UTF-8.
    paths['mojibake'] = os.path.join(tmp, label + '.moji.html')
    with open(paths['mojibake'], 'wb') as f:
        mojibake = legacy_bytes.decode('utf-8', errors='replace')
        f.write(mojibake.replace(charset, 'utf-8', 1).encode('utf-8'))

    images = {}
    for kind, path in paths.items():
        images[kind] = render(exe, path, os.path.join(tmp, label + kind + '.png'))
        if images[kind] is None:
            return 'norender', kind

    if images['legacy'] == images['utf8']:
        return 'decoded', None
    if images['legacy'] == images['mojibake']:
        return 'undecoded', None
    return 'neither', None


def main(argv):
    if len(argv) != 1:
        print(__doc__)
        return 2
    exe = os.path.abspath(argv[0])
    if not os.path.exists(exe):
        print('no such binary: %s' % exe)
        return 2

    results = {}
    with tempfile.TemporaryDirectory(prefix='shot-charset-') as tmp:
        for label, codec, charset, text, group in CASES:
            verdict, detail = classify(exe, tmp, label, codec, charset, text)
            results[label] = (verdict, group)
            note = {
                'decoded': 'decodes correctly',
                'undecoded': 'charset ignored, renders as mojibake',
                'neither': 'matches NEITHER control',
                'norender': 'the %s render produced no PNG' % detail,
            }[verdict]
            print('  %-9s %-14s %-7s %s'
                  % (verdict.upper(), label, group, note))

    print()
    failures = []

    broken = [k for k, (v, _) in results.items() if v in ('neither', 'norender')]
    if broken:
        failures.append('these matched neither control: %s' % ', '.join(broken))

    # The invariant: the group whose ICU tables were removed must behave the
    # same as the group whose tables were kept. Either both decode or neither
    # does; a split means the data cut caused it.
    verdicts = {}
    for group in ('cjk', 'single'):
        got = {v for k, (v, g) in results.items() if g == group}
        verdicts[group] = got
        if len(got) > 1:
            failures.append('the %s encodings disagree with each other: %s'
                            % (group, ', '.join(sorted(got))))
    if (len(verdicts['cjk']) == 1 and len(verdicts['single']) == 1 and
            verdicts['cjk'] != verdicts['single']):
        failures.append(
            'the CJK encodings are %s but the single-byte ones are %s; the '
            'removed ICU converter tables are the difference between them'
            % (verdicts['cjk'].pop(), verdicts['single'].pop()))

    if failures:
        for f in failures:
            print('FAIL: %s' % f)
        return 1

    state = results[CASES[0][0]][0]
    if state == 'undecoded':
        print('ALL ENCODINGS CONSISTENT (all undecoded -- see this file\'s '
              'header: ForceSynchronousDocumentInstall hardcodes UTF-8)')
    else:
        print('ALL ENCODINGS CONSISTENT (%s)' % state)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
