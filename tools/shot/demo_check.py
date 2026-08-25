#!/usr/bin/env python3
"""Run the reftest suite in shot/testdata/demos.

Every test is a pair: NAME.html exercises a feature, NAME-ref.html produces the
same pixels using only absolutely positioned blocks with a background-color, or
the same text through a path the feature under test does not touch. If the two
renders are not byte-identical, the feature is broken. There are no golden
images, so nothing here goes stale when the renderer legitimately changes.

That matters for this tree specifically. Cutting code out of Blink does not
usually produce a build error when it goes wrong -- it produces a page that
lays out slightly differently, and a suite that compares against stored images
would either have to be re-blessed after every cut (hiding regressions) or
would drown in diffs. A reftest states the expected result in CSS the cut
cannot plausibly break, so it keeps meaning the same thing.

A page with no -ref.html is a smoke test instead: it must render, produce more
than one distinct colour, and produce identical bytes on a second run. That is
for features whose output cannot be restated exactly -- blurs, shadows,
anything with antialiasing -- where the useful question is only whether it
still runs and still runs deterministically.

A test may allow a bounded difference the way WPT does, by declaring

    <meta name="fuzzy" content="maxDifference=1;totalPixels=0-5000">

Use it only where the difference is inherent rather than suspicious: two
correct code paths that round in different places, for instance. The allowance
is stated in the test file so it sits next to the reason for it, and both
bounds are upper limits -- a render that differs by less still passes.

Usage:
    python tools/shot/demo_check.py out/ShotWip/shotium.exe [--filter SUBSTRING]
                                    [--jobs N] [--out DIR]
"""

import argparse
import concurrent.futures
import io
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEMOS = os.path.join(ROOT, 'shot', 'testdata', 'demos')

# The demos are authored against this viewport. Both halves of a pair get it,
# so a mismatch is never the harness's doing.
WIDTH, HEIGHT = 400, 200


def render(exe, html, png, extra=()):
    proc = subprocess.run(
        [exe, '--file', html, '--width', str(WIDTH), '--height', str(HEIGHT),
         '--output', png, *extra],
        capture_output=True, text=True)
    if not os.path.exists(png):
        tail = (proc.stderr or proc.stdout or '').strip().splitlines()
        return None, (tail[-1] if tail else 'no output and no message')
    return open(png, 'rb').read(), None


FUZZY_RE = re.compile(
    r'<meta\s+name=["\']?fuzzy["\']?\s+content=["\']([^"\']+)["\']', re.I)


def read_fuzzy(html_path):
    """Return (max_channel_delta, max_differing_pixels) allowed by the test."""
    text = io.open(html_path, encoding='utf-8', errors='replace').read(4096)
    m = FUZZY_RE.search(text)
    if not m:
        return 0, 0
    delta, pixels = 0, 0
    for part in m.group(1).split(';'):
        key, _, value = part.strip().partition('=')
        hi = value.split('-')[-1]
        if key.strip() == 'maxDifference':
            delta = int(hi)
        elif key.strip() == 'totalPixels':
            pixels = int(hi)
    return delta, pixels


def measure(a_png, b_png):
    """(differing pixels, worst channel delta, bbox, total pixels), or None."""
    try:
        from PIL import Image, ImageChops
    except ImportError:
        return None
    a = Image.open(io.BytesIO(a_png)).convert('RGB')
    b = Image.open(io.BytesIO(b_png)).convert('RGB')
    if a.size != b.size:
        return ('size', a.size, b.size)
    diff = ImageChops.difference(a, b)
    hist = diff.convert('L').histogram()
    moved = sum(hist[1:])
    worst = max(i for i, n in enumerate(hist) if n)
    return moved, worst, diff.getbbox(), a.size[0] * a.size[1]


def describe_difference(a_png, b_png):
    m = measure(a_png, b_png)
    if m is None:
        return 'differs (install pillow for a pixel breakdown)'
    if m[0] == 'size':
        return 'different sizes: %dx%d vs %dx%d' % (m[1] + m[2])
    moved, worst, bbox, total = m
    return ('%d px differ (%.3f%%), worst channel delta %d, bbox %s'
            % (moved, 100.0 * moved / total, worst, bbox))


def distinct_colors(png_bytes, cap=64):
    try:
        from PIL import Image
    except ImportError:
        return None
    img = Image.open(io.BytesIO(png_bytes)).convert('RGB')
    colors = img.getcolors(maxcolors=cap)
    return len(colors) if colors is not None else cap


