#!/usr/bin/env python3
"""Compare two Shot builds through cold CLI, serve, and the C ABI.

The benchmark keeps all image output in memory or a temporary directory and
prints one JSON document. It is intentionally dependency-free so release
builders can use it before packaging.
"""

import argparse
import ctypes
import hashlib
import json
import os
import pathlib
import statistics
import struct
import subprocess
import sys
import tempfile
import time


ROOT = pathlib.Path(__file__).resolve().parents[2]
CORPUS = ROOT / "shot" / "testdata" / "render_corpus.html"


def percentile(values, percent):
    ordered = sorted(values)
    position = (len(ordered) - 1) * percent / 100
    lower = int(position)
    upper = min(lower + 1, len(ordered) - 1)
    fraction = position - lower
    return ordered[lower] * (1 - fraction) + ordered[upper] * fraction


def distribution(values):
    return {
        "p50": statistics.median(values),
        "p95": percentile(values, 95),
        "mean": statistics.fmean(values),
    }


def working_set(process_handle):
    if sys.platform != "win32":
        return None

    class ProcessMemoryCounters(ctypes.Structure):
        _fields_ = [
            ("cb", ctypes.c_uint32),
            ("PageFaultCount", ctypes.c_uint32),
            ("PeakWorkingSetSize", ctypes.c_size_t),
            ("WorkingSetSize", ctypes.c_size_t),
            ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
            ("QuotaPagedPoolUsage", ctypes.c_size_t),
            ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
            ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
            ("PagefileUsage", ctypes.c_size_t),
            ("PeakPagefileUsage", ctypes.c_size_t),
        ]

    counters = ProcessMemoryCounters()
    counters.cb = ctypes.sizeof(counters)
    get_memory = ctypes.windll.psapi.GetProcessMemoryInfo
    get_memory.argtypes = [ctypes.c_void_p,
                           ctypes.POINTER(ProcessMemoryCounters),
                           ctypes.c_uint32]
    get_memory.restype = ctypes.c_int
    ok = get_memory(ctypes.c_void_p(int(process_handle)),
                    ctypes.byref(counters), counters.cb)
    if not ok:
        return None
    return {
        "workingSet": counters.WorkingSetSize,
        "peakWorkingSet": counters.PeakWorkingSetSize,
    }


def current_process_handle():
    if sys.platform != "win32":
        return None
    get_current_process = ctypes.windll.kernel32.GetCurrentProcess
    get_current_process.restype = ctypes.c_void_p
    return get_current_process()


def read_exact(stream, size):
    result = bytearray()
    while len(result) < size:
        chunk = stream.read(size - len(result))
        if not chunk:
            raise RuntimeError("Shot closed its response stream")
        result.extend(chunk)
    return bytes(result)


def serve_capture(process, request):
    payload = json.dumps(request, separators=(",", ":")).encode()
    started = time.perf_counter()
    process.stdin.write(struct.pack("<I", len(payload)))
    process.stdin.write(payload)
    process.stdin.flush()
    header_size = struct.unpack("<I", read_exact(process.stdout, 4))[0]
    header = json.loads(read_exact(process.stdout, header_size))
    image_size = struct.unpack("<I", read_exact(process.stdout, 4))[0]
    image = read_exact(process.stdout, image_size)
    elapsed = (time.perf_counter() - started) * 1000
    if not header.get("ok"):
        raise RuntimeError(header.get("error", "capture failed"))
    return elapsed, header["stats"], image


def summarize_samples(samples):
    timings = {"wall": [sample["wall"] for sample in samples]}
    for name in ("fetch", "render", "setup", "wait", "lifecycle",
                 "paint", "raster", "encode", "total"):
        timings[name] = [sample["timing"][name] for sample in samples]
    result = {name: distribution(values) for name, values in timings.items()}
    result["throughput"] = 1000 * len(samples) / sum(timings["wall"])
    result["bytes"] = statistics.median(sample["bytes"] for sample in samples)
    return result


def compare_serve(builds, request, warmups, count):
    processes = {}
    for label, executable in builds.items():
        processes[label] = subprocess.Popen(
            [executable, "--serve", "--allow-file-access"],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL)
    try:
        for _ in range(warmups):
            for process in processes.values():
                serve_capture(process, request)

        memory_after_warmup = {
            label: working_set(process._handle)
            for label, process in processes.items()
        }
        samples = {label: [] for label in builds}
        digests = {label: set() for label in builds}
        labels = list(builds)
        for index in range(count):
            order = labels if index % 2 == 0 else reversed(labels)
            for label in order:
                wall, stats, image = serve_capture(processes[label], request)
                samples[label].append({
                    "wall": wall,
                    "timing": stats["timing"],
                    "bytes": len(image),
                })
                digests[label].add(hashlib.sha256(image).hexdigest())

        result = {label: summarize_samples(values)
                  for label, values in samples.items()}
        for label, process in processes.items():
            result[label]["memoryAfterWarmup"] = memory_after_warmup[label]
            result[label]["memoryAfterSamples"] = working_set(process._handle)
            result[label]["sha256"] = sorted(digests[label])
        return result
    finally:
        for process in processes.values():
            if process.stdin:
                process.stdin.close()
        for process in processes.values():
            process.wait(timeout=120)


