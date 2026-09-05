#!/usr/bin/env python3
"""Check paired images independently of timing, retaining every failed case."""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageChops, ImageStat


def verify(result_path):
    result_path = Path(result_path)
    result = json.loads(result_path.read_text(encoding="utf-8"))
    checks = []
    for case in result["cases"]:
        record = {"name": case["name"], "status": "error"}
        checks.append(record)
        try:
            images = {}
            for label in ["baseline", "candidate"]:
                sample = case["samples"][label][0]["workers"][0]
                hashes = {worker["sha256"] for row in case["samples"][label]
                          for worker in row["workers"]}
                assert hashes == {sample["sha256"]}, "Output changed within the static case; every variant needs image verification"
                file = Path(sample["evidence"])
                if not file.is_file():
                    basename = sample["evidence"].replace("\\", "/").rsplit("/", 1)[-1]
                    file = Path(str(result_path) + ".images") / case["name"] / basename
                assert hashlib.sha256(file.read_bytes()).hexdigest() == sample["sha256"], "Evidence hash mismatch"
                with Image.open(file) as image:
                    image.load()
                    images[label] = image.convert("RGBA")
            a, b = images["baseline"], images["candidate"]
            assert a.size == b.size, f"Dimensions changed: {a.size} -> {b.size}"
            assert max(ImageStat.Stat(a).stddev[:3]) > 1, "Baseline appears blank"
            assert max(ImageStat.Stat(b).stddev[:3]) > 1, "Candidate appears blank"
            difference = ImageChops.difference(a, b)
            mean = ImageStat.Stat(difference).mean
            histogram = difference.histogram()
            changed = sum(sum(histogram[channel * 256 + 17:(channel + 1) * 256])
                          for channel in range(4)) / (a.width * a.height * 4)
            record.update(dimensions=list(a.size), mean_channel_error=mean,
                          fraction_channels_error_above_16=changed)
            # Fixed before sampling. Allows sparse antialiasing/codec rounding;
            # no resizing or cropping that could hide missing rows or pictures.
            assert max(mean) <= 1 and changed <= 0.01, "Paired pixels differ beyond tolerance"
            record["status"] = "pass"
        except (OSError, AssertionError, KeyError, IndexError) as error:
            record["error"] = str(error)
    required = set(result["selectedCases"])
    passed = {check["name"] for check in checks if check["status"] == "pass"}
    output = {"result_sha256": hashlib.sha256(result_path.read_bytes()).hexdigest(),
              "status": "pass" if passed == required else "not-passed", "checks": checks}
    result_path.with_suffix(".pixels.json").write_text(json.dumps(output, indent=2), encoding="utf-8")
    print(f"Paired images: {len(passed)}/{len(required)} passed")
    for check in checks:
        if check["status"] != "pass":
            print(check["name"], check.get("error"))
    return output["status"] == "pass"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("result")
    raise SystemExit(0 if verify(parser.parse_args().result) else 1)
