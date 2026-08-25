#!/usr/bin/env python3
"""Exercises shot's network stack against a local server.

serve_check.py covers the protocol and the capture geometry, both over file:.
This covers the part that only exists once //net is linked in: that an http URL
is fetched at all, that redirects are followed, that the disk cache is used
across worker processes, and that networkidle waits for something a plain load
would not.

The strongest check here is the last one in section 1: the same document, served
over http and read off the disk, must render to the same bytes. The transport is
not supposed to be visible in the picture, and comparing digests is how that
stops being an assumption.

    python tools/shot/net_check.py out/ShotSize/shotium.exe
"""

import argparse
import hashlib
import http.server
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import threading
import time

INDEX_HTML = """<!DOCTYPE html>
<html lang="en">
<meta charset="utf-8">
<title>shot network corpus</title>
<link rel="stylesheet" href="style.css">
<div id="box"></div>
<img src="pic.png" width="64" height="64">
</html>
"""

# Kept separate from index.html because index.html is also rendered off the
# disk, and a redirect is something only the server can do: the file: version
# would come back with a broken image and the comparison would be measuring the
# missing file rather than the transport.
REDIRECT_HTML = """<!DOCTYPE html>
<html lang="en">
<meta charset="utf-8">
<title>shot redirect corpus</title>
<img src="r/pic.png" width="64" height="64">
</html>
"""

STYLE_CSS = """
html, body { margin: 0; padding: 0; background: #ffffff; }
#box { width: 200px; height: 100px; background: #3366cc; }
img { display: block; image-rendering: pixelated; }
"""


class Handler(http.server.BaseHTTPRequestHandler):
    # Set by main(); shared by every request thread.
    counts = None
    lock = threading.Lock()
    root = None

    def log_message(self, *args):
        pass  # The counters are the log.

    def _count(self):
        with Handler.lock:
            Handler.counts[self.path] = Handler.counts.get(self.path, 0) + 1

    def do_GET(self):
        self._count()

        if self.path == "/r/pic.png":
            self.send_response(302)
            self.send_header("Location", "/pic.png")
            self.send_header("Content-Length", "0")
            self.end_headers()
            return

        if self.path == "/slow.css":
            # Arrives well after the document has finished parsing, which is
            # what separates networkidle from load.
            time.sleep(0.7)
            body = b"#box { background: #cc3366; }"
            self.send_response(200)
            self.send_header("Content-Type", "text/css")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(body)
            return

        name = self.path.lstrip("/")
        path = os.path.join(Handler.root, name)
        if not os.path.isfile(path):
            self.send_error(404)
            return

        with open(path, "rb") as f:
            body = f.read()
        kind = {
            ".html": "text/html; charset=utf-8",
            ".css": "text/css",
            ".png": "image/png",
        }[os.path.splitext(name)[1]]
        self.send_response(200)
        self.send_header("Content-Type", kind)
        self.send_header("Content-Length", str(len(body)))
        # Long enough that a second worker must hit the cache rather than the
        # network, and without must-revalidate so there is no conditional
        # request either.
        self.send_header("Cache-Control", "max-age=3600")
        self.end_headers()
        self.wfile.write(body)


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


class Worker:
    def __init__(self, exe, args):
        self.proc = subprocess.Popen([exe, "--serve", *args],
                                     stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE)

    def ask(self, request):
        send(self.proc, request)
        return recv(self.proc)

    def close(self):
        self.proc.stdin.close()
        return self.proc.wait(timeout=60)


