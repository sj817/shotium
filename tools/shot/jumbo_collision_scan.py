#!/usr/bin/env python3
"""Find Jumbo symbol collisions before the compiler does.

    python tools/shot/jumbo_collision_scan.py out/ProbeLinux out/Shot

Jumbo concatenates translation units, so two files that each define the same
internal-linkage name -- a file-scope static in C, a type or function inside an
anonymous namespace in C++ -- stop being separate and the second becomes a
redefinition of the first. Which files share a chunk follows the source list,
so the same latent collision fires on one platform and not another. Several
turned up on Linux one CI round at a time, twenty to thirty minutes each, none
of them visible from a Windows build.

The second argument is a build directory for a platform that is known to
compile, and it is what makes the output short enough to read. Every pair of
files sharing a chunk there is already proved not to collide, so only pairs the
target platform newly puts together can be a problem. Without it the scan
reports around a hundred candidates that Windows compiles every day.

Candidates are also limited to translation units the binary actually compiles.
In a Jumbo build `ninja -t inputs` names the generated .cc files rather than
the original sources, so that is the form to match; a chunk that is in the
graph but not in shot's inputs -- ui/events/ozone/evdev, for one -- costs
nothing to skip and would otherwise be several findings to chase.

What it cannot tell you, both learned by checking its output against the
source:

  * It compares names, not signatures, so legal overloads look like
    collisions. AdjustAlongAxis takes an int in rect.cc and a float in
    rect_f.cc; _serialize_cff1_charstrings takes three parameters in one
    harfbuzz file and four in another. Read the declarations before acting.

  * It reads symbols, not macros. Skia's Vulkan backend broke a Linux build
    with VK_CALL defined two different ways in one chunk, and nothing here
    would have seen it.

Files under a DEPS checkout are not present on a host that does not check them
out -- third_party/fontconfig/src on Windows, for instance -- so they cannot be
scanned. They are counted and named rather than passed over silently, because
a collision there is exactly as real: fontconfig's `static free_lock` in
fccfg.c and fccache.c had to be found by building.
"""
import os
import re
import subprocess
import sys
from collections import defaultdict

ROOT = 'D:/Github/chromium'
OUT = sys.argv[1] if len(sys.argv) > 1 else 'out/ProbeLinux'
# The build directory of a platform that is known to compile. Every pair of
# files sharing a chunk there has already been proved not to collide, so only
# pairs this platform newly puts together can be a problem. Without this the
# regex reports a hundred candidates that Windows compiles every day.
BASELINE = sys.argv[2] if len(sys.argv) > 2 else 'out/Shot'
os.chdir(ROOT)

INCLUDE = re.compile(r'^\s*#include\s+"([^"]+)"', re.M)

# A static function or variable at file scope. Both the one-line form and the
# K&R-ish form fontconfig uses, where the type is on its own line.
STATIC_DEF = re.compile(
    r'^static\s+(?:[A-Za-z_][\w:<>,\s\*&]*?)\s*\**\s*'
    r'([A-Za-z_]\w*)\s*[\(\[=;]', re.M)
STATIC_LEADING = re.compile(r'^static\s+[\w\s\*]+$', re.M)

ANON_OPEN = re.compile(r'^namespace\s*\{\s*$', re.M)
ANON_CLOSE = re.compile(r'^\}\s*//\s*namespace\s*$', re.M)
# Inside an anonymous namespace: types, and functions declared as
# "<type> <name>(". Two details keep the false positives down:
#   * the type must end in real whitespace, so static_assert( and
#     BASE_FEATURE( are not read as a type "s" and a name "tatic_assert"
#   * the name must not be preceded by ::, so Class::method defined out of
#     line is a member function and not an internal-linkage name
ANON_NAME = re.compile(
    r'^(?:struct|class|union|enum(?:\s+class)?)\s+([A-Za-z_]\w*)|'
    r'^[A-Za-z_][\w:<>,\s\*&]*?\s+\**\s*(?<![:\w])([A-Za-z_]\w*)\s*\(',
    re.M)


def names_defined(path):
    """Internal-linkage names this file defines, as best a regex can tell."""
    try:
        with open(path, encoding='utf-8', errors='replace') as f:
            text = f.read()
    except OSError:
        return None
    found = set()

    for m in STATIC_DEF.finditer(text):
        found.add(m.group(1))

    # The two-line form: "static void\nfree_lock (void)".
    lines = text.split('\n')
    for i, line in enumerate(lines[:-1]):
        if re.match(r'^static\s+[\w\s\*]+$', line):
            m = re.match(r'^\s*\**\s*([A-Za-z_]\w*)\s*\(', lines[i + 1])
            if m:
                found.add(m.group(1))

    # Anonymous namespace blocks.
    depth_start = None
    for m in ANON_OPEN.finditer(text):
        start = m.end()
        close = ANON_CLOSE.search(text, start)
        block = text[start:close.start()] if close else text[start:]
        for n in ANON_NAME.finditer(block):
            name = n.group(1) or n.group(2)
            if name and name not in ('if', 'for', 'while', 'switch', 'return',
                                     'sizeof', 'return_type'):
                found.add(name)
    return found


