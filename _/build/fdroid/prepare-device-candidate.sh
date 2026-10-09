#!/usr/bin/env bash
set -euo pipefail
[[ $# -eq 2 ]] || { echo 'usage: prepare-device-candidate.sh UNSIGNED_APK OUTPUT_DIR' >&2; exit 2; }
build_root=$(cd "$(dirname "$0")/.." && pwd)
source "$build_root/fdroid/release-values.sh"
unsigned=$(realpath "$1")
mkdir -p "$2"
output=$(cd "$2" && pwd)
source_sha=$(git -C "$build_root" rev-parse HEAD)
[[ $source_sha == "${SOURCE_REVISION:?exact source revision required}" ]]
bash "$build_root/fdroid/verify-apk.sh" "$unsigned"
sdk_root=${SDK_ROOT:-${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}}
export ANDROID_HOME="$sdk_root"
export ANDROID_BUILD_TOOLS="$sdk_root/build-tools/$WEGERT_BUILD_TOOLS"
candidate="$output/zero-infinity-$WEGERT_VERSION_NAME-device-candidate.apk"
[[ $unsigned != "$candidate" ]]
unsigned_sha=$(sha256sum "$unsigned" | awk '{print $1}')
bash "$build_root/android-direct/sign-debug.sh" "$unsigned" "$candidate"
[[ $(sha256sum "$unsigned" | awk '{print $1}') == "$unsigned_sha" ]]

# Signing may add signature metadata, never change application entry bytes.
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
unzip -Z1 "$unsigned" | sort > "$work/unsigned.entries"
unzip -Z1 "$candidate" | grep -Eiv '^META-INF/(MANIFEST[.]MF|[^/]+[.](RSA|DSA|EC|SF))$' | sort > "$work/signed.entries"
cmp "$work/unsigned.entries" "$work/signed.entries"
while IFS= read -r entry; do
    unzip -p "$unsigned" "$entry" > "$work/unsigned.entry"
    unzip -p "$candidate" "$entry" > "$work/signed.entry"
    cmp "$work/unsigned.entry" "$work/signed.entry"
done < "$work/unsigned.entries"

# Exercise the unsigned guard against an actually signed APK, not just fixtures.
if bash "$build_root/fdroid/verify-unsigned.sh" "$candidate" > "$work/rejection.log" 2>&1; then
    echo 'Signed device candidate escaped the unsigned release guard' >&2; exit 1
fi
grep -Eq 'APK signing block present|JAR signature records present' "$work/rejection.log"
"$ANDROID_BUILD_TOOLS/apksigner" verify --print-certs "$candidate" > "$output/signing-certificate.txt"
{
    printf 'source_sha=%s\n' "$source_sha"
    printf 'unsigned_apk_sha256=%s\n' "$unsigned_sha"
    printf 'candidate_apk_sha256=%s\n' "$(sha256sum "$candidate" | awk '{print $1}')"
    printf 'version_name=%s\nversion_code=%s\n' "$WEGERT_VERSION_NAME" "$WEGERT_VERSION_CODE"
    printf 'ndk_revision=%s\n' "$WEGERT_NDK_VERSION"
    printf 'signing=test-only\npayload_comparison=PASS\nphysical_device=NOT_RUN\n'
} > "$output/candidate.properties"
(cd "$output" && sha256sum "$(basename "$candidate")" > "$(basename "$candidate").sha256")
printf 'Prepared exact F-Droid payload for physical testing: %s\n' "$candidate"
