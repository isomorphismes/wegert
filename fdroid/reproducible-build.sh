#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "$repo_root/fdroid/release-values.sh"

output_dir="${1:-$repo_root/build/reproducible-fdroid}"
source_revision="${SOURCE_REVISION:-HEAD}"
source_date_epoch="$(git -C "$repo_root" show -s --format=%ct "$source_revision")"

mkdir -p "$output_dir"
output_dir="$(cd "$output_dir" && pwd)"

for checkout in "$repo_root/_deps/Idric" "$repo_root/_deps/idric-arm-thumb"; do
    git -C "$checkout" rev-parse --verify HEAD >/dev/null
    test -n "$(git -C "$checkout" rev-parse HEAD)"
done

build_once() {
    local run="$1"
    local result="$output_dir/run-$run.apk"
    local dex_receipt="$output_dir/run-$run.classes.dex.sha256"

    rm -rf "$repo_root/build/direct"
    rm -rf "$repo_root/_deps/idric-arm-thumb/build"
    rm -f "$repo_root/complex_math_ick.o" "$repo_root/app/wegert-debug.keystore"

    (
        cd "$repo_root"
        export SOURCE_DATE_EPOCH="$source_date_epoch"
        bash android-direct/generate-dex.sh build/direct/classes.dex
        sha256sum build/direct/classes.dex > "$dex_receipt"
        bash android-direct/build.sh \
            build/direct/classes.dex \
            build/direct/wegert-direct-unsigned.apk
        cp build/direct/wegert-direct-unsigned.apk "$result"
    )

    "$repo_root/fdroid/verify-apk.sh" "$result"
}

build_once 1
build_once 2

cmp "$output_dir/run-1.classes.dex.sha256" "$output_dir/run-2.classes.dex.sha256"

if ! cmp -s "$output_dir/run-1.apk" "$output_dir/run-2.apk"; then
    sha256sum "$output_dir/run-1.apk" "$output_dir/run-2.apk" >&2
    if command -v diffoscope >/dev/null 2>&1; then
        diffoscope "$output_dir/run-1.apk" "$output_dir/run-2.apk" \
            > "$output_dir/diffoscope.txt" || true
        echo "APK mismatch; see $output_dir/diffoscope.txt" >&2
    else
        echo "APK mismatch; install diffoscope for a structural report" >&2
    fi
    exit 1
fi

cp "$output_dir/run-1.apk" "$output_dir/wegert-$WEGERT_VERSION_NAME-fdroid-unsigned.apk"
sha256sum "$output_dir/wegert-$WEGERT_VERSION_NAME-fdroid-unsigned.apk" \
    > "$output_dir/wegert-$WEGERT_VERSION_NAME-fdroid-unsigned.apk.sha256"
printf 'byte-identical direct DEX/JNI builds: %s\n' "$output_dir/wegert-$WEGERT_VERSION_NAME-fdroid-unsigned.apk"
