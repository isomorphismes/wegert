#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$repo_root/fdroid/release-values.sh"

output_dir="${1:-$repo_root/build/reproducible-fdroid}"
source_revision="${SOURCE_REVISION:-HEAD}"
work_root="$(mktemp -d "${TMPDIR:-/tmp}/wegert-reproducible.XXXXXX")"
trap 'rm -rf "$work_root"' EXIT

mkdir -p "$output_dir"
output_dir="$(cd "$output_dir" && pwd)"
source_date_epoch="$(git -C "$repo_root" show -s --format=%ct "$source_revision")"

build_once() {
    local run="$1"
    local source_dir="$work_root/source"
    local gradle_home="$work_root/gradle-home-$run"
    local result="$output_dir/run-$run.apk"

    rm -rf "$source_dir"
    mkdir -p "$source_dir" "$gradle_home"
    git -C "$repo_root" archive "$source_revision" | tar -x -C "$source_dir"
    rm -f "$source_dir/complex_math_ick.o"
    rm -f "$source_dir/app/wegert-debug.keystore"

    (
        cd "$source_dir"
        SOURCE_DATE_EPOCH="$source_date_epoch" \
        GRADLE_USER_HOME="$gradle_home" \
        gradle --no-daemon --no-build-cache --rerun-tasks \
            -PfdroidBuild=true :app:assembleRelease
    )

    cp "$source_dir/app/build/outputs/apk/release/app-release-unsigned.apk" "$result"
    "$repo_root/fdroid/verify-apk.sh" "$result"
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
printf 'byte-identical clean builds: %s\n' "$output_dir/wegert-$WEGERT_VERSION_NAME-fdroid-unsigned.apk"
