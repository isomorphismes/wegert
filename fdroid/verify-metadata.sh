#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "$repo_root/fdroid/release-values.sh"

appid=org.isomorphisms.wegert
metadata="$repo_root/fdroid/$appid.yml.template"
locale="$repo_root/fastlane/metadata/android/en-US"
direct_native="$repo_root/android-direct/build-native.sh"
direct_manifest="$repo_root/android-direct/AndroidManifest.xml"
legacy_gradle="$repo_root/app/build.gradle.kts"

metadata_license="$(sed -n 's/^License: \(.*\)$/\1/p' "$metadata")"
metadata_version_name="$(sed -n 's/^  - versionName: \(.*\)$/\1/p' "$metadata" | tail -n 1)"
metadata_version_code="$(sed -n 's/^    versionCode: \([0-9][0-9]*\)$/\1/p' "$metadata" | tail -n 1)"
metadata_commit="$(sed -n 's/^    commit: \(.*\)$/\1/p' "$metadata" | tail -n 1)"
metadata_output="$(sed -n 's/^    output: \(.*\)$/\1/p' "$metadata" | tail -n 1)"
current_version_name="$(sed -n 's/^CurrentVersion: \(.*\)$/\1/p' "$metadata")"
current_version_code="$(sed -n 's/^CurrentVersionCode: \([0-9][0-9]*\)$/\1/p' "$metadata")"
legacy_version_name="$(sed -n 's/^val releaseVersionName = "\([^"]*\)"$/\1/p' "$legacy_gradle")"
legacy_version_code="$(sed -n 's/^val releaseVersionCode = \([0-9][0-9]*\)$/\1/p' "$legacy_gradle")"

test "$metadata_license" = GPL-3.0-or-later
test "$metadata_version_name" = "$WEGERT_VERSION_NAME"
test "$metadata_version_code" = "$WEGERT_VERSION_CODE"
test "$metadata_commit" = __SOURCE_COMMIT__
test "$metadata_output" = build/direct/wegert-direct-unsigned.apk
test "$current_version_name" = "$WEGERT_VERSION_NAME"
test "$current_version_code" = "$WEGERT_VERSION_CODE"
test "$legacy_version_name" = "$WEGERT_VERSION_NAME"
test "$legacy_version_code" = "$WEGERT_VERSION_CODE"
grep -Fxq '    submodules: true' "$metadata"
grep -Fq 'bash android-direct/generate-dex.sh build/direct/classes.dex' "$metadata"
grep -Fq 'bash android-direct/build.sh build/direct/classes.dex build/direct/wegert-direct-unsigned.apk' "$metadata"
! grep -Eq 'gradle|assembleRelease|javac|kotlinc|d8|smali' "$metadata"

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

# The direct source build produces one universal APK with these three native
# ABIs. The checked ICK object is deliberately forbidden from this route.
for abi in arm64-v8a armeabi-v7a x86_64; do
    grep -Fq "$abi" "$direct_native"
done
grep -Fq -- '-DWEGERT_USE_ICK_PREBUILT=OFF' "$direct_native"
grep -Fq 'complex_math_ick.o' "$metadata"
grep -Fq 'app/wegert-debug.keystore' "$metadata"

if grep -Eq '<uses-permission[^>]+android.permission.INTERNET' "$direct_manifest"; then
    echo "Fastlane description says there is no network permission, but the direct manifest requests it" >&2
    exit 1
fi

grep -Fq 'org.isomorphisms.wegert' "$direct_manifest"
grep -Fq '.WegertActivity' "$direct_manifest"
grep -Fq 'android:hasCode="true"' "$direct_manifest"

printf 'metadata: %s (%s), %s\n' "$WEGERT_VERSION_NAME" "$WEGERT_VERSION_CODE" "$metadata_license"
printf 'fastlane: title, descriptions, icon, changelog, and %d phone screenshot(s)\n' "${#screenshots[@]}"
printf 'apk packaging: direct DEX/JNI, one APK with arm64-v8a, armeabi-v7a, and x86_64\n'