def compare_cold_cli(builds, count):
    samples = {label: [] for label in builds}
    digests = {label: set() for label in builds}
    labels = list(builds)
    with tempfile.TemporaryDirectory(prefix="shot-perf-cli-") as temporary:
        for index in range(count):
            order = labels if index % 2 == 0 else reversed(labels)
            for label in order:
                output = pathlib.Path(temporary, f"{label}.png")
                started = time.perf_counter()
                subprocess.run([
                    builds[label], "--file", CORPUS, "--width", "1248",
                    "--height", "1320", "--output", output,
                ], check=True, stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL)
                samples[label].append((time.perf_counter() - started) * 1000)
                digests[label].add(
                    hashlib.sha256(output.read_bytes()).hexdigest())
    return {
        label: {**distribution(values), "sha256": sorted(digests[label])}
        for label, values in samples.items()
    }


def c_api_child(library_path, warmups, count):
    out_dir = pathlib.Path(library_path).resolve().parent
    if sys.platform == "win32":
        os.add_dll_directory(str(out_dir))
    library = ctypes.CDLL(str(pathlib.Path(library_path).resolve()))
    pointer = ctypes.c_void_p
    library.shot_engine_create.argtypes = [
        ctypes.c_char_p, ctypes.POINTER(pointer), ctypes.POINTER(pointer)]
    library.shot_engine_create.restype = ctypes.c_int32
    library.shot_engine_capture.argtypes = [
        pointer, ctypes.c_char_p, ctypes.POINTER(pointer),
        ctypes.POINTER(pointer), ctypes.POINTER(pointer)]
    library.shot_engine_capture.restype = ctypes.c_int32
    library.shot_engine_destroy.argtypes = [pointer]
    library.shot_buffer_data.argtypes = [pointer]
    library.shot_buffer_data.restype = ctypes.POINTER(ctypes.c_uint8)
    library.shot_buffer_size.argtypes = [pointer]
    library.shot_buffer_size.restype = ctypes.c_size_t
    library.shot_buffer_free.argtypes = [pointer]

    def buffer_bytes(value):
        if not value.value:
            return b""
        return ctypes.string_at(library.shot_buffer_data(value),
                                library.shot_buffer_size(value))

    engine = pointer()
    error = pointer()
    options = json.dumps({
        "resourceDir": str(out_dir),
        "allowFileAccess": True,
    }).encode()
    started = time.perf_counter()
    status = library.shot_engine_create(
        options, ctypes.byref(engine), ctypes.byref(error))
    create_ms = (time.perf_counter() - started) * 1000
    if status:
        message = buffer_bytes(error).decode("utf-8", "replace")
        raise RuntimeError(f"shot_engine_create failed: {message}")

    request = json.dumps({
        "file": str(CORPUS),
        "width": 1248,
        "height": 1320,
        "allowFileAccess": True,
    }, separators=(",", ":")).encode()
    samples = []
    digests = set()

    def capture(keep):
        image = pointer()
        stats = pointer()
        error = pointer()
        started = time.perf_counter()
        status = library.shot_engine_capture(
            engine, request, ctypes.byref(image), ctypes.byref(stats),
            ctypes.byref(error))
        wall = (time.perf_counter() - started) * 1000
        if status:
            message = buffer_bytes(error).decode("utf-8", "replace")
            raise RuntimeError(f"shot_engine_capture failed: {message}")
        image_bytes = buffer_bytes(image)
        stats_value = json.loads(
            buffer_bytes(stats).rstrip(b"\0").decode("utf-8"))
        if keep:
            samples.append({
                "wall": wall,
                "timing": stats_value["timing"],
                "bytes": len(image_bytes),
            })
            digests.add(hashlib.sha256(image_bytes).hexdigest())
        for value in (image, stats, error):
            if value.value:
                library.shot_buffer_free(value)

    try:
        for _ in range(warmups):
            capture(False)
        memory_after_warmup = working_set(current_process_handle())
        for _ in range(count):
            capture(True)
        result = summarize_samples(samples)
        result["create"] = create_ms
        result["memoryAfterWarmup"] = memory_after_warmup
        result["memoryAfterSamples"] = working_set(current_process_handle())
        result["sha256"] = sorted(digests)
        return result
    finally:
        library.shot_engine_destroy(engine)


def compare_c_api(libraries, warmups, count):
    results = {}
    for label, library in libraries.items():
        completed = subprocess.run([
            sys.executable, __file__, "--c-api-child", library,
            "--warmups", str(warmups), "--samples", str(count),
        ], check=True, text=True, stdout=subprocess.PIPE,
           stderr=subprocess.PIPE)
        results[label] = json.loads(completed.stdout)
    return results


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("first")
    parser.add_argument("second", nargs="?")
    parser.add_argument("--first-label", default="first")
    parser.add_argument("--second-label", default="second")
    parser.add_argument("--first-dll")
    parser.add_argument("--second-dll")
    parser.add_argument("--samples", type=int, default=100)
    parser.add_argument("--cold-samples", type=int, default=30)
    parser.add_argument("--warmups", type=int, default=10)
    parser.add_argument("--c-api-child", action="store_true")
    args = parser.parse_args()

    if args.c_api_child:
        print(json.dumps(c_api_child(args.first, args.warmups,
                                     args.samples)))
        return
    if not args.second:
        parser.error("second executable is required")

    builds = {
        args.first_label: str(pathlib.Path(args.first).resolve()),
        args.second_label: str(pathlib.Path(args.second).resolve()),
    }
    request = {
        "file": str(CORPUS),
        "width": 1248,
        "height": 1320,
        "allowFileAccess": True,
    }
    result = {
        "coldCli": compare_cold_cli(builds, args.cold_samples),
        "serve": compare_serve(builds, request, args.warmups, args.samples),
    }
    if args.first_dll and args.second_dll:
        result["cApi"] = compare_c_api({
            args.first_label: str(pathlib.Path(args.first_dll).resolve()),
            args.second_label: str(pathlib.Path(args.second_dll).resolve()),
        }, args.warmups, args.samples)
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
