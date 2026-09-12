#!/usr/bin/env bash
set -euo pipefail

build_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
git_root="$(git -C "$build_root" rev-parse --show-toplevel)"
source "$build_root/fdroid/release-values.sh"

output_dir="${1:-$build_root/build/reproducible-fdroid}"
source_revision="${SOURCE_REVISION:-HEAD}"
work_root="$(mktemp -d "${TMPDIR:-/tmp}/wegert-reproducible.XXXXXX")"
trap 'rm -rf "$work_root"' EXIT

mkdir -p "$output_dir"
output_dir="$(cd "$output_dir" && pwd)"
source_date_epoch="$(git -C "$git_root" show -s --format=%ct "$source_revision")"

build_once() {
    local run="$1"
    local source_dir="$work_root/source"
    local source_build="$source_dir/_/build"
    local result="$output_dir/run-$run.apk"

    rm -rf "$source_dir"
    mkdir -p "$source_dir"
    git -C "$git_root" archive "$source_revision" | tar -x -C "$source_dir"
    rm -rf \
        "$source_build/gradle" \
        "$source_build/gradlew" \
        "$source_build/gradlew.bat" \
        "$source_build/build.gradle.kts" \
        "$source_build/settings.gradle.kts" \
        "$source_build/app/build.gradle.kts"
    rm -f "$source_build/complex_math_ick.o" "$source_build/app/wegert-debug.keystore"

    (
        cd "$source_build"
        SOURCE_DATE_EPOCH="$source_date_epoch" \
        SDK_ROOT="${SDK_ROOT:-${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}}" \
        NDK_ROOT="${NDK_ROOT:-}" \
        bash fdroid/build-apk.sh "$result"
    )

    "$build_root/fdroid/verify-apk.sh" "$result"
}

build_once 1
build_once 2

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
printf 'byte-identical clean manual builds: %s\n' "$output_dir/wegert-$WEGERT_VERSION_NAME-fdroid-unsigned.apk"
