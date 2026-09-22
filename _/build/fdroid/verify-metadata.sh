#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$repo_root/fdroid/release-values.sh"

source_revision="${SOURCE_REVISION:-$(git -C "$repo_root" rev-parse HEAD)}"
template="$repo_root/fdroid/$WEGERT_PACKAGE_ID.yml.template"
play="$repo_root/app/src/main/play"
locale="$play/listings/en-US"
release_note="$play/release-notes/en-US/default.txt"
icon="$locale/graphics/icon/icon.png"
screenshots_dir="$locale/graphics/phone-screenshots"
builder="$repo_root/fdroid/build-apk.sh"

[[ "$source_revision" =~ ^[0-9a-f]{40}$ ]]

if [[ -e "$repo_root/fastlane/metadata/android" ]]; then
    echo "legacy Fastlane metadata must not coexist with canonical Triple-T metadata" >&2
    exit 1
fi

scratch="$(mktemp -d "${TMPDIR:-/tmp}/wegert-fdroid-metadata.XXXXXX")"
trap 'rm -rf "$scratch"' EXIT
metadata="$scratch/$WEGERT_PACKAGE_ID.yml"
"$repo_root/fdroid/render-metadata.sh" "$source_revision" "$metadata"

metadata_version_name="$(sed -n 's/^  - versionName: \(.*\)$/\1/p' "$metadata" | tail -n 1)"
metadata_version_code="$(sed -n 's/^    versionCode: \([0-9][0-9]*\)$/\1/p' "$metadata" | tail -n 1)"
metadata_commit="$(sed -n 's/^    commit: \(.*\)$/\1/p' "$metadata" | tail -n 1)"
metadata_ndk="$(sed -n 's/^    ndk: \(.*\)$/\1/p' "$metadata" | tail -n 1)"

test "$metadata_version_name" = "$WEGERT_VERSION_NAME"
test "$metadata_version_code" = "$WEGERT_VERSION_CODE"
test "$metadata_commit" = "$source_revision"
test "$metadata_ndk" = "$WEGERT_NDK_VERSION"
grep -Fxq "CurrentVersion: $WEGERT_VERSION_NAME" "$metadata"
grep -Fxq "CurrentVersionCode: $WEGERT_VERSION_CODE" "$metadata"
grep -Fxq 'AuthorName: isomorphisms' "$metadata"
grep -Fxq 'AuthorEmail: isomorphisms@sdf.org' "$metadata"
grep -Fxq 'AuthorWebSite: https://x.com/isomorph_nix' "$metadata"
grep -Fxq 'SourceCode: https://github.com/isomorphismes/wegert' "$metadata"
grep -Fxq 'Repo: https://github.com/isomorphismes/wegert.git' "$metadata"
grep -Fxq 'AutoUpdateMode: Version' "$metadata"
grep -Fxq 'UpdateCheckMode: Tags ^v[0-9]+\.[0-9]+\.[0-9]+$' "$metadata"
grep -Fxq 'UpdateCheckData: _/build/fdroid/release.properties|versionCode=([0-9]+)|.|versionName=([0-9.]+)' "$metadata"
grep -Fxq '    subdir: _/build' "$metadata"
grep -Fxq '    build: SDK_ROOT="$$SDK$$" NDK_ROOT="$$NDK$$" bash fdroid/build-apk.sh' "$metadata"
grep -Fxq "      - sdkmanager \"platforms;android-$WEGERT_TARGET_SDK\" \"build-tools;$WEGERT_BUILD_TOOLS\"" "$metadata"
grep -Fxq "      - sdkmanager \"cmake;$WEGERT_CMAKE_VERSION\"" "$metadata"

if grep -Eq '^[[:space:]]+(gradle|gradleprops):' "$metadata"; then
    echo "F-Droid metadata must not invoke Gradle" >&2
    exit 1
fi

for removed in     _/build/app/build.gradle.kts     _/build/app/wegert-debug.keystore     _/build/build.gradle.kts     _/build/complex_math_ick.o     _/build/gradle     _/build/settings.gradle.kts; do
    grep -Fxq "      - $removed" "$metadata"
done

for required in title.txt short-description.txt full-description.txt; do
    test -s "$locale/$required"
done
test -s "$release_note"
test -s "$icon"

mapfile -t screenshots < <(
    if [[ -d "$screenshots_dir" ]]; then
        find "$screenshots_dir" -maxdepth 1 -type f -name '*.png' -print | sort
    fi
)

test "$(wc -m < "$locale/title.txt")" -le 51
test "$(wc -m < "$locale/short-description.txt")" -le 81
test "$(wc -m < "$locale/full-description.txt")" -le 4001
test "$(wc -m < "$release_note")" -le 501

python3 - "$icon" "${screenshots[@]}" <<'PY'
import struct
import sys
from pathlib import Path

def png_size(path: Path) -> tuple[int, int]:
    with path.open("rb") as stream:
        header = stream.read(24)
    if header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR":
        raise SystemExit(f"not a PNG: {path}")
    return struct.unpack(">II", header[16:24])

icon = Path(sys.argv[1])
if png_size(icon) != (512, 512):
    raise SystemExit(f"F-Droid icon must be 512x512: {icon}")
for raw in sys.argv[2:]:
    path = Path(raw)
    width, height = png_size(path)
    if width < 1 or height < 1:
        raise SystemExit(f"empty screenshot: {path} ({width}x{height})")
PY

for abi in arm64-v8a armeabi-v7a x86_64; do
    grep -Fq "$abi" "$builder"
done
grep -Fq -- '-DWEGERT_USE_ICK_PREBUILT=OFF' "$builder"
grep -Fq 'aapt2' "$builder"
grep -Fq 'zipalign' "$builder"

if grep -Eq '<uses-permission[^>]+android.permission.INTERNET' "$repo_root/app/src/main/AndroidManifest.xml"; then
    echo "Triple-T description says there is no network permission, but the manifest requests it" >&2
    exit 1
fi

printf 'release: %s %s (%s), source %s\n' "$WEGERT_PACKAGE_ID" "$WEGERT_VERSION_NAME" "$WEGERT_VERSION_CODE" "$source_revision"
printf 'metadata: canonical Triple-T; %d phone screenshot(s)\n' "${#screenshots[@]}"
printf 'F-Droid build: direct NDK/CMake + aapt2, no Gradle\n'
