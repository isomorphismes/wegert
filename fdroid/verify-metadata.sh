#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$repo_root/fdroid/release-values.sh"

appid=org.isomorphisms.wegert
metadata="$repo_root/fdroid/$appid.yml.template"
locale="$repo_root/fastlane/metadata/android/en-US"

metadata_version_name="$(sed -n 's/^  - versionName: \(.*\)$/\1/p' "$metadata")"
metadata_version_code="$(sed -n 's/^    versionCode: \([0-9][0-9]*\)$/\1/p' "$metadata")"
metadata_commit="$(sed -n 's/^    commit: \(.*\)$/\1/p' "$metadata")"
current_version_name="$(sed -n 's/^CurrentVersion: \(.*\)$/\1/p' "$metadata")"
current_version_code="$(sed -n 's/^CurrentVersionCode: \([0-9][0-9]*\)$/\1/p' "$metadata")"

test "$metadata_version_name" = "$WEGERT_VERSION_NAME"
test "$metadata_version_code" = "$WEGERT_VERSION_CODE"
test "$metadata_commit" = "v$WEGERT_VERSION_NAME"
test "$current_version_name" = "$WEGERT_VERSION_NAME"
test "$current_version_code" = "$WEGERT_VERSION_CODE"

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

# One assembleRelease output carries all three ABIs. No Android APK splits are
# configured, so this is deliberately not a multiple-APK native-code release.
if grep -R -E -n --include='*.gradle' --include='*.gradle.kts' \
    '(^|[^A-Za-z])splits[[:space:]]*\{' \
    "$repo_root/app" "$repo_root/build.gradle.kts" >/dev/null; then
    echo "unexpected APK split configuration" >&2
    exit 1
fi
for abi in arm64-v8a armeabi-v7a x86_64; do
    grep -Fq "\"$abi\"" "$repo_root/app/build.gradle.kts"
done

if grep -Eq '<uses-permission[^>]+android.permission.INTERNET' "$repo_root/app/src/main/AndroidManifest.xml"; then
    echo "Fastlane description says there is no network permission, but the manifest requests it" >&2
    exit 1
fi

printf 'metadata: %s (%s)\n' "$WEGERT_VERSION_NAME" "$WEGERT_VERSION_CODE"
printf 'fastlane: title, descriptions, icon, changelog, and %d phone screenshot(s)\n' "${#screenshots[@]}"
printf 'apk packaging: one universal APK with arm64-v8a, armeabi-v7a, and x86_64\n'