class Checks:
    def __init__(self):
        self.failures = 0

    def check(self, ok, label, detail=""):
        print(f"  {'PASS' if ok else 'FAIL'}  {label}" + (f"   {detail}" if detail else ""))
        if not ok:
            self.failures += 1

    def skip(self, label, detail=""):
        print(f"  SKIP  {label}" + (f"   {detail}" if detail else ""))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("exe")
    ap.add_argument("--https-probe", default="https://example.com",
                    help="a public https URL, to check TLS end to end")
    args = ap.parse_args()

    exe = os.path.abspath(args.exe)
    checks = Checks()

    root = tempfile.mkdtemp(prefix="shot-net-")
    cache = tempfile.mkdtemp(prefix="shot-cache-")
    try:
        with open(os.path.join(root, "index.html"), "w", encoding="utf-8") as f:
            f.write(INDEX_HTML)
        with open(os.path.join(root, "style.css"), "w", encoding="utf-8") as f:
            f.write(STYLE_CSS)
        with open(os.path.join(root, "redirect.html"), "w",
                  encoding="utf-8") as f:
            f.write(REDIRECT_HTML)
        shutil.copyfile("shot/testdata/checker.png",
                        os.path.join(root, "pic.png"))

        Handler.counts = {}
        Handler.root = root
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        threading.Thread(target=server.serve_forever, daemon=True).start()
        base = f"http://127.0.0.1:{server.server_address[1]}"
        print(f"\nserving {root} at {base}")

        viewport = {"width": 400, "height": 300}

        print("\n== 1. an http document, its subresources and a redirect ==")
        worker = Worker(exe, [f"--cache-dir={cache}"])
        header, http_png = worker.ask({"file": f"{base}/index.html", **viewport})
        checks.check(header.get("ok") is True, "the page renders over http",
                     header.get("error", ""))
        counts = dict(Handler.counts)
        checks.check(counts.get("/index.html") == 1, "the document was fetched",
                     str(counts.get("/index.html")))
        checks.check(counts.get("/style.css") == 1, "the stylesheet was fetched",
                     str(counts.get("/style.css")))
        checks.check(counts.get("/pic.png") == 1, "the image was fetched",
                     str(counts.get("/pic.png")))

        header, redirected = worker.ask(
            {"file": f"{base}/redirect.html", **viewport})
        checks.check(header.get("ok") is True, "a redirecting image renders",
                     header.get("error", ""))
        counts = dict(Handler.counts)
        checks.check(counts.get("/r/pic.png") == 1,
                     "the redirecting URL was requested",
                     str(counts.get("/r/pic.png")))
        checks.check(counts.get("/pic.png", 0) >= 1,
                     "and the redirect was followed to its target",
                     str(counts.get("/pic.png")))

        # The same bytes off the disk. Different URL, same picture -- if these
        # differ, something about the transport is reaching the pixels.
        header, file_png = worker.ask(
            {"file": os.path.join(root, "index.html"), "allowFileAccess": True,
             **viewport})
        checks.check(header.get("ok") is True, "the same page renders over file:",
                     header.get("error", ""))
        checks.check(
            hashlib.sha256(http_png).hexdigest() ==
            hashlib.sha256(file_png).hexdigest(),
            "http and file: produce byte-identical images")
        worker.close()

        print("\n== 2. the disk cache survives the worker that filled it ==")
        before = dict(Handler.counts)
        worker = Worker(exe, [f"--cache-dir={cache}"])
        header, cached_png = worker.ask({"file": f"{base}/index.html", **viewport})
        checks.check(header.get("ok") is True, "a second worker renders it",
                     header.get("error", ""))
        after = dict(Handler.counts)
        checks.check(
            after.get("/style.css") == before.get("/style.css"),
            "and did not re-request the cacheable stylesheet",
            f"{before.get('/style.css')} -> {after.get('/style.css')}")
        checks.check(
            after.get("/pic.png") == before.get("/pic.png"),
            "or the cacheable image",
            f"{before.get('/pic.png')} -> {after.get('/pic.png')}")
        checks.check(
            hashlib.sha256(cached_png).hexdigest() ==
            hashlib.sha256(http_png).hexdigest(),
            "and the cached render is identical to the uncached one")
        worker.close()

        print("\n== 3. networkidle waits for what load does not ==")
        # A stylesheet that arrives late changes the box from blue to pink. A
        # render that stops at `load` may or may not have it; one that waits for
        # the network to go quiet must.
        with open(os.path.join(root, "late.html"), "w", encoding="utf-8") as f:
            f.write(INDEX_HTML.replace(
                '<link rel="stylesheet" href="style.css">',
                '<link rel="stylesheet" href="style.css">'
                '<link rel="stylesheet" href="slow.css">'))
        worker = Worker(exe, ["--cache-dir=" + cache])
        header, idle_png = worker.ask({
            "file": f"{base}/late.html",
            "pageGotoParams": {"waitUntil": "networkidle"},
            **viewport,
        })
        checks.check(header.get("ok") is True, "networkidle renders",
                     header.get("error", ""))
        checks.check(
            hashlib.sha256(idle_png).hexdigest() !=
            hashlib.sha256(http_png).hexdigest(),
            "and the late stylesheet is in the picture")
        worker.close()

        print("\n== 4. https, against a real server ==")
        worker = Worker(exe, ["--cache-dir=" + cache])
        header, payload = worker.ask({
            "file": args.https_probe,
            "pageGotoParams": {"timeout": 15000},
            **viewport,
        })
        if header.get("ok"):
            checks.check(len(payload) > 0, f"{args.https_probe} renders over TLS",
                         f"{len(payload)} bytes")
        else:
            # No internet is not a failure of this binary, and pretending it is
            # would make the check useless on a machine that is offline.
            checks.skip(f"{args.https_probe} could not be reached",
                        header.get("error", ""))
        worker.close()

    finally:
        shutil.rmtree(root, ignore_errors=True)
        shutil.rmtree(cache, ignore_errors=True)

    print(f"\n{'ALL CHECKS PASSED' if not checks.failures else str(checks.failures) + ' CHECK(S) FAILED'}")
    return 1 if checks.failures else 0


if __name__ == "__main__":
    sys.exit(main())
