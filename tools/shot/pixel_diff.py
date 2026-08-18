"""Compare two PNGs and report how far apart they are.

The acceptance criterion for this tree is not "the images match" -- a stripped
renderer is allowed to differ -- it is that every difference is measured and
explained. So this reports numbers that distinguish *kinds* of difference
rather than a single pass/fail:

  max channel delta      one antialiasing seam and a missing element both show
                         255 here, so this alone says very little
  differing pixels       how much of the image moved at all
  pixels over threshold  how much of it moved *visibly* (default 8/255, below
                         which a difference is not perceptible on screen)
  mean delta             separates "everything shifted slightly", which is a
                         colour-space or gamma difference, from "a few regions
                         are completely wrong", which is a missing feature
  bounding box           where the damage is; a box around one element points
                         at that element's code path

A difference image is written alongside, amplified so that small deltas are
actually visible.

Usage:
  pixel_diff.py <a.png> <b.png> [--out diff.png] [--threshold 8]
"""

import sys

from PIL import Image, ImageChops


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if len(args) < 2:
        sys.exit(__doc__)
    threshold = 8
    if "--threshold" in sys.argv:
        threshold = int(sys.argv[sys.argv.index("--threshold") + 1])
    out_path = "diff.png"
    if "--out" in sys.argv:
        out_path = sys.argv[sys.argv.index("--out") + 1]

    a = Image.open(args[0]).convert("RGB")
    b = Image.open(args[1]).convert("RGB")

    print("a: %s  %dx%d" % (args[0], a.width, a.height))
    print("b: %s  %dx%d" % (args[1], b.width, b.height))
    if a.size != b.size:
        # Comparing after a resize would invent differences that are artifacts
        # of the resampling, so crop both to the overlap and say so.
        w = min(a.width, b.width)
        h = min(a.height, b.height)
        print("SIZE MISMATCH -- comparing the common %dx%d region only" % (w, h))
        a = a.crop((0, 0, w, h))
        b = b.crop((0, 0, w, h))

    diff = ImageChops.difference(a, b)
    total = a.width * a.height

    # Per-pixel maximum across channels.
    bands = diff.split()
    worst = bands[0]
    for band in bands[1:]:
        worst = ImageChops.lighter(worst, band)

    histogram = worst.histogram()
    differing = sum(histogram[1:])
    visible = sum(histogram[threshold:])
    max_delta = max(i for i, n in enumerate(histogram) if n) if differing else 0
    mean = sum(i * n for i, n in enumerate(histogram)) / float(total)

    print("max channel delta       %d" % max_delta)
    print("differing pixels        %d / %d  (%.4f%%)"
          % (differing, total, 100.0 * differing / total))
    print("over threshold %-8d %d / %d  (%.4f%%)"
          % (threshold, visible, total, 100.0 * visible / total))
    print("mean delta              %.4f" % mean)

    box = worst.point(lambda v: 255 if v >= threshold else 0).getbbox()
    print("bounding box            %s" % (box,))

    # Amplify so a delta of 8 is actually visible in the written image.
    worst.point(lambda v: min(255, v * 8)).save(out_path)
    print("wrote                   %s" % out_path)


main()
