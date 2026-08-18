"""Delete calls to macros whose definition is gone.

A dangling macro call is not a one-line problem. These are statement macros
spanning several lines:

    TRACE_SCHEDULE_STYLE_INVALIDATION(element, *descendants, ClassChange,
                                      class_name);

and some of them are wrapped in an `if (UNLIKELY(...))` that exists only to
guard them. Deleting the first line leaves the arguments behind as a syntax
error three lines later, which is why this walks parentheses instead of using a
regular expression.

What it does NOT do is define the macro away to nothing. A no-op macro leaves
every call site reading as though instrumentation is still happening, and the
arguments still have to name types that may themselves be cut. If the consumer
is gone, the call goes.

Usage:
  cpp_drop_macro_calls.py --macro NAME [--call Class::Method ...] <file> ...
  cpp_drop_macro_calls.py --list-undefined <file> [<file> ...]
"""

import io
import re
import sys

CALL = re.compile(r"^([ \t]*)([A-Z][A-Z0-9_]{6,})\(", re.M)
# Same shape, but for a qualified static call used as a statement:
#
#     AuditsIssue::ReportMixedContentIssue(a, b,
#                                          c);
#
# which is what a reporting facade looks like once its consumer is cut.
QUALIFIED = re.compile(r"^([ \t]*)([A-Za-z_]\w*::[A-Za-z_]\w*)\(", re.M)


def call_extent(text, open_paren):
    """Return the index just past the `);` closing the call, or None."""
    depth = 0
    i = open_paren
    n = len(text)
    while i < n:
        c = text[i]
        if c == '"' or c == "'":
            quote = c
            i += 1
            while i < n and text[i] != quote:
                i += 2 if text[i] == "\\" else 1
        elif c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                j = i + 1
                while j < n and text[j] in " \t":
                    j += 1
                if j < n and text[j] == ";":
                    j += 1
                while j < n and text[j] in " \t":
                    j += 1
                if text.startswith("\r\n", j):
                    j += 2
                elif j < n and text[j] == "\n":
                    j += 1
                return j
        i += 1
    return None


def drop(text, macros, pattern=CALL):
    """Remove every call to one of `macros`. Returns (text, count)."""
    dropped = 0
    while True:
        for m in pattern.finditer(text):
            if m.group(2) not in macros:
                continue
            end = call_extent(text, m.end() - 1)
            if end is None:
                continue
            indent = m.group(1)
            note = "%s// %s(...) was here.\n" % (indent, m.group(2))
            if "\r\n" in text:
                note = note.replace("\n", "\r\n")
            text = text[:m.start()] + note + text[end:]
            dropped += 1
            break
        else:
            return text, dropped


def main(argv):
    macros = set()
    calls = set()
    files = []
    it = iter(argv)
    list_only = False
    for a in it:
        if a == "--macro":
            macros.add(next(it))
        elif a == "--call":
            calls.add(next(it))
        elif a == "--list-undefined":
            list_only = True
        else:
            files.append(a)
    if not files or (not macros and not calls and not list_only):
        sys.exit(__doc__)

    for path in files:
        src = io.open(path, encoding="utf-8", newline="").read()
        if list_only:
            names = sorted({m.group(2) for m in CALL.finditer(src)})
            print("%s: %s" % (path, " ".join(names)))
            continue
        out, n = drop(src, macros)
        if calls:
            out, k = drop(out, calls, QUALIFIED)
            n += k
        if n:
            io.open(path, "w", encoding="utf-8", newline="").write(out)
        print("  %-72s %d dropped" % (path, n))


if __name__ == "__main__":
    main(sys.argv[1:])
