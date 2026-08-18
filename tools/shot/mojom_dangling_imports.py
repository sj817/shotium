"""Find `import "..."` lines in .mojom files whose target no longer exists.

mojom_parser resolves every import before generating anything, and it reports
the *first* unresolvable one and stops. A target holding thirty mojoms that each
import a different deleted component therefore costs thirty build rounds to
discover, one name per round -- the same trap as ninja's missing-input check,
one layer up.

This resolves every import in the given directories against the source root and
lists the ones that are gone, so the whole set can be dealt with in one pass.

Imports are always root-relative in chromium, so no include-path guessing is
needed. But some mojoms *are* generated -- blink emits
runtime_feature_state/runtime_feature.mojom and
origin_trials/origin_trial_feature.mojom from its feature lists -- and those
resolve against <out>/gen, not the source root. Checking only the source root
reports them as dangling, which sends you looking through git history for a file
that was never committed. So the gen directory is searched too.

Usage:
  mojom_dangling_imports.py <out-dir> <dir> [<dir> ...]
"""

import os
import re
import sys

ROOT = r"D:\Github\chromium"
IMPORT = re.compile(r'^import "([^"]+)";', re.M)
# An import can be guarded, and a guarded one is never resolved unless the
# feature is on. Reporting them anyway is how this tool ends up listing ten
# ChromeOS mojoms next to the four that actually block the build.
#
#     [EnableIf=is_chromeos]
#     import "chromeos/ash/experiences/arc/mojom/video_decoder.mojom";
#
# Only is_win is passed to mojom_parser here, so anything guarded by another
# feature is reported separately rather than as a blocker.
GUARDED_IMPORT = re.compile(
    r'^\[EnableIf(?:Not)?=(\w+)\]\s*\n\s*import "([^"]+)";', re.M)
ENABLED_FEATURES = {"is_win"}


def main():
    args = sys.argv[1:]
    if len(args) < 2:
        sys.exit(__doc__)
    gen = os.path.join(ROOT, args[0], "gen")
    dirs = args[1:]
    missing = {}
    for d in dirs:
        for dirpath, dirnames, filenames in os.walk(os.path.join(ROOT, d)):
            for fn in filenames:
                if not fn.endswith(".mojom"):
                    continue
                fp = os.path.join(dirpath, fn)
                try:
                    src = open(fp, encoding="utf-8").read()
                except (OSError, UnicodeDecodeError):
                    continue
                guarded = {}
                for feature, imp in GUARDED_IMPORT.findall(src):
                    if feature not in ENABLED_FEATURES:
                        guarded[imp] = feature
                for imp in IMPORT.findall(src):
                    if imp in guarded:
                        continue
                    rel_native = imp.replace("/", os.sep)
                    if os.path.exists(os.path.join(ROOT, rel_native)):
                        continue
                    if os.path.exists(os.path.join(gen, rel_native)):
                        continue  # Generated into <out>/gen.
                    rel = os.path.relpath(fp, ROOT).replace("\\", "/")
                    missing.setdefault(imp, []).append(rel)

    for imp in sorted(missing, key=lambda i: -len(missing[i])):
        print("%4d  %s" % (len(missing[imp]), imp))
        for who in sorted(missing[imp])[:6]:
            print("        %s" % who)
        if len(missing[imp]) > 6:
            print("        ... %d more" % (len(missing[imp]) - 6))
    print("---- %d dangling import(s), %d distinct target(s)"
          % (sum(len(v) for v in missing.values()), len(missing)))


main()