def compiled_tus(out_dir, target='shot'):
    """The Jumbo TUs the target actually compiles, build-dir relative.

    Everything else in gen/ is in the graph without being reached, and a
    collision there is not a build failure waiting to happen.
    """
    ninja = os.path.join(ROOT, 'third_party', 'ninja',
                         'ninja.exe' if os.name == 'nt' else 'ninja')
    if not os.path.exists(ninja):
        return None
    r = subprocess.run([ninja, '-C', out_dir, '-t', 'inputs', target],
                       cwd=ROOT, capture_output=True, text=True,
                       errors='replace')
    if r.returncode != 0:
        return None
    return {l.strip().replace('\\', '/') for l in r.stdout.split('\n')
            if '_shot_jumbo_' in l}


wanted = compiled_tus(OUT)
if wanted is None:
    print('could not ask ninja what is compiled; scanning every chunk')
else:
    print('jumbo TUs the target compiles: %d' % len(wanted))

chunks = []
gen = os.path.join(OUT, 'gen')
for base, dirs, files in os.walk(gen):
    for fn in files:
        if '_shot_jumbo_' not in fn or not fn.endswith(('.cc', '.c', '.mm')):
            continue
        full = os.path.join(base, fn)
        if wanted is not None:
            rel = os.path.relpath(full, os.path.join(ROOT, OUT))
            if rel.replace('\\', '/') not in wanted:
                continue
        try:
            with open(full, encoding='utf-8', errors='replace') as f:
                text = f.read()
        except OSError:
            continue
        members = []
        for inc in INCLUDE.findall(text):
            # Written as "../../base/foo.cc". The quoted form is tried
            # relative to the including file first and does not resolve there;
            # what makes it work is -I../.. from the build directory, which is
            # the source root. So strip the ../../ and take what is left as
            # repository-relative.
            rel = inc
            while rel.startswith('../'):
                rel = rel[3:]
            members.append(os.path.join(ROOT, rel.replace('/', os.sep)))
        if len(members) > 1:
            chunks.append((os.path.relpath(full, ROOT).replace('\\', '/'),
                           members))

print('jumbo translation units with more than one member: %d' % len(chunks))


def pairs_of(out_dir):
    """Every unordered pair of files that share a chunk in this build dir."""
    seen = set()
    g = os.path.join(out_dir, 'gen')
    for b, _d, fs in os.walk(g):
        for fn in fs:
            if '_shot_jumbo_' not in fn or not fn.endswith(('.cc', '.c', '.mm')):
                continue
            try:
                with open(os.path.join(b, fn), encoding='utf-8',
                          errors='replace') as f:
                    t = f.read()
            except OSError:
                continue
            mem = []
            for inc in INCLUDE.findall(t):
                r = inc
                while r.startswith('../'):
                    r = r[3:]
                mem.append(r)
            mem.sort()
            for i in range(len(mem)):
                for j in range(i + 1, len(mem)):
                    seen.add((mem[i], mem[j]))
    return seen


baseline_pairs = pairs_of(BASELINE) if os.path.isdir(BASELINE) else set()
print('pairs already proved safe by %s: %d' % (BASELINE, len(baseline_pairs)))

unreadable = defaultdict(int)
collisions = []
for tu, members in chunks:
    per_file = {}
    for m in members:
        names = names_defined(m)
        if names is None:
            rel = os.path.relpath(m, ROOT).replace('\\', '/')
            unreadable['/'.join(rel.split('/')[:3])] += 1
            continue
        per_file[m] = names
    seen = defaultdict(list)
    for path, names in per_file.items():
        for n in names:
            seen[n].append(path)
    for n, paths in seen.items():
        if len(paths) > 1:
            collisions.append((tu, n, sorted(
                os.path.relpath(p, ROOT).replace('\\', '/') for p in paths)))

if unreadable:
    print('\nfiles not on this host (DEPS checkouts) -- not scanned:')
    for d, n in sorted(unreadable.items()):
        print('  %-44s %d file(s)' % (d, n))

print('\n=== candidate collisions: %d ===' % len(collisions))
by_tu = defaultdict(list)
for tu, name, paths in collisions:
    by_tu[tu].append((name, paths))
for tu in sorted(by_tu):
    print('\n%s' % tu)
    for name, paths in sorted(by_tu[tu])[:12]:
        print('    %-34s %s' % (name, ' + '.join(
            p.split('/')[-1] for p in paths)))
    if len(by_tu[tu]) > 12:
        print('    ... and %d more' % (len(by_tu[tu]) - 12))
