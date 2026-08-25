#!/usr/bin/env bash
set -euo pipefail

if [[ "$#" -ne 1 ]]; then
    echo "usage: $0 APK" >&2
    exit 2
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$repo_root/fdroid/release-values.sh"

apk="$1"
test -s "$apk"
unzip -t "$apk" >/dev/null

scratch="$(mktemp -d "${TMPDIR:-/tmp}/wegert-apk.XXXXXX")"
trap 'rm -rf "$scratch"' EXIT
unzip -Z1 "$apk" > "$scratch/files"

for library in \
    lib/arm64-v8a/libwegert.so \
    lib/armeabi-v7a/libwegert.so \
    lib/x86_64/libwegert.so; do
    grep -Fxq "$library" "$scratch/files"
done

sed -n 's,^lib/\([^/][^/]*\)/.*$,\1,p' "$scratch/files" | sort -u > "$scratch/abis"
printf '%s\n' arm64-v8a armeabi-v7a x86_64 | sort > "$scratch/expected-abis"
cmp "$scratch/expected-abis" "$scratch/abis"

sdk_root="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
if [[ -z "$sdk_root" ]]; then
    echo "ANDROID_SDK_ROOT or ANDROID_HOME is required" >&2
    exit 1
fi
aapt="$sdk_root/build-tools/36.0.0/aapt"
test -x "$aapt"
"$aapt" dump badging "$apk" > "$scratch/badging"
grep -Fq "package: name='org.isomorphisms.wegert' versionCode='$WEGERT_VERSION_CODE' versionName='$WEGERT_VERSION_NAME'" "$scratch/badging"
grep -Fq "application-label:'zero & infinity'" "$scratch/badging"

sha256sum "$apk"
