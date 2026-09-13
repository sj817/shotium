#!/bin/sh
set -eu

: "${PLATFORM:?PLATFORM is required}"
case "$PLATFORM" in
  linux-amd64-musl) cpu=x64 ;;
  linux-arm64-musl) cpu=arm64 ;;
  *) echo "unsupported musl verification target: $PLATFORM" >&2; exit 1 ;;
esac

apk add --no-cache \
  7zip build-base ca-certificates cargo dotnet8-sdk go maven nodejs npm \
  openjdk21-jdk python3 rust
npm install --global pnpm@9.15.9

export DOTNET_CLI_TELEMETRY_OPTOUT=1
export DOTNET_NOLOGO=1
pnpm -C scripts install --frozen-lockfile

sevenzip=$(command -v 7zz || command -v 7z)
for archive in out/ffi-archive/*.7z; do "$sevenzip" x "$archive" -oout/ffi-native -y; done
pnpm package:examples --dest out/language-examples --sevenzip "$sevenzip"
for archive in out/language-examples/*.7z; do "$sevenzip" x "$archive" -oout/language-sources -y; done

pnpm package:cli --os linux --dest "out/ffi-native/shotium-cli-$PLATFORM" --check
pnpm package:c-abi --os linux --dest "out/ffi-native/shotium-c-abi-$PLATFORM" --check
pnpm package:node --os linux --dest "out/ffi-native/shotium-node-$PLATFORM" --check
sh scripts/ci/check-linux-deps.sh musl \
  "out/ffi-native/shotium-cli-$PLATFORM/shotium" \
  "out/ffi-native/shotium-c-abi-$PLATFORM/libshotium.so" \
  "out/ffi-native/shotium-node-$PLATFORM/shotium.node"
pnpm package:platform \
  --from-archive "out/ffi-archive/shotium-node-$PLATFORM.7z" \
  --os linux --arch "$cpu" --libc musl --dest out/ffi-npm-stage \
  --sevenzip "$sevenzip"
mkdir -p out/ffi-npm
(cd "out/ffi-npm-stage/shotium-linux-$cpu-musl" && npm pack --pack-destination ../../ffi-npm)

pnpm -C apps/typescript install --no-lockfile
pnpm -C apps/typescript run build

cli="out/ffi-native/shotium-cli-$PLATFORM/shotium"
pnpm verify:serve "$cli"
pnpm verify:net "$cli"
for language in go python rust csharp java; do
  pnpm verify:ffi \
    --cli "$cli" \
    --library-dir "out/ffi-native/shotium-c-abi-$PLATFORM" \
    --source-dir "out/language-sources/shotium-example-$language" \
    --languages "$language" \
    --output "out/ffi-check/$language"
done
pnpm verify:delivery --platform-dir out/ffi-npm --cli "$cli"
