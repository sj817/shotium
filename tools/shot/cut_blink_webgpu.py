"""Delete blink platform's WebGPU client and the WebXR frame transport.

//gpu/webgpu, //third_party/dawn and gpu/command_buffer/client/webgpu_interface.h
are already gone, but blink kept its whole client side of them:
platform/graphics/gpu/{dawn_*,webgpu_*} plus the WebXR frame transport, which
exists only to hand rendered frames to a WebXR device over either the WebGL or
the WebGPU path. A screenshot engine has neither.

This deletes the files and unhooks them from platform/BUILD.gn. The references
that survive in other files are handled separately: most of the remaining
"WebGPU" hits in blink are comments, histogram strings and the
CanvasRenderingAPI::kWebgpu enumerator, all of which are inert -- the enumerator
is only ever produced by `canvas.getContext("webgpu")`, and the factory that
would answer that call lives in modules/webgpu, which is already deleted.

Usage:
  cut_blink_webgpu.py [--apply]
"""

import os
import re
import sys

ROOT = r"D:\Github\chromium"
GPU_DIR = os.path.join(ROOT, "third_party", "blink", "renderer", "platform",
                       "graphics", "gpu")
BUILD = os.path.join(ROOT, "third_party", "blink", "renderer", "platform",
                     "BUILD.gn")

# Basenames under platform/graphics/gpu to remove.
KILL = re.compile(r"^(?:dawn_|webgpu_|xr_frame_transport|xr_gpu_frame_transport)")

DEAD_DEPS = ['"//gpu/webgpu:common",']


def main():
    apply_changes = "--apply" in sys.argv
    gone = sorted(fn for fn in os.listdir(GPU_DIR) if KILL.match(fn))
    for fn in gone:
        print("  del  graphics/gpu/%s" % fn)
        if apply_changes:
            os.remove(os.path.join(GPU_DIR, fn))

    src = open(BUILD, encoding="utf-8").read()
    out = src
    for fn in gone:
        out = re.sub(r'^[ \t]*"graphics/gpu/%s",?[ \t]*\n' % re.escape(fn), "",
                     out, flags=re.M)
    for dep in DEAD_DEPS:
        out = re.sub(r'^[ \t]*%s[ \t]*\n' % re.escape(dep), "", out, flags=re.M)
    print("---- %d file(s) deleted, BUILD.gn -%d bytes"
          % (len(gone), len(src) - len(out)))
    if apply_changes:
        open(BUILD, "w", encoding="utf-8", newline="").write(out)


main()
