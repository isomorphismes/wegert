#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

if [[ "$#" -ne 1 ]]; then
    echo "usage: $0 APK" >&2
    exit 2
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "$repo_root/fdroid/release-values.sh"

apk="$1"
test -s "$apk"
unzip -t "$apk" >/dev/null

scratch="$(mktemp -d "${TMPDIR:-/tmp}/wegert-apk.XXXXXX")"
trap 'rm -rf "$scratch"' EXIT
unzip -Z1 "$apk" > "$scratch/files"

for required in \
    classes.dex \
    assets/wegert.frag \
    assets/licenses/WEGERT_LICENSE.txt \
    assets/licenses/NOTICE.txt \
    lib/arm64-v8a/libwegert.so \
    lib/armeabi-v7a/libwegert.so \
    lib/x86_64/libwegert.so; do
    grep -Fxq "$required" "$scratch/files"
done

test "$(grep -Fxc classes.dex "$scratch/files")" -eq 1
if grep -Eq '[.]class$' "$scratch/files"; then
    echo "direct APK unexpectedly contains JVM .class files" >&2
    exit 1
fi

sed -n 's,^lib/\([^/][^/]*\)/.*$,\1,p' "$scratch/files" | sort -u > "$scratch/abis"
printf '%s\n' arm64-v8a armeabi-v7a x86_64 | sort > "$scratch/expected-abis"
cmp "$scratch/expected-abis" "$scratch/abis"

unzip -p "$apk" assets/licenses/WEGERT_LICENSE.txt \
    | grep -Fq 'SPDX-License-Identifier: GPL-3.0-or-later'
unzip -p "$apk" assets/licenses/NOTICE.txt \
    | grep -Fq 'Android native_app_glue'
unzip -p "$apk" assets/licenses/NOTICE.txt \
    | grep -Fq 'Apache License, Version 2.0'

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
grep -Fq "launchable-activity: name='org.isomorphisms.wegert.WegertActivity'" "$scratch/badging"

sha256sum "$apk"
