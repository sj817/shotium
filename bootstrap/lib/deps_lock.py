# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Evaluates a pinned DEPS file with the pinned depot_tools and emits JSON.

This is the machine-readable half of docs/minimal-checkout.md section 7.3. It
deliberately reuses depot_tools' own ``gclient_eval`` rather than
re-implementing DEPS semantics: the condition language, ``deps_os`` folding and
the CIPD/GCS schemas are gclient implementation details that a hand-written
parser gets subtly wrong, and every number in the design doc was produced this
way.

Nothing here touches the network or any checkout. Input is a DEPS blob; output
is a JSON document on stdout or at --out.
"""

import argparse
import hashlib
import json
import os
import sys


def _load_gclient_eval(depot_tools):
    if not os.path.isdir(depot_tools):
        raise SystemExit("depot_tools not found: %s" % depot_tools)
    sys.path.insert(0, depot_tools)
    try:
        import gclient_eval  # noqa: E402
    except ImportError as exc:  # pragma: no cover - environment failure
        raise SystemExit("cannot import gclient_eval from %s: %s" %
                         (depot_tools, exc))
    return gclient_eval


# Mirrors gclient.GClient.get_builtin_vars() for target_os=['win'],
# target_cpu=['x64'], host win/x64. Hard-coded rather than detected: the lock is
# a statement about the intended build, not about whatever machine runs it.
BUILTIN_VARS = {
    "checkout_android": False,
    "checkout_chromeos": False,
    "checkout_fuchsia": False,
    "checkout_ios": False,
    "checkout_linux": False,
    "checkout_mac": False,
    "checkout_win": True,
    "host_os": "win",
    "checkout_arm": False,
    "checkout_arm64": False,
    "checkout_x86": False,
    "checkout_mips": False,
    "checkout_mips64": False,
    "checkout_ppc": False,
    "checkout_riscv64": False,
    "checkout_s390": False,
    "checkout_x64": True,
    "checkout_loong64": False,
    "host_cpu": "x64",
}


def build_variables(gclient_eval, parsed, custom_vars):
    """Reproduces gclient's variable precedence: DEPS < builtin < custom."""
    variables = dict(parsed.get("vars", {}))
    variables.update(BUILTIN_VARS)
    variables.update(custom_vars or {})
    return variables


def evaluate(gclient_eval, condition, variables):
    if not condition:
        return True
    return bool(gclient_eval.EvaluateCondition(condition, variables))


def classify(dep_info):
    return dep_info.get("dep_type", "git")


def gcs_size(dep_info, gclient_eval, variables):
    """Returns (bytes actually fetched here, bytes declared by the entry).

    The two differ and the difference is large. docs/minimal-checkout.md section
    1 reports 2140.23 MiB for the 15 selected GCS entries; that is the declared
    total over every object in those entries. Object-level conditions are ANDed
    with the entry condition, and on Windows x64 they exclude the other-platform
    Rust/LLVM/libclang objects, leaving 1025.94 MiB actually downloaded. Both
    numbers are emitted so a report cannot silently mix them.
    """
    fetched = 0
    declared = 0
    for obj in dep_info.get("objects", []):
        size = int(obj.get("size_bytes", 0))
        declared += size
        if evaluate(gclient_eval, obj.get("condition"), variables):
            fetched += size
    return fetched, declared


def summarize(gclient_eval, parsed, custom_vars, custom_deps):
    """Returns (summary dict, per-dep list) for one configuration."""
    variables = build_variables(gclient_eval, parsed, custom_vars)
    removed = set(custom_deps or [])

    entries = []
    counts = {"git": 0, "cipd": 0, "gcs": 0}
    gcs_bytes = 0
    gcs_declared_bytes = 0
    selected = 0
    removed_active = 0

    for path, info in sorted(parsed.get("deps", {}).items()):
        if info is None:
            entries.append({
                "path": path,
                "type": "none",
                "selected": False,
                "condition": None,
                "removedByCustomDeps": path in removed,
            })
            continue
        condition = info.get("condition")
        is_selected = evaluate(gclient_eval, condition, variables)
        dep_type = classify(info)
        entry = {
            "path": path,
            "type": dep_type,
            "condition": condition,
            "selected": bool(is_selected),
            "removedByCustomDeps": path in removed,
        }
        if dep_type == "git":
            entry["url"] = info.get("url")
        elif dep_type == "cipd":
            entry["packages"] = [{
                "package": p.get("package"),
                "version": p.get("version"),
            } for p in info.get("packages", [])]
        elif dep_type == "gcs":
            entry["bucket"] = info.get("bucket")
            entry["objects"] = [{
                "objectName": o.get("object_name"),
                "sha256sum": o.get("sha256sum"),
                "sizeBytes": o.get("size_bytes"),
                "generation": o.get("generation"),
                "condition": o.get("condition"),
            } for o in info.get("objects", [])]
            fetched, declared = gcs_size(info, gclient_eval, variables)
            entry["sizeBytes"] = fetched
            entry["declaredSizeBytes"] = declared

        if is_selected:
            if path in removed:
                # custom_deps = None only saves anything when the entry would
                # otherwise have been selected; counting requested removals
                # would overstate the saving (doc section 4.3).
                removed_active += 1
            else:
                selected += 1
                counts[dep_type] = counts.get(dep_type, 0) + 1
                if dep_type == "gcs":
                    gcs_bytes += entry["sizeBytes"]
                    gcs_declared_bytes += entry["declaredSizeBytes"]
        entries.append(entry)

    summary = {
        "declared": len(parsed.get("deps", {})),
        "selectedBeforeCustomDeps": selected + removed_active,
        "customDepsRequested": len(removed),
        "customDepsActiveRemovals": removed_active,
        "remaining": selected,
        "git": counts["git"],
        "cipd": counts["cipd"],
        "gcs": counts["gcs"],
        "gcsSizeBytes": gcs_bytes,
        "gcsDeclaredSizeBytes": gcs_declared_bytes,
    }
    return summary, entries


