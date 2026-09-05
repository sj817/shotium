#!/usr/bin/env python3
"""Offline regressions for the two original Bilibili article reports.

python tools/shot/bilibili_check.py --fixtures-only
python tools/shot/bilibili_check.py --package shotium

Pillow is needed only for the engine checks. No browser, server or network is
needed. The Node subprocess exercises screenshot() and screenshotTiles().
"""

import argparse
import hashlib
from html.parser import HTMLParser
import json
from pathlib import Path
import re
import subprocess
import tempfile
import unittest
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / 'shot/testdata/bilibili'
MANIFEST = json.loads((FIXTURES / 'manifest.json').read_text(encoding='utf-8'))
ARTICLES = {'1788403871415', '1788434008828'}


class Resources(HTMLParser):
    def __init__(self):
        super().__init__()
        self.urls = []
        self.images = []

    def handle_starttag(self, tag, attrs):
        self.urls.extend(value for key, value in attrs
                         if key in ('src', 'href', 'poster') and value)
        if tag == 'img':
            self.images.append(dict(attrs))


class FixtureTests(unittest.TestCase):
    def test_all_resources_are_local_and_accounted_for(self):
        self.assertEqual(set(MANIFEST['sources']), ARTICLES)
        assets = {a['path'] for a in MANIFEST['assets']}
        referenced = set()
        for article in MANIFEST['sources']:
            text = (FIXTURES / f'{article}.html').read_text(encoding='utf-8')
            parser = Resources()
            parser.feed(text)
            urls = parser.urls + re.findall(r'url\(["\']?([^\)"\']+)', text)
            for url in urls:
                if url.startswith('#'):
                    continue
                with self.subTest(article=article, resource=url[:100]):
                    self.assertFalse(urlsplit(url).scheme, 'external or embedded resource')
                    self.assertFalse(urlsplit(url).netloc, 'external resource')
                    self.assertIn(url, assets)
                    self.assertTrue((FIXTURES / unquote(url)).is_file())
                    referenced.add(url)
        self.assertEqual(referenced, assets, 'unreferenced fixture assets')

    def test_original_resource_bytes_are_pinned(self):
        for asset in MANIFEST['assets']:
            with self.subTest(asset=asset['path']):
                data = (FIXTURES / asset['path']).read_bytes()
                self.assertEqual(len(data), asset['bytes'])
                self.assertEqual(hashlib.sha256(data).hexdigest(), asset['sha256'])
                if asset['path'].endswith('.woff2'):
                    self.assertEqual(data[:4], b'wOF2')


