#!/usr/bin/env python3
"""Exercises shot --serve over its own protocol.

The worker is the half of shotium that has no other test: the CLI path is
covered by tools/shot/accept.ps1 comparing pixels against an oracle, but that
says nothing about framing, about whether a second request on one process
renders the same as the first, or about what happens when a request is refused.

What it checks, in order:

  1. two requests on one process produce byte-identical PNGs -- which is the
     claim that made a resident worker possible in the first place, since
     ShotRenderer used to overwrite its page without detaching the old one
  2. the worker's PNG is byte-identical to the CLI's, so the two entry points
     really do share one path
  3. allowFileAccess actually gates subresources: the same document without it
     must not come back the same, because the corpus loads fonts and bitmaps
     over file:, and --allow-file-access moves what silence means without
     overriding a request that states the field either way
  4. the capture geometry -- fullPage, clip, selector, scale -- lands on the
     exact pixels shot/testdata/features.html states it should
  5. omitBackground really keeps the alpha channel, checked by decoding the PNG
     rather than by comparing sizes
  6. jpeg and webp come back as jpeg and webp
  7. a malformed request, an impossible combination and an unknown field are
     answered and the stream survives them, rather than taking the worker down
  8. closing stdin exits cleanly

    python tools/shot/serve_check.py out/ShotSize/shotium.exe
"""

import argparse
import hashlib
import json
import os
import stat
import struct
import subprocess
import sys
import zlib


def send(proc, request):
    payload = json.dumps(request).encode("utf-8")
    proc.stdin.write(struct.pack("<I", len(payload)))
    proc.stdin.write(payload)
    proc.stdin.flush()


def read_exactly(stream, count):
    data = b""
    while len(data) < count:
        chunk = stream.read(count - len(data))
        if not chunk:
            raise EOFError("worker closed the stream mid-frame")
        data += chunk
    return data


def read_frame(stream):
    (length,) = struct.unpack("<I", read_exactly(stream, 4))
    return read_exactly(stream, length) if length else b""


def recv(proc):
    header = json.loads(read_frame(proc.stdout).decode("utf-8"))
    return header, read_frame(proc.stdout)


def recv_tiles(proc):
    """A tiles reply: the header, then one payload frame per tile it lists.

    A failure header is followed by the one empty frame every failure gets,
    so the stream stays in step either way."""
    header = json.loads(read_frame(proc.stdout).decode("utf-8"))
    count = len(header["tiles"]) if header.get("ok") else 1
    return header, [read_frame(proc.stdout) for _ in range(count)]


# A document taller than blink paints from one scroll position (32767 CSS
# pixels): white all the way down, and a red strip as the last 10px, so that
# "did the bottom get painted" is one pixel read. A file rather than a data:
# URL, which the worker does not load.
TALL_HEIGHT = 36000


def tall_document():
    path = os.path.abspath("shot/testdata/out/tall_check.html")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write("<body style='margin:0'>"
                f"<div style='height:{TALL_HEIGHT - 10}px;background:#fff'>"
                "</div><div style='height:10px;background:#f00'></div></body>")
    return path


def fixture(name, contents):
    path = os.path.abspath(os.path.join("shot/testdata/out", name))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(contents)
    return path


def png_size(data):
    """(width, height) out of the IHDR, which is always the first chunk."""
    return struct.unpack(">II", data[16:24])


