#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$repo_root/fdroid/release-values.sh"

appid=org.isomorphisms.wegert
metadata="$repo_root/fdroid/$appid.yml.template"
locale="$repo_root/fastlane/metadata/android/en-US"
builder="$repo_root/fdroid/build-apk.sh"

metadata_version_name="$(sed -n 's/^  - versionName: \(.*\)$/\1/p' "$metadata" | tail -n 1)"
metadata_version_code="$(sed -n 's/^    versionCode: \([0-9][0-9]*\)$/\1/p' "$metadata" | tail -n 1)"
metadata_commit="$(sed -n 's/^    commit: \(.*\)$/\1/p' "$metadata" | tail -n 1)"
metadata_output="$(sed -n 's/^    output: \(.*\)$/\1/p' "$metadata" | tail -n 1)"
current_version_name="$(sed -n 's/^CurrentVersion: \(.*\)$/\1/p' "$metadata")"
current_version_code="$(sed -n 's/^CurrentVersionCode: \([0-9][0-9]*\)$/\1/p' "$metadata")"
metadata_ndk="$(sed -n 's/^    ndk: \(.*\)$/\1/p' "$metadata" | tail -n 1)"

test "$metadata_version_name" = "$WEGERT_VERSION_NAME"
test "$metadata_version_code" = "$WEGERT_VERSION_CODE"
test "$metadata_commit" = "v$WEGERT_VERSION_NAME"
test "$metadata_output" = "_/build/build/fdroid/wegert-release-unsigned.apk"
test "$current_version_name" = "$WEGERT_VERSION_NAME"
test "$current_version_code" = "$WEGERT_VERSION_CODE"
test "$metadata_ndk" = "$WEGERT_NDK_VERSION"

grep -Fxq 'SourceCode: https://github.com/isomorphismes/wegert' "$metadata"
grep -Fxq 'Repo: https://github.com/isomorphismes/wegert.git' "$metadata"
grep -Fxq 'AutoUpdateMode: Version' "$metadata"
grep -Fxq 'UpdateCheckMode: Tags ^v[0-9]+\.[0-9]+\.[0-9]+$' "$metadata"
grep -Fxq 'UpdateCheckData: _/build/fdroid/release.properties|versionCode=([0-9]+)|.|versionName=([0-9.]+)' "$metadata"
grep -Fxq '    build: SDK_ROOT="$$SDK$$" NDK_ROOT="$$NDK$$" bash _/build/fdroid/build-apk.sh' "$metadata"

if grep -Eq '^[[:space:]]+(gradle|gradleprops):' "$metadata"; then
    echo "F-Droid metadata must not invoke Gradle" >&2
    exit 1
fi

for removed in \
    _/build/app/build.gradle.kts \
    _/build/build.gradle.kts \
    _/build/gradle \
    _/build/settings.gradle.kts; do
    grep -Fxq "      - $removed" "$metadata"
done

for abi in arm64-v8a armeabi-v7a x86_64; do
    grep -Fq "$abi" "$builder"
done
grep -Fq -- '-DWEGERT_USE_ICK_PREBUILT=OFF' "$builder"
grep -Fq 'aapt2' "$builder"
grep -Fq 'zipalign' "$builder"

for required in \
    title.txt \
    short_description.txt \
    full_description.txt \
    "changelogs/$WEGERT_VERSION_CODE.txt" \
    images/icon.png; do
    test -s "$locale/$required"
done

mapfile -t screenshots < <(find "$locale/images/phoneScreenshots" -maxdepth 1 -type f -name '*.png' -print | sort)
test "${#screenshots[@]}" -ge 1

test "$(wc -m < "$locale/title.txt")" -le 51
test "$(wc -m < "$locale/short_description.txt")" -le 81
test "$(wc -m < "$locale/full_description.txt")" -le 4001
test "$(wc -m < "$locale/changelogs/$WEGERT_VERSION_CODE.txt")" -le 501

python3 - "$locale/images/icon.png" "${screenshots[@]}" <<'PY'
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

for raw_path in sys.argv[2:]:
    path = Path(raw_path)
    width, height = png_size(path)
    if width < 1 or height < 1:
        raise SystemExit(f"empty screenshot: {path} ({width}x{height})")
PY

if grep -Eq '<uses-permission[^>]+android.permission.INTERNET' "$repo_root/app/src/main/AndroidManifest.xml"; then
    echo "Fastlane description says there is no network permission, but the manifest requests it" >&2
    exit 1
fi

printf 'metadata: %s (%s), tag auto-update enabled\n' "$WEGERT_VERSION_NAME" "$WEGERT_VERSION_CODE"
printf 'F-Droid build: direct NDK/CMake + aapt2, no Gradle\n'
printf 'fastlane: title, descriptions, icon, changelog, and %d phone screenshot(s)\n' "${#screenshots[@]}"
printf 'apk packaging: one universal APK with arm64-v8a, armeabi-v7a, and x86_64\n'