class RenderTests(unittest.TestCase):
    package = ROOT / 'shotium'

    @classmethod
    def setUpClass(cls):
        from PIL import Image, ImageChops, ImageStat
        cls.Image, cls.ImageChops, cls.ImageStat = Image, ImageChops, ImageStat
        output_root = ROOT / 'shot/testdata/out'
        output_root.mkdir(exist_ok=True)
        cls.output = Path(tempfile.mkdtemp(prefix='bilibili-', dir=output_root))
        subprocess.run(['node', str(ROOT / 'tools/shot/bilibili_capture.cjs'),
                        str(cls.package.resolve()), str(cls.output)],
                       check=True, timeout=300, cwd=ROOT)
        cls.captures = json.loads((cls.output / 'captures.json').read_text())
        if {page['id'] for page in cls.captures['pages']} != ARTICLES:
            raise AssertionError('did not capture both original articles')
        print(f'Bilibili captures: {cls.output}', flush=True)
        print(f'Engine SHA256: {cls.captures["librarySha256"]}', flush=True)

    def assert_pixels_close(self, actual, expected, tolerance=1):
        self.assertEqual(actual.size, expected.size)
        diff = self.ImageChops.difference(actual.convert('RGB'), expected.convert('RGB'))
        self.assertLessEqual(max(high for _, high in diff.getextrema()), tolerance)

    def assert_tile_pixels(self, actual, expected):
        self.assertEqual(actual.size, expected.size)
        diff = self.ImageChops.difference(actual.convert('RGB'), expected.convert('RGB'))
        # Raster strips may move a few antialiased rounded-corner pixels.
        # Bound both aggregate error and its coverage; even one missing row
        # across an 8000px tile exceeds the latter bound.
        self.assertLess(max(self.ImageStat.Stat(diff).mean), 0.01)
        histogram = diff.histogram()
        changed = sum(sum(histogram[channel + 5:channel + 256])
                      for channel in (0, 256, 512))
        self.assertLess(changed / (actual.width * actual.height * 3), 0.0001)

    def test_full_page_and_tiles_have_no_gaps_or_missing_pixels(self):
        for page in self.captures['pages']:
            with self.subTest(article=page['id']):
                self.assertEqual(page['stats']['failed'], 0)
                self.assertEqual(page['tileStats']['failed'], 0)
                with self.Image.open(page['fullPath']) as full:
                    self.assertEqual(full.width, 1440)
                    # Both articles exceed the old 32767 CSS-pixel boundary.
                    self.assertGreater(full.height, 32767)
                    self.assertGreater(len(page['tiles']), 1)
                    y = 0
                    for tile in page['tiles']:
                        self.assertEqual((tile['x'], tile['y'], tile['width']), (0, y, 1440))
                        self.assertGreater(tile['height'], 0)
                        self.assertLessEqual(tile['height'], 8000)
                        with self.Image.open(tile['path']) as image:
                            self.assertEqual(image.size, (1440, tile['height']))
                            self.assert_tile_pixels(image, full.crop((0, y, 1440, y + tile['height'])))
                        y += tile['height']
                    self.assertEqual(y, full.height)

    def test_article_images_match_their_source_and_the_full_page(self):
        for page in self.captures['pages']:
            parser = Resources()
            parser.feed((FIXTURES / f'{page["id"]}.html').read_text(encoding='utf-8'))
            photos = {asset['path'] for asset in MANIFEST['assets']
                      if '/new_dyn/' in asset['source']} & set(parser.urls)
            expected = photos | {image['src'] for image in parser.images
                                 if image.get('alt') == '二维码'}
            self.assertTrue(photos, 'no article photos found')
            self.assertEqual({probe['source'] for probe in page['probes']}, expected)
            self.assertEqual(len(page['probes']), len(expected))
            with self.Image.open(page['fullPath']) as full:
                self.assertGreater(page['probes'][-1]['y'], 32767)
                for probe in page['probes']:
                    with self.subTest(article=page['id'], image=probe['source']):
                        self.assertEqual(probe['stats']['failed'], 0)
                        with self.Image.open(probe['path']) as selected:
                            x, y, w, h = (probe[k] for k in ('x', 'y', 'width', 'height'))
                            # Stay clear of rounded corners and shadows. This
                            # independent source check also rejects matching
                            # blank output from the full and tiled paths.
                            box = (w // 4, h // 4, w * 3 // 4, h * 3 // 4)
                            center = selected.crop(box).convert('RGB')
                            with self.Image.open(FIXTURES / probe['source']) as source:
                                background = selected.convert('RGB').getpixel((w // 2, 0))
                                reference = self.Image.new('RGBA', (w, h), (*background, 255))
                                reference.alpha_composite(source.convert('RGBA').resize((w, h), self.Image.Resampling.BILINEAR))
                                reference = reference.convert('RGB').crop(box)
                            # Different resamplers/subpixel positions can vary
                            # at edges. Compare low-frequency photo content.
                            a = center.resize((16, 16), self.Image.Resampling.BOX)
                            b = reference.resize((16, 16), self.Image.Resampling.BOX)
                            difference = self.ImageStat.Stat(self.ImageChops.difference(a, b))
                            self.assertLess(max(difference.mean), 12)
                            from_full = full.crop((x + box[0], y + box[1], x + box[2], y + box[3]))
                            self.assert_pixels_close(center, from_full, tolerance=4)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixtures-only', action='store_true')
    parser.add_argument('--package', type=Path, default=ROOT / 'shotium')
    args = parser.parse_args()
    RenderTests.package = args.package
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(FixtureTests)
    if not args.fixtures_only:
        suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(RenderTests))
    return 0 if unittest.TextTestRunner(verbosity=2).run(suite).wasSuccessful() else 1


if __name__ == '__main__':
    raise SystemExit(main())