def png_pixels(data):
    """Decodes a non-interlaced 8-bit PNG to (width, height, channels, rows).

    Written out here rather than pulled from Pillow so the check has no
    dependency beyond the standard library: a screenshot tool whose test suite
    needs an image library to say whether the alpha channel survived is a test
    suite that will not be run.
    """
    width, height = png_size(data)
    depth, colour = data[24], data[25]
    assert depth == 8, f"expected 8 bits per channel, got {depth}"
    channels = {0: 1, 2: 3, 4: 2, 6: 4}[colour]

    idat = b""
    offset = 8
    while offset < len(data):
        (length,) = struct.unpack(">I", data[offset:offset + 4])
        kind = data[offset + 4:offset + 8]
        if kind == b"IDAT":
            idat += data[offset + 8:offset + 8 + length]
        offset += 12 + length

    raw = zlib.decompress(idat)
    stride = width * channels
    rows = []
    previous = bytearray(stride)
    pos = 0
    for _ in range(height):
        filter_type = raw[pos]
        line = bytearray(raw[pos + 1:pos + 1 + stride])
        pos += 1 + stride
        for i in range(stride):
            a = line[i - channels] if i >= channels else 0
            b = previous[i]
            c = previous[i - channels] if i >= channels else 0
            if filter_type == 1:
                line[i] = (line[i] + a) & 0xFF
            elif filter_type == 2:
                line[i] = (line[i] + b) & 0xFF
            elif filter_type == 3:
                line[i] = (line[i] + (a + b) // 2) & 0xFF
            elif filter_type == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[i] = (line[i] + pred) & 0xFF
        rows.append(bytes(line))
        previous = line
    return width, height, channels, rows


def pixel(rows, channels, x, y):
    row = rows[y]
    return tuple(row[x * channels:(x + 1) * channels])


class Checks:
    def __init__(self):
        self.failures = 0

    def check(self, ok, label, detail=""):
        print(f"  {'PASS' if ok else 'FAIL'}  {label}" + (f"   {detail}" if detail else ""))
        if not ok:
            self.failures += 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("exe")
    ap.add_argument("--corpus", default="shot/testdata/render_corpus.html")
    ap.add_argument("--features", default="shot/testdata/features.html")
    ap.add_argument("--width", type=int, default=1248)
    ap.add_argument("--height", type=int, default=1320)
    args = ap.parse_args()

    exe = os.path.abspath(args.exe)
    corpus = os.path.abspath(args.corpus)
    features = os.path.abspath(args.features)
    checks = Checks()

    # The CLI's answer, to compare the worker against.
    cli_png = os.path.abspath("shot/testdata/out/shot_cli.png")
    subprocess.run(
        [exe, "--file", corpus, "--width", str(args.width),
         "--height", str(args.height), "--output", cli_png],
        check=True)
    invalid_cli = subprocess.run(
        [exe, "--file", corpus, "--tile-height", "32001",
         "--output", cli_png],
        capture_output=True, text=True)
    checks.check(invalid_cli.returncode == 2 and
                 "1 to 32000" in invalid_cli.stderr,
                 "the CLI rejects tile heights above the paint limit",
                 invalid_cli.stderr.strip())
    with open(cli_png, "rb") as f:
        cli_digest = hashlib.sha256(f.read()).hexdigest()
    print(f"\nCLI     sha256 {cli_digest[:32]}")

    proc = subprocess.Popen([exe, "--serve"],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE)

    request = {
        "file": corpus,
        "width": args.width,
        "height": args.height,
        "allowFileAccess": True,
    }

    def ask(extra, base=None):
        payload = dict(base if base is not None else request)
        payload.update(extra)
        send(proc, payload)
        return recv(proc)

    print("\n== two requests on one process ==")
    digests = []
    for i in range(2):
        header, payload = ask({})
        checks.check(header.get("ok") is True, f"request {i + 1} succeeded",
                     header.get("error", ""))
        checks.check(len(payload) == header.get("bytes"),
                     f"request {i + 1} payload length matches header",
                     f"{len(payload)} vs {header.get('bytes')}")
        digests.append(hashlib.sha256(payload).hexdigest())
        print(f"        sha256 {digests[-1][:32]}")

    checks.check(digests[0] == digests[1],
                 "the second render is byte-identical to the first")
    checks.check(digests[0] == cli_digest,
                 "the worker's PNG is byte-identical to the CLI's")

    print("\n== allowFileAccess actually gates subresources ==")
    denied_request = {k: v for k, v in request.items() if k != "allowFileAccess"}
    send(proc, denied_request)
    header, payload = recv(proc)
    checks.check(header.get("ok") is True, "request without file access still renders",
                 header.get("error", ""))
    denied = hashlib.sha256(payload).hexdigest()
    checks.check(denied != digests[0],
                 "and renders differently, because fonts and images were refused")

    # The same question asked of a worker started with --allow-file-access.
    # The point of the flag is that the answer to silence belongs to whoever
    # launched the process rather than to whoever sends the request, so both
    # halves matter: silence now means yes, and an explicit no is still obeyed.
    print("\n== --allow-file-access moves the default, not the decision ==")
    permissive = subprocess.Popen([exe, "--serve", "--allow-file-access"],
                                  stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE)
    send(permissive, denied_request)
    header, payload = recv(permissive)
    checks.check(header.get("ok") is True,
                 "a silent request renders on a permissive worker",
                 header.get("error", ""))
    checks.check(hashlib.sha256(payload).hexdigest() == digests[0],
                 "and matches the render that asked for file access")

    send(permissive, dict(denied_request, allowFileAccess=False))
    header, payload = recv(permissive)
    checks.check(header.get("ok") is True,
                 "an explicit allowFileAccess:false still renders",
                 header.get("error", ""))
    checks.check(hashlib.sha256(payload).hexdigest() == denied,
                 "and matches the refused render, so the request still wins")
    permissive.stdin.close()
    permissive.wait(timeout=30)

    # features.html states its own geometry, so these are exact.
    print("\n== capture geometry ==")
    geometry = {"file": features, "width": 400, "height": 300,
                "allowFileAccess": True}

    header, payload = ask({}, geometry)
    checks.check(header.get("ok") is True, "the feature page renders",
                 header.get("error", ""))
    checks.check(png_size(payload) == (400, 300), "the viewport shot is 400x300",
                 str(png_size(payload)))

    header, full = ask({"fullPage": True}, geometry)
    checks.check(header.get("ok") is True, "fullPage renders",
                 header.get("error", ""))
    checks.check(png_size(full) == (400, 2000),
                 "fullPage reaches the bottom of a 2000px document",
                 str(png_size(full)))

    header, clipped = ask(
        {"clip": {"x": 40, "y": 60, "width": 200, "height": 120}}, geometry)
    checks.check(header.get("ok") is True, "clip renders", header.get("error", ""))
    checks.check(png_size(clipped) == (200, 120), "clip is exactly 200x120",
                 str(png_size(clipped)))

    header, selected = ask({"selector": "#box"}, geometry)
    checks.check(header.get("ok") is True, "selector renders",
                 header.get("error", ""))
    checks.check(png_size(selected) == (200, 120),
                 "the selected element is exactly 200x120",
                 str(png_size(selected)))

    print("\n== ordinary output paths stay literal ==")
    literal_path = os.path.abspath(
        f"shot/testdata/out/literal-{{n}}-{os.getpid()}.png")
    expanded_path = literal_path.replace("{n}", "1")
    for candidate in (literal_path, expanded_path):
        if os.path.isfile(candidate):
            os.unlink(candidate)
    header, payload = ask({"path": literal_path}, geometry)
    checks.check(header.get("ok") is True,
                 "an ordinary path containing {n} renders",
                 header.get("error", ""))
    checks.check(header.get("path") == literal_path and
                 os.path.isfile(literal_path) and
                 not os.path.exists(expanded_path) and not payload,
                 "and reports and writes the literal path only",
                 str(header.get("path")))
    if os.path.isfile(literal_path):
        os.unlink(literal_path)

    if os.name != "nt":
        mode_path = os.path.abspath(
            f"shot/testdata/out/mode-{os.getpid()}.png")
        with open(mode_path, "wb") as f:
            f.write(b"old screenshot")
        os.chmod(mode_path, 0o640)
        header, _ = ask({"path": mode_path}, geometry)
        checks.check(header.get("ok") is True,
                     "an existing POSIX output is replaced",
                     header.get("error", ""))
        checks.check(stat.S_IMODE(os.stat(mode_path).st_mode) == 0o640,
                     "without changing its permission bits",
                     oct(stat.S_IMODE(os.stat(mode_path).st_mode)))
        os.unlink(mode_path)

    print("\n== a document taller than one paint reaches its bottom ==")
    tall = {"file": tall_document(), "width": 400, "height": 300}
    header, banded = ask({"fullPage": True, "scale": 0.25}, tall)
    checks.check(header.get("ok") is True, "fullPage renders a 36000px page",
                 header.get("error", ""))
    checks.check(png_size(banded) == (100, TALL_HEIGHT // 4),
                 "as one image of the whole height", str(png_size(banded)))
    _, _, channels, rows = png_pixels(banded)
    bottom = pixel(rows, channels, 50, TALL_HEIGHT // 4 - 2)[:3]
    checks.check(bottom == (255, 0, 0),
                 "and the last rows are painted, not left blank",
                 str(bottom))
    above = pixel(rows, channels, 50, 32767 // 4 - 2)[:3]
    checks.check(above == (255, 255, 255),
                 "with the rows either side of the paint limit intact",
                 str(above))

    print("\n== tiles ==")
    send(proc, dict(tall, fullPage=True, tile={"height": 8000}))
    header, tiles = recv_tiles(proc)
    checks.check(header.get("ok") is True, "a tiles request renders",
                 header.get("error", ""))
    listed = header.get("tiles", [])
    checks.check(len(listed) == 5 and len(tiles) == 5,
                 "36000px in 8000px tiles is five tiles",
                 f"{len(listed)} listed, {len(tiles)} frames")
    checks.check([t["y"] for t in listed] == [0, 8000, 16000, 24000, 32000],
                 "stacked top to bottom", str([t["y"] for t in listed]))
    checks.check([t["height"] for t in listed] == [8000] * 4 + [4000],
                 "the last tile being what was left",
                 str([t["height"] for t in listed]))
    sizes = [png_size(t) for t in tiles]
    checks.check(sizes == [(400, 8000)] * 4 + [(400, 4000)],
                 "and each frame is a PNG of its tile's size", str(sizes))
    checks.check(all(len(t) == e["bytes"] for t, e in zip(tiles, listed)),
                 "whose lengths match the header")
    _, _, channels, rows = png_pixels(tiles[-1])
    bottom = pixel(rows, channels, 200, 3998)[:3]
    checks.check(bottom == (255, 0, 0), "the last tile ends in the red strip",
                 str(bottom))

    send(proc, dict(tall, fullPage=True, tile={"height": 8000},
                    path="tiles.png"))
    header, _ = recv_tiles(proc)
    checks.check(header.get("ok") is False and "{n}" in header.get("error", ""),
                 "a tiles path without {n} is refused by name",
                 header.get("error", ""))
    header, payload = ask({}, geometry)
    checks.check(header.get("ok") is True and png_size(payload) == (400, 300),
                 "and the stream is still in step afterwards")
    checks.check(selected == clipped,
                 "selector and clip found the same box, to the byte")

    print("\n== fractional tiles share the whole image's device grid ==")
    fractional_path = fixture(
        "fractional_tiles.html",
        "<style>html,body{margin:0;width:20px}body{height:1001px;"
        "background:repeating-linear-gradient(to bottom,#123456 0 1px,"
        "#abcdef 1px 2px)}</style>")
    fractional = {"file": fractional_path, "width": 20, "height": 100,
                  "allowFileAccess": True, "fullPage": True, "scale": 1.5}
    header, fractional_whole = ask({}, fractional)
    checks.check(header.get("ok") is True, "the fractional whole image renders",
                 header.get("error", ""))
    send(proc, dict(fractional, tile={"height": 333}))
    header, fractional_tiles = recv_tiles(proc)
    checks.check(header.get("ok") is True, "the fractional tiles render",
                 header.get("error", ""))
    whole_width, whole_height, whole_channels, whole_rows = png_pixels(
        fractional_whole)
    tile_images = [png_pixels(tile) for tile in fractional_tiles]
    tile_rows = [row for image in tile_images for row in image[3]]
    checks.check(sum(image[1] for image in tile_images) == whole_height,
                 "their heights add up to the whole image",
                 f"{[image[1] for image in tile_images]} vs {whole_height}")
    checks.check(all(image[0] == whole_width and image[2] == whole_channels
                     for image in tile_images) and tile_rows == whole_rows,
                 "and concatenating them reproduces every whole-image pixel")

    print("\n== scrolled capture windows do not repeat viewport content ==")
    fixed_path = fixture(
        "fixed_clip_window.html",
        "<style>html,body{margin:0;width:100px}body{height:34000px;"
        "background:#06c}.fixed{position:fixed;inset:0 0 auto;height:20px;"
        "background:#f00}</style><div class=fixed></div>")
    fixed = {"file": fixed_path, "width": 100, "height": 720,
             "allowFileAccess": True, "scale": 0.25}
    clip = {"x": 0, "y": 1000, "width": 100, "height": 100}
    header, short_clip = ask({"clip": clip}, fixed)
    checks.check(header.get("ok") is True, "the short offset clip renders",
                 header.get("error", ""))
    clip = {"x": 0, "y": 1000, "width": 100, "height": 32000}
    header, long_clip = ask({"clip": clip}, fixed)
    checks.check(header.get("ok") is True, "the windowed offset clip renders",
                 header.get("error", ""))
    _, _, channels, short_rows = png_pixels(short_clip)
    _, _, long_channels, long_rows = png_pixels(long_clip)
    blue = (0x00, 0x66, 0xCC)
    checks.check(pixel(short_rows, channels, 12, 2)[:3] == blue and
                 pixel(long_rows, long_channels, 12, 2)[:3] == blue,
                 "the same document rows stay clear of the fixed header")

    top_sticky_path = fixture(
        "top_sticky_window.html",
        "<style>html,body{margin:0;width:100px}body{background:#06c}"
        ".before{height:31990px}.sticky{position:sticky;top:0;height:20px;"
        "background:#0f0}.after{height:3990px}</style>"
        "<div class=before></div><div class=sticky></div><div class=after></div>")
    inset_sticky_path = fixture(
        "inset_sticky_window.html",
        "<style>html,body{margin:0;width:100px}body{background:#06c}"
        ".before{height:31990px}.sticky{position:sticky;top:10px;height:20px;"
        "background:#0f0}.after{height:3990px}</style>"
        "<div class=before></div><div class=sticky></div><div class=after></div>")
    bottom_sticky_path = fixture(
        "bottom_sticky_window.html",
        "<style>html,body{margin:0;width:100px}body{background:#06c}"
        ".before{height:33000px}.sticky{position:sticky;bottom:0;height:20px;"
        "background:#0f0}.after{height:2980px}</style>"
        "<div class=before></div><div class=sticky></div><div class=after></div>")

    def sticky_rows(path):
        header, image = ask(
            {"file": path, "width": 100, "height": 720,
             "allowFileAccess": True, "fullPage": True, "scale": 0.25}, {})
        checks.check(header.get("ok") is True,
                     os.path.basename(path) + " renders", header.get("error", ""))
        _, _, image_channels, image_rows = png_pixels(image)
        colours = [pixel(image_rows, image_channels, 12, y)[:3]
                   for y in range(len(image_rows))]
        return ([y for y, colour in enumerate(colours) if colour != blue],
                [y for y, colour in enumerate(colours)
                 if colour == (0, 255, 0)])

    top_painted, top_green = sticky_rows(top_sticky_path)
    inset_painted, inset_green = sticky_rows(inset_sticky_path)
    _, bottom_green = sticky_rows(bottom_sticky_path)
    checks.check(top_painted == list(range(7997, 8003)) and
                 top_green == list(range(7998, 8002)),
                 "a top-sticky box crossing the seam keeps exactly its flow rows",
                 f"painted {top_painted}, solid {top_green}")
    checks.check(inset_painted == list(range(7997, 8003)) and
                 inset_green == list(range(7998, 8002)),
                 "a sticky inset does not hide the suffix beyond the seam",
                 f"painted {inset_painted}, solid {inset_green}")
    checks.check(len(bottom_green) == 5 and bottom_green == list(range(175, 180)),
                 "a bottom-sticky box is kept in only the requested viewport",
                 str(bottom_green))

    print("\n== tile files commit as one result ==")
    atomic_dir = os.path.abspath("shot/testdata/out/atomic_tiles")
    os.makedirs(atomic_dir, exist_ok=True)
    first_tile = os.path.join(atomic_dir, "tile-1.png")
    blocked_tile = os.path.join(atomic_dir, "tile-2.png")
    sentinel = b"the old first tile"
    with open(first_tile, "wb") as f:
        f.write(sentinel)
    if os.path.isfile(blocked_tile):
        os.unlink(blocked_tile)
    os.makedirs(blocked_tile, exist_ok=True)
    send(proc, dict(fractional, scale=1, tile={"height": 600},
                    path=os.path.join(atomic_dir, "tile-{n}.png")))
    header, _ = recv_tiles(proc)
    checks.check(header.get("ok") is False,
                 "a tile set reports a failed final commit")
    with open(first_tile, "rb") as f:
        restored = f.read()
    checks.check(restored == sentinel,
                 "and rolls an earlier destination back to its old bytes")
    os.rmdir(blocked_tile)

    if os.name != "nt":
        mode_template = os.path.join(atomic_dir, "mode-{n}.png")
        mode_tile = mode_template.replace("{n}", "1")
        with open(mode_tile, "wb") as f:
            f.write(b"old tile")
        os.chmod(mode_tile, 0o640)
        send(proc, dict(fractional, scale=1, tile={"height": 600},
                        path=mode_template))
        header, mode_tiles = recv_tiles(proc)
        checks.check(header.get("ok") is True and not any(mode_tiles),
                     "an existing POSIX tile is replaced",
                     header.get("error", ""))
        checks.check(stat.S_IMODE(os.stat(mode_tile).st_mode) == 0o640,
                     "without changing the tile's permission bits",
                     oct(stat.S_IMODE(os.stat(mode_tile).st_mode)))
        for tile in header.get("tiles", []):
            if tile.get("path") and os.path.isfile(tile["path"]):
                os.unlink(tile["path"])

    width, height, channels, rows = png_pixels(clipped)
    checks.check(pixel(rows, channels, 0, 0)[:3] == (0xCC, 0x00, 0x00),
                 "and the box really is #cc0000",
                 str(pixel(rows, channels, 0, 0)))

    print("\n== selectors beyond the viewport ==")
    oversized_geometry = {
        "file": os.path.join(os.path.dirname(features),
                             "selector_oversized.html"),
        "width": 400,
        "height": 300,
        "allowFileAccess": True,
    }
    header, oversized = ask({"selector": "#oversized"}, oversized_geometry)
    checks.check(header.get("ok") is True,
                 "an oversized selector renders", header.get("error", ""))
    checks.check(png_size(oversized) == (800, 600),
                 "and uses the element's full 800x600 box",
                 str(png_size(oversized)))
    _, _, channels, rows = png_pixels(oversized)
    checks.check(pixel(rows, channels, 100, 100)[:3] == (0x00, 0xCC, 0x00),
                 "content inside the original viewport is present",
                 str(pixel(rows, channels, 100, 100)))
    checks.check(pixel(rows, channels, 300, 100)[:3] == (0xCC, 0x00, 0x00),
                 "and 50vw stayed 200px instead of reflowing to 400px",
                 str(pixel(rows, channels, 300, 100)))

    centered_geometry = {
        "file": os.path.join(os.path.dirname(features),
                             "selector_centered.html"),
        "width": 400,
        "height": 300,
        "allowFileAccess": True,
    }
    header, centered = ask({"selector": "#centered"}, centered_geometry)
    checks.check(header.get("ok") is True,
                 "a centered selector larger than the viewport renders",
                 header.get("error", ""))
    checks.check(png_size(centered) == (800, 600),
                 "and negative initial bounds settle to the full 800x600 box",
                 str(png_size(centered)))
    _, _, channels, rows = png_pixels(centered)
    checks.check(pixel(rows, channels, 0, 0)[:3] == (0x00, 0x66, 0xCC) and
                 pixel(rows, channels, 799, 599)[:3] == (0x00, 0x66, 0xCC),
                 "and both far corners were painted",
                 f"{pixel(rows, channels, 0, 0)} / "
                 f"{pixel(rows, channels, 799, 599)}")

    header, scaled = ask({"scale": 2}, geometry)
    checks.check(header.get("ok") is True, "scale 2 renders",
                 header.get("error", ""))
    checks.check(png_size(scaled) == (800, 600),
                 "scale 2 doubles the pixels, not the layout",
                 str(png_size(scaled)))

    print("\n== omitBackground keeps the alpha channel ==")
    header, opaque = ask({}, geometry)
    _, _, channels, rows = png_pixels(opaque)
    corner = pixel(rows, channels, 399, 299)
    # Three channels, not four: the encoder drops an alpha channel that is
    # opaque everywhere, so "no alpha channel at all" is the stronger form of
    # what this is asserting.
    checks.check(channels == 3, "without it the PNG has no alpha channel",
                 f"{channels} channels")
    checks.check(corner == (0xFF, 0xFF, 0xFF),
                 "and the page sits on white", str(corner))

    header, transparent = ask({"omitBackground": True}, geometry)
    checks.check(header.get("ok") is True, "omitBackground renders",
                 header.get("error", ""))
    _, _, channels, rows = png_pixels(transparent)
    corner = pixel(rows, channels, 399, 299)
    checks.check(channels == 4, "with it the PNG keeps its alpha channel",
                 f"{channels} channels")
    checks.check(corner[3] == 0, "and the uncovered corner is transparent",
                 str(corner))
    checks.check(pixel(rows, channels, 100, 100)[:3] == (0xCC, 0x00, 0x00),
                 "and the painted box is still there",
                 str(pixel(rows, channels, 100, 100)))

    print("\n== the other encoders ==")
    header, jpeg = ask({"type": "jpeg", "quality": 90}, geometry)
    checks.check(header.get("ok") is True, "jpeg renders", header.get("error", ""))
    checks.check(jpeg[:3] == b"\xff\xd8\xff", "and is a JPEG", jpeg[:3].hex())

    header, webp = ask({"type": "webp", "quality": 90}, geometry)
    checks.check(header.get("ok") is True, "webp renders", header.get("error", ""))
    checks.check(webp[:4] == b"RIFF" and webp[8:12] == b"WEBP", "and is a WebP",
                 webp[:12].hex())

    print("\n== bad input is answered, not fatal ==")
    send(proc, {"file": corpus, "tile": {"height": 32001}})
    header, payload = recv(proc)
    checks.check(header.get("ok") is False and not payload,
                 "a tile taller than the paint limit is rejected")
    checks.check("tile.height" in header.get("error", "") and
                 "32000" in header.get("error", ""),
                 "and the error names the supported tile.height range",
                 header.get("error", ""))

    send(proc, {"file": corpus, "width": "1248"})
    header, payload = recv(proc)
    checks.check(header.get("ok") is False, "a string where a number belongs is rejected")
    checks.check("width" in header.get("error", ""), "the error names the field",
                 header.get("error", ""))

    send(proc, {"file": features, "fullPage": True,
                "clip": {"x": 0, "y": 0, "width": 10, "height": 10}})
    header, payload = recv(proc)
    checks.check(header.get("ok") is False,
                 "fullPage and clip together are rejected")
    checks.check("fullPage" in header.get("error", ""), "and the error says why",
                 header.get("error", ""))

    send(proc, {"file": features, "type": "jpeg", "omitBackground": True})
    header, payload = recv(proc)
    checks.check(header.get("ok") is False,
                 "omitBackground on a jpeg is rejected, not silently dropped")
    checks.check("alpha" in header.get("error", ""), "and the error says why",
                 header.get("error", ""))

    send(proc, {"file": features, "pngCompression": "balanced"})
    header, payload = recv(proc)
    checks.check(header.get("ok") is False,
                 "the removed PNG compression mode is rejected")
    checks.check("removed" in header.get("error", ""),
                 "and the error explains that it is gone",
                 header.get("error", ""))

    send(proc, {"file": features, "selector": "!!not a selector"})
    header, payload = recv(proc)
    checks.check(header.get("ok") is False, "an invalid selector is rejected")
    checks.check("selector" in header.get("error", ""), "and named",
                 header.get("error", ""))

    print("\n== the stream survived all of that ==")
    send(proc, request)
    header, payload = recv(proc)
    checks.check(header.get("ok") is True, "a good request after five bad ones still works")
    checks.check(hashlib.sha256(payload).hexdigest() == digests[0],
                 "and produces the same bytes as the first")

    print("\n== closing stdin exits cleanly ==")
    proc.stdin.close()
    code = proc.wait(timeout=60)
    checks.check(code == 0, "exit code is 0", str(code))

    print(f"\n{'ALL CHECKS PASSED' if not checks.failures else str(checks.failures) + ' CHECK(S) FAILED'}")
    return 1 if checks.failures else 0


if __name__ == "__main__":
    sys.exit(main())