def run_one(exe, tmp, name):
    test = os.path.join(DEMOS, name + '.html')
    ref = os.path.join(DEMOS, name + '-ref.html')
    got, err = render(exe, test, os.path.join(tmp, name + '.png'))
    if got is None:
        return name, 'FAIL', 'did not render: %s' % err

    if os.path.exists(ref):
        want, err = render(exe, ref, os.path.join(tmp, name + '-ref.png'))
        if want is None:
            return name, 'ERROR', 'the reference did not render: %s' % err
        if got == want:
            return name, 'PASS', 'matches its reference'

        max_delta, max_pixels = read_fuzzy(test)
        if max_delta or max_pixels:
            m = measure(want, got)
            if m is not None and m[0] != 'size':
                moved, worst, _bbox, _total = m
                if worst <= max_delta and moved <= max_pixels:
                    return name, 'FUZZY', (
                        'within the declared allowance: %d px differ by at '
                        'most %d (allowed %d px, %d)'
                        % (moved, worst, max_pixels, max_delta))
        return name, 'FAIL', describe_difference(want, got)

    # Smoke test.
    n = distinct_colors(got)
    if n is not None and n < 2:
        return name, 'FAIL', 'rendered a single flat colour'
    again, err = render(exe, test, os.path.join(tmp, name + '.2.png'))
    if again != got:
        # Say how they differ, not just that they do. The two answers look
        # nothing alike and lead in opposite directions: a handful of pixels
        # off by one or two is text rasterising against a font cache that was
        # cold for the first render, while a large or structural difference is
        # the engine itself being non-deterministic. Reporting only "differ"
        # once cost a CI round that could not distinguish them.
        return name, 'FAIL', 'two renders of the same page differ: %s' % (
            describe_difference(got, again))
    detail = 'renders, deterministic'
    if n is not None:
        detail += ', %s%d colours' % ('>=' if n >= 64 else '', n)
    return name, 'SMOKE', detail


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument('exe')
    ap.add_argument('--filter', default='')
    ap.add_argument('--jobs', type=int, default=8)
    ap.add_argument('--out', help='keep the PNGs in this directory')
    args = ap.parse_args(argv)

    exe = os.path.abspath(args.exe)
    if not os.path.exists(exe):
        print('no such binary: %s' % exe)
        return 2
    if not os.path.isdir(DEMOS):
        print('no demos directory at %s' % DEMOS)
        return 2

    names = sorted(f[:-5] for f in os.listdir(DEMOS)
                   if f.endswith('.html') and not f.endswith('-ref.html'))
    if args.filter:
        names = [n for n in names if args.filter in n]
    if not names:
        print('no demos matched')
        return 2

    tmpdir = args.out or tempfile.mkdtemp(prefix='shot-demos-')
    os.makedirs(tmpdir, exist_ok=True)

    # The first render on a cold system font cache can differ from every later
    # one, which reads as a determinism failure on whichever text-heavy demo
    # happens to run first. Warm the cache once, on a page outside the suite,
    # so that cost lands somewhere it cannot be mistaken for a result.
    warm = os.path.join(tmpdir, '_warmup.html')
    io.open(warm, 'w', encoding='utf-8', newline='\n').write(
        '<!doctype html><meta charset="utf-8">'
        '<div style="font:16px serif">Aa Bb 0123 漢字 עב</div>'
        '<div style="font:16px sans-serif">Aa Bb</div>'
        '<div style="font:16px monospace">Aa Bb</div>\n')
    render(exe, warm, os.path.join(tmpdir, '_warmup.png'))

    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = [pool.submit(run_one, exe, tmpdir, n) for n in names]
        for f in concurrent.futures.as_completed(futures):
            results.append(f.result())

    order = {'FAIL': 0, 'ERROR': 1, 'FUZZY': 2, 'SMOKE': 3, 'PASS': 4}
    results.sort(key=lambda r: (order[r[1]], r[0]))
    for name, verdict, detail in results:
        print('  %-6s %-22s %s' % (verdict, name, detail))

    counts = {}
    for _, verdict, _ in results:
        counts[verdict] = counts.get(verdict, 0) + 1
    print()
    print('  '.join('%s %d' % (k, counts[k]) for k in sorted(counts)))
    bad = counts.get('FAIL', 0) + counts.get('ERROR', 0)
    if bad:
        print('\n%d of %d demos FAILED' % (bad, len(results)))
        if args.out:
            print('renders kept in %s' % tmpdir)
        return 1
    print('\nALL %d DEMOS PASSED' % len(results))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
