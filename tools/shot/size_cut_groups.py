import collections, re, sys
mod_re = re.compile(r"^\s*Mod (\d+) \| `([^`]*)`")
mn = {}
for line in open(sys.argv[2], errors="replace"):
    m = mod_re.match(line)
    if m: mn[int(m.group(1))] = m.group(2)
sc = re.compile(r"^\s*SC\[[^\]]+\]\s*\| mod = (\d+), [0-9a-fA-F]+:[0-9a-fA-F]+, size = (\d+)")
# candidate cut groups: label -> list of path substrings
CUTS = [
 ("devtools+inspector (C++ side)", ["/devtools/", "/inspector/", "blink/renderer/core/probe/", "devtools_"]),
 ("WebRTC + mediastream/peerconnection", ["third_party/webrtc/", "blink/renderer/modules/mediastream/", "blink/renderer/modules/peerconnection/", "third_party/libsrtp/", "third_party/usrsctp/"]),
 ("ML: tflite/xnnpack/webnn/modules{ml,ai}", ["third_party/tflite/", "third_party/xnnpack/", "third_party/ruy/", "third_party/fp16/", "third_party/pthreadpool/", "third_party/cpuinfo/", "services/webnn/", "blink/renderer/modules/ml/", "blink/renderer/modules/ai/", "services/on_device_model/"]),
 ("audio/video codecs + media pipeline", ["third_party/libvpx/", "third_party/ffmpeg/", "third_party/opus/", "third_party/iamf_tools/", "third_party/openh264/", "media/", "blink/renderer/modules/webcodecs/", "blink/renderer/modules/mediasource/", "blink/renderer/modules/media_controls/", "blink/renderer/modules/mediarecorder/"]),
 ("GPU stack: ANGLE+Dawn+gpu+ui/gl+spirv", ["third_party/angle/", "third_party/dawn/", "third_party/spirv-tools/", "third_party/vulkan", "gpu/", "ui/gl/", "third_party/glslang/"]),
 ("V8 (all libs)", ["v8/"]),
 ("Blink bindings (core+modules)", ["blink/renderer/bindings/"]),
 ("Blink modules/* (all 118)", ["blink/renderer/modules/"]),
 ("ICU data blob", ["icu/icudata"]),
 ("QUIC/HTTP3", ["net/third_party/quiche/"]),
 ("XR/WebGL/WebGPU (blink side)", ["blink/renderer/modules/xr/", "blink/renderer/modules/webgl/", "blink/renderer/modules/webgpu/", "blink/renderer/modules/vr/"]),
 ("accessibility", ["blink/renderer/modules/accessibility/", "ui/accessibility/", "content/browser/accessibility/"]),
 ("storage: idb/cachestorage/fs/sqlite", ["blink/renderer/modules/indexeddb/", "blink/renderer/modules/cache_storage/", "blink/renderer/modules/file_system_access/", "blink/renderer/modules/filesystem/", "third_party/sqlite/", "components/services/storage/", "storage/browser/"]),
 ("service worker + shared/dedicated workers", ["blink/renderer/modules/service_worker/", "content/browser/service_worker/", "content/browser/worker_host/", "blink/renderer/modules/exported/"]),
 ("perfetto tracing", ["third_party/perfetto/", "services/tracing/"]),
]
tot = collections.Counter(); grand = 0
for line in open(sys.argv[1], errors="replace"):
    m = sc.match(line)
    if not m: continue
    s = int(m.group(2))
    if not s: continue
    p = mn.get(int(m.group(1)), "?").replace("\\","/").lower()
    i = p.find("/obj/"); p = p[i+5:] if i>=0 else "<ext>/"+p.rsplit("/",1)[-1]
    grand += s
    for label, pats in CUTS:
        if any(x in p for x in pats):
            tot[label] += s
print("image total: {:,}\n".format(grand))
print("%-44s %14s %7s" % ("candidate cut", "bytes", "% img"))
print("-"*68)
for label, _ in CUTS:
    v = tot[label]
    print("%-44s %14s %6.1f%%" % (label, "{:,}".format(v), 100.0*v/grand))
