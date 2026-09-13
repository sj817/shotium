#!/bin/sh
set -eu

if [ "$#" -lt 2 ]; then
  echo "usage: check-linux-deps.sh <glibc|musl> <elf>..." >&2
  exit 2
fi

libc=$1
shift
case "$libc" in glibc|musl) ;; *) echo "unsupported libc: $libc" >&2; exit 2;; esac

readelf_bin=${SHOTIUM_READELF:-readelf}
report=out/ffi-check/dependencies/report.txt
mkdir -p "$(dirname "$report")"
: > "$report"

for binary in "$@"; do
  test -f "$binary" || { echo "missing ELF: $binary" >&2; exit 1; }
  needed=$("$readelf_bin" --dynamic "$binary" |
    sed -n 's/.*Shared library: \[\([^]]*\)\].*/\1/p')
  interpreter=$("$readelf_bin" --program-headers "$binary" |
    sed -n 's/.*Requesting program interpreter: \([^]]*\)\].*/\1/p')
  {
    echo "$binary"
    if [ -n "$interpreter" ]; then printf '  interpreter: %s\n' "$interpreter"; fi
    if [ -n "$needed" ]; then printf '  %s\n' $needed; else echo "  (no DT_NEEDED entries)"; fi
  } | tee -a "$report"

  if [ -n "$interpreter" ]; then
    case "$libc:$interpreter" in
      glibc:/lib64/ld-linux-x86-64.so.2|glibc:/lib/ld-linux-aarch64.so.1) ;;
      musl:/lib/ld-musl-x86_64.so.1|musl:/lib/ld-musl-aarch64.so.1) ;;
      *) echo "$binary has the wrong program interpreter for $libc: $interpreter" >&2; exit 1 ;;
    esac
  fi

  unexpected=
  for dependency in $needed; do
    case "$libc:$dependency" in
      glibc:libc.so.6|glibc:libm.so.6|glibc:libdl.so.2|glibc:libpthread.so.0|glibc:librt.so.1|glibc:libresolv.so.2|glibc:libutil.so.1|glibc:ld-linux-x86-64.so.2|glibc:ld-linux-aarch64.so.1) ;;
      musl:libc.musl-*.so.1|musl:ld-musl-*.so.1) ;;
      *) unexpected="$unexpected $dependency" ;;
    esac
  done
  if [ -n "$unexpected" ]; then
    echo "$binary has non-libc runtime dependencies:$unexpected" >&2
    exit 1
  fi
done

echo "PASS: only the selected libc runtime remains" | tee -a "$report"
