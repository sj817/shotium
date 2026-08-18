"""Break a shot-vs-oracle difference down by region, and say what kind it is.

pixel_diff.py answers "how far apart are these two images". That number alone
cannot satisfy the acceptance criterion for this tree, which is that every
difference is measured *and accounted for*. A single 3% is compatible with
"antialiasing is a shade different everywhere" and with "one element is missing
entirely", and those call for opposite responses.

So this splits the corpus into the regions it was built out of -- one per
feature under test -- and reports each separately, plus a classification of the
difference's shape:

  coverage    fraction of the region's pixels that differ at all
  visible     fraction differing by more than --threshold (default 8/255)
  mean/max    magnitude
  runs        mean horizontal run length of differing pixels. An edge that is
              antialiased slightly differently gives runs of 1-2; a region that
              is uniformly wrong gives runs as wide as the region. This is what
              separates "the same picture, rasterised differently" from "a
              different picture".
  channels    per-channel mean delta. A difference that is equal across R, G
              and B is geometric or gamma; one that is not is a colour path.

Regions are given as `name:x,y,w,h` on the command line, or read from a file
with --regions (one per line, '#' comments allowed), so the corpus layout lives
next to the corpus rather than in this script.

Usage:
  diff_report.py oracle.png shot.png --regions shot/testdata/regions.txt
  diff_report.py oracle.png shot.png text:23,35,1200,160
"""

import sys

from PIL import Image, ImageChops


def parse_region(spec):
    name, _, rect = spec.partition(':')
    x, y, w, h = (int(v) for v in rect.split(','))
    return name, (x, y, x + w, y + h)


def load_regions(argv):
    regions = []
    if '--regions' in argv:
        path = argv[argv.index('--regions') + 1]
        with open(path, encoding='utf-8') as f:
            for line in f:
                line = line.split('#', 1)[0].strip()
                if line:
                    regions.append(parse_region(line))
    for arg in argv:
        if ':' in arg and not arg.startswith('--'):
            regions.append(parse_region(arg))
    return regions


def worst_channel(diff):
    bands = diff.split()
    worst = bands[0]
    for band in bands[1:]:
        worst = ImageChops.lighter(worst, band)
    return worst


def mean_run_length(mask):
    """Mean length of a horizontal run of set pixels.

    Reading the rows out of the image once and scanning them in python is fast
    enough at this size, and keeps the tool dependency-free beyond PIL.
    """
    width, height = mask.size
    data = mask.load()
    runs = 0
    total = 0
    for y in range(height):
        run = 0
        for x in range(width):
            if data[x, y]:
                run += 1
                total += 1
            elif run:
                runs += 1
                run = 0
        if run:
            runs += 1
    return (float(total) / runs) if runs else 0.0


def report(name, a, b, box, threshold):
    ra = a.crop(box)
    rb = b.crop(box)
    diff = ImageChops.difference(ra, rb)
    worst = worst_channel(diff)
    total = ra.width * ra.height

    histogram = worst.histogram()
    differing = sum(histogram[1:])
    visible = sum(histogram[threshold:])
    max_delta = max((i for i, n in enumerate(histogram) if n), default=0)
    mean = sum(i * n for i, n in enumerate(histogram)) / float(total)

    channel_means = []
    for band in diff.split():
        hist = band.histogram()
        channel_means.append(
            sum(i * n for i, n in enumerate(hist)) / float(total))

    mask = worst.point(lambda v: 255 if v >= threshold else 0).convert('1')
    runs = mean_run_length(mask)

    print('%-22s %7.3f%% %7.3f%% %6.2f %4d %6.2f   %.2f/%.2f/%.2f'
          % (name, 100.0 * differing / total, 100.0 * visible / total,
             mean, max_delta, runs,
             channel_means[0], channel_means[1], channel_means[2]))


def main():
    argv = sys.argv[1:]
    files = [a for a in argv if a.endswith('.png')]
    if len(files) < 2:
        sys.exit(__doc__)
    threshold = 8
    if '--threshold' in argv:
        threshold = int(argv[argv.index('--threshold') + 1])

    a = Image.open(files[0]).convert('RGB')
    b = Image.open(files[1]).convert('RGB')
    if a.size != b.size:
        sys.exit('size mismatch: %s vs %s' % (a.size, b.size))

    regions = load_regions(argv)
    regions.append(('WHOLE IMAGE', (0, 0, a.width, a.height)))

    print('%-22s %8s %8s %6s %4s %6s   %s'
          % ('region', 'differ', 'visible', 'mean', 'max', 'runs', 'R/G/B mean'))
    print('-' * 78)
    for name, box in regions:
        report(name, a, b, box, threshold)


main()