def summarize_hooks(gclient_eval, parsed, custom_vars):
    variables = build_variables(gclient_eval, parsed, custom_vars)
    hooks = []
    for index, hook in enumerate(parsed.get("hooks", [])):
        action = [str(a) for a in hook.get("action", [])]
        condition = hook.get("condition")
        hooks.append({
            "order": index,
            "name": hook.get("name"),
            "pattern": hook.get("pattern"),
            "condition": condition,
            "selected": evaluate(gclient_eval, condition, variables),
            "action": action,
            "actionSha256": hashlib.sha256(
                "\n".join(action).encode("utf-8")).hexdigest(),
        })
    return hooks


def read_custom_deps(path):
    """Reads the candidate list; '#' comments and blank lines are ignored."""
    if not path:
        return []
    out = []
    with open(path, "r", encoding="utf-8") as handle:
        for line in handle:
            line = line.split("#", 1)[0].strip()
            if line:
                out.append(line)
    if len(out) != len(set(out)):
        raise SystemExit("duplicate entries in %s" % path)
    return out


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--deps-file", required=True,
                        help="path to the DEPS blob to evaluate")
    parser.add_argument("--depot-tools", required=True,
                        help="pinned depot_tools checkout")
    parser.add_argument("--custom-deps-file",
                        help="newline separated dep paths to set to None")
    parser.add_argument("--custom-vars", default="{}",
                        help="JSON object of .gclient custom_vars")
    parser.add_argument("--out", help="write JSON here instead of stdout")
    parser.add_argument("--baseline-matrix", action="store_true",
                        help="also emit the default/small/small+null rows that "
                             "docs/minimal-checkout.md section 7.3 pins")
    parser.add_argument("--include-deps", action="store_true",
                        help="include the full per-dependency list")
    args = parser.parse_args(argv)

    gclient_eval = _load_gclient_eval(args.depot_tools)

    with open(args.deps_file, "rb") as handle:
        raw = handle.read()
    deps_sha256 = hashlib.sha256(raw).hexdigest()
    content = raw.decode("utf-8")

    custom_vars = json.loads(args.custom_vars)
    custom_deps = read_custom_deps(args.custom_deps_file)

    parsed = gclient_eval.Parse(content, filename="DEPS",
                                vars_override=custom_vars,
                                builtin_vars=BUILTIN_VARS)

    summary, entries = summarize(gclient_eval, parsed, custom_vars, custom_deps)
    hooks = summarize_hooks(gclient_eval, parsed, custom_vars)

    result = {
        "schema": "shot-deps-lock/1",
        "depsSha256": deps_sha256,
        "depsBytes": len(raw),
        "customVars": custom_vars,
        "customDeps": custom_deps,
        "builtinVars": BUILTIN_VARS,
        "summary": summary,
        "hooks": hooks,
        "hookCount": len(hooks),
        "selectedHookCount": sum(1 for h in hooks if h["selected"]),
    }
    if args.include_deps:
        result["deps"] = entries

    if args.baseline_matrix:
        rows = {}
        for label, cvars, cdeps in (
            ("default", {}, []),
            ("small", {"checkout_configuration": "small"}, []),
            ("small+null", {"checkout_configuration": "small"}, custom_deps),
        ):
            row_parsed = gclient_eval.Parse(content, filename="DEPS",
                                            vars_override=cvars,
                                            builtin_vars=BUILTIN_VARS)
            row_summary, _ = summarize(gclient_eval, row_parsed, cvars, cdeps)
            rows[label] = row_summary
        result["baselineMatrix"] = rows

    text = json.dumps(result, indent=2, sort_keys=True)
    if args.out:
        with open(args.out, "w", encoding="utf-8") as handle:
            handle.write(text)
            handle.write("\n")
    else:
        sys.stdout.write(text + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
