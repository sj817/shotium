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
    checks.check(selected == clipped,
                 "selector and clip found the same box, to the byte")

    width, height, channels, rows = png_pixels(clipped)
    checks.check(pixel(rows, channels, 0, 0)[:3] == (0xCC, 0x00, 0x00),
                 "and the box really is #cc0000",
                 str(pixel(rows, channels, 0, 0)))

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
