#!/usr/bin/env python3
"""Render complete coverage and failures without selecting the best run."""
import argparse
import hashlib
import json
from pathlib import Path

PLATFORMS = {f"{system}-{arch}" for system in ["linux", "win32", "darwin"]
             for arch in ["x64", "arm64"]}


def report(files, output, platforms=PLATFORMS):
    lines = ["# 本次候选构建与 npm 的完整矩阵验收", "",
             f"验收范围：{', '.join(sorted(platforms))}。只有范围内的完整矩阵、逐项耗时和像素检查全部通过，才满足这份验收。"
             "倒退、未能确定方向、缺测和失败不会相互抵消。", ""]
    seen = set()
    identities = set()
    passing = set()
    for file in files:
        data = json.loads(file.read_text(encoding="utf-8"))
        platform = f'{data["platform"]}-{data["arch"]}'
        if platform not in platforms:
            raise ValueError(f"Unexpected platform outside acceptance scope: {platform}")
        if platform in seen:
            raise ValueError(f"Duplicate platform {platform}; do not select the best repeat")
        seen.add(platform)
        identities.add((data["revision"], data["sourceDiffSha256"], data["harnessSha256"],
                        data["fixtureManifestSha256"]))
        required = set(data["requiredCases"])
        cases = data["cases"]
        observed = {case["name"] for case in cases}
        pixels_file = file.with_suffix(".pixels.json")
        pixels = json.loads(pixels_file.read_text(encoding="utf-8")) if pixels_file.is_file() else {}
        pixel_ok = (pixels.get("status") == "pass" and
                    pixels.get("result_sha256") == hashlib.sha256(file.read_bytes()).hexdigest())
        complete = (data.get("complete") and data.get("shard") == "all" and
                    observed == required and len(cases) == len(required))
        accepted = sum(bool(case.get("accepted")) for case in cases)
        counts = {status: sum(case["status"] == status for case in cases)
                  for status in ["faster", "equivalent", "slower", "unproven"]}
        external = sum(case.get("class") == "external" for case in cases)
        calibration = data.get("calibration", {})
        band = calibration.get("tolerance", 0.0)
        # One band for the body and one for the tail; an older result carried
        # a single number for both.
        body_band, tail_band = ((band.get("primary", 0.0), band.get("tail", 0.0))
                                if isinstance(band, dict) else (band, band))
        band_source = calibration.get("source", "未记录")
        sampling = data.get("sampling", {})
        if complete and accepted == len(required) and pixel_ok:
            passing.add(platform)
        lines += [f"## {platform}", "",
                  f"覆盖：{len(observed)}/{len(required)}；验收通过：{accepted}/{len(required)}；"
                  f"完整采样：{'是' if complete else '否'}；"
                  f"独立像素检查：{'通过' if pixel_ok else '未通过或未运行'}。", "",
                  f"噪声带：中位/均值 +{body_band * 100:.2f}%，尾部 +{tail_band * 100:.2f}%（{band_source}）。"
                  + (f"采样：每项至少 {sampling.get('minimumPairs')} 对，每侧累计 {sampling.get('minSeconds')} s，"
                     + (f"之后直到 p50 与均值比值区间半宽 ≤ {sampling.get('precision') * 100:g}% 或每侧 {sampling.get('maxSeconds')} s，"
                        if sampling.get('precision') else "")
                     + f"最多 {sampling.get('maxPairs')} 对。" if sampling else "")
                  + f"判定：{counts['faster']} 项更快，{counts['equivalent']} 项等价（噪声带内，不可分辨），"
                  f"{counts['slower']} 项更慢，{counts['unproven']} 项方向未定。"
                  f"引擎场景须「更快」才算通过；{external} 项被外部等待占死的场景（服务端固定延迟、静默窗口）"
                  "只要求不更慢。以下均为毫秒。", "",
                  "| 场景 | 本地 dev | npm | 中位差 | 判定 |",
                  "|---|---:|---:|---:|---|"]
        labels = {"faster": "更快", "equivalent": "等价", "slower": "更慢",
                  "unproven": "未定", "insufficient-samples": "样本不足", "error": "出错"}
        for case in cases:
            summary = case.get("summary")
            if not summary:
                lines.append(f'| {case["name"]} | — | — | — | 未完成 |')
                continue
            dev, npm = summary["candidate"]["p50"], summary["baseline"]["p50"]
            difference = dev - npm
            delta = (f'{"慢" if difference > 0 else "快"} {abs(difference):.2f} ms'
                     if difference else "相同")
            verdict = labels.get(case["status"], case["status"])
            mark = "✓" if case.get("accepted") else "✗"
            lines.append(f'| {case["name"]} | {dev:.2f} ms | {npm:.2f} ms | {delta} | {mark} {verdict} |')
        lines += ["", "<details>", "<summary>完整区间与尾部耗时</summary>", "",
                  "每项指标给出候选/npm 比值的 99% 配对 bootstrap 区间。`faster`：均值整段区间低于 1，或 p50 整段低于 1 且均值落在噪声带内，且没有指标更慢；"
                  "`equivalent`：p50 与均值区间都落在噪声带内、都不低于 1；`slower`：某项区间整段超出它的噪声带；"
                  "`unproven`：p50 或均值区间跨过噪声带边缘。p95 只在整段区间超出尾部带时把一项判为更慢，不做别的。", "",
                  "| 场景 | 样本对 | npm p50 / p95 / mean ms | 本地 dev p50 / p95 / mean ms | 比值区间 p50 / mean / p95 | 状态 |",
                  "|---|---:|---:|---:|---|---|"]
        for case in cases:
            summary = case.get("summary", {})
            wall = case.get("metrics", {}).get("wall", {})
            def numbers(label):
                values = summary.get(label)
                return " / ".join(f"{values[key]:.3f}" for key in ["p50", "p95", "mean"]) if values else "—"
            def spans():
                intervals = wall.get("intervals")
                if not intervals:
                    return "—"
                return " / ".join(f"[{intervals[k]['lo']:.3f}, {intervals[k]['hi']:.3f}]"
                                  for k in ["p50", "mean", "p95"])
            pairs = wall.get("samples", "—")
            lines.append(f'| {case["name"]} | {pairs} | {numbers("baseline")} | {numbers("candidate")} | {spans()} | {case["status"]} |')
            for key, metric in case.get("metrics", {}).items():
                if key != "wall":
                    a, b = metric["summary"]["baseline"], metric["summary"]["candidate"]
                    fmt = lambda row: " / ".join(f"{row[k]:.3f}" for k in ["p50", "p95", "mean"])
                    lines.append(f'| ↳ {key} | — | {fmt(a)} | {fmt(b)} | — | {metric["status"]} |')
        lines += ["", "</details>", "",
                  f'源码基点：`{data["revision"]}`；原生 diff：`{data["sourceDiffSha256"]}`。', ""]
        for label, meta in data["metadata"].items():
            lines.append(f'- {label}：包 {meta["packageVersion"]}；原生库 SHA256 `{meta["librarySha256"]}`。')
        lines += ["", f"原始证据：[{file.name}]({file.resolve().as_posix()})", ""]
    missing = platforms - seen
    lines.insert(4, f"**总体验收：{'通过' if passing == platforms and len(identities) == 1 else '未通过'}。** "
                 f"已验证通过 {len(passing)}/{len(platforms)} 平台。缺少：{', '.join(sorted(missing)) or '无'}。")
    lines += ["新增长图和分片能力还须分别通过 `bilibili_check.py`。"
              "新进程测试包含导入和启动，但操作系统文件缓存未清空；"
              "有限矩阵不能证明任意输入、任意机器和每一次调用均不变慢。", ""]
    output.write_text("\n".join(lines), encoding="utf-8")
    print(lines[4])
    return passing == platforms and len(identities) == 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="+", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--platform", action="append", choices=sorted(PLATFORMS),
                        help="Acceptance platform; repeat to require several (default: all six)")
    args = parser.parse_args()
    raise SystemExit(0 if report(args.files, args.output,
                                set(args.platform or PLATFORMS)) else 1)
