#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
for command in od stat unzip zip base64 awk cmp; do command -v "$command" >/dev/null; done
passed=0
expect_failure() {
    local label=$1 pattern=$2
    shift 2
    if "$@" >"$work/result.log" 2>&1; then
        echo "FAIL: $label unexpectedly passed" >&2; exit 1
    fi
    grep -Eq "$pattern" "$work/result.log" || { cat "$work/result.log" >&2; exit 1; }
    printf 'PASS: rejected %s\n' "$label"
    passed=$((passed + 1))
}
# Byte fixtures test the envelope guard only; they are not Android execution.
printf '%s' 'UEsDBBQAAAAAAAAAIVyXGlfbOgAAADoAAAATAAAAQW5kcm9pZE1hbmlmZXN0LnhtbHVuc2lnbmVkIFpJUC1lbnZlbG9wZSB0ZXN0IGZpeHR1cmUsIG5vdCBhbiBpbnN0YWxsYWJsZSBBUEtQSwECFAMUAAAAAAAAACFclxpX2zoAAAA6AAAAEwAAAAAAAAAAAAAAgAEAAAAAQW5kcm9pZE1hbmlmZXN0LnhtbFBLBQYAAAAAAQABAEEAAABrAAAAAAA=' | base64 -d > "$work/unsigned.apk"
printf '%s' 'UEsDBBQAAAAAAAAAIVyXGlfbOgAAADoAAAATAAAAQW5kcm9pZE1hbmlmZXN0LnhtbHVuc2lnbmVkIFpJUC1lbnZlbG9wZSB0ZXN0IGZpeHR1cmUsIG5vdCBhbiBpbnN0YWxsYWJsZSBBUEsYAAAAAAAAABgAAAAAAAAAQVBLIFNpZyBCbG9jayA0MlBLAQIUAxQAAAAAAAAAIVyXGlfbOgAAADoAAAATAAAAAAAAAAAAAACAAQAAAABBbmRyb2lkTWFuaWZlc3QueG1sUEsFBgAAAAABAAEAQQAAAIsAAAAAAA==' | base64 -d > "$work/block.apk"
printf '%s' 'UEsDBBQAAAAAAAAAIVyXGlfbOgAAADoAAAATAAAAQW5kcm9pZE1hbmlmZXN0LnhtbHVuc2lnbmVkIFpJUC1lbnZlbG9wZSB0ZXN0IGZpeHR1cmUsIG5vdCBhbiBpbnN0YWxsYWJsZSBBUEv/AAAAAAAAABgAAAAAAAAAQVBLIFNpZyBCbG9jayA0MlBLAQIUAxQAAAAAAAAAIVyXGlfbOgAAADoAAAATAAAAAAAAAAAAAACAAQAAAABBbmRyb2lkTWFuaWZlc3QueG1sUEsFBgAAAAABAAEAQQAAAIsAAAAAAA==' | base64 -d > "$work/broken-block.apk"
bash "$root/verify-unsigned.sh" "$work/unsigned.apk"
expect_failure 'APK signing block' 'APK signing block present' bash "$root/verify-unsigned.sh" "$work/block.apk"
expect_failure 'malformed signing block' 'APK signing block present' bash "$root/verify-unsigned.sh" "$work/broken-block.apk"
cp "$work/unsigned.apk" "$work/trailing.apk"
printf x >> "$work/trailing.apk"
expect_failure 'trailing data' 'ZIP footer' bash "$root/verify-unsigned.sh" "$work/trailing.apk"
printf 'not a ZIP' > "$work/truncated.apk"
expect_failure 'truncation' 'ZIP size' bash "$root/verify-unsigned.sh" "$work/truncated.apk"
mkdir -p "$work/META-INF"
printf 'not a valid signature' > "$work/META-INF/TEST.SF"
cp "$work/unsigned.apk" "$work/jar-signed.apk"
(cd "$work" && zip -q jar-signed.apk META-INF/TEST.SF)
expect_failure 'JAR signature record' 'JAR signature records' bash "$root/verify-unsigned.sh" "$work/jar-signed.apk"
expected=29.0.14206865
sdk="$work/sdk"
for version in "$expected" 27.3.13750724; do
    mkdir -p "$sdk/ndk/$version/build/cmake"
    printf 'Pkg.Revision = %s\n' "$version" > "$sdk/ndk/$version/source.properties"
    printf '# fixture\n' > "$sdk/ndk/$version/build/cmake/android.toolchain.cmake"
done
selected=$(env -u NDK_ROOT ANDROID_NDK_HOME="$sdk/ndk/27.3.13750724" ANDROID_NDK="$sdk/ndk/27.3.13750724" bash "$root/select-ndk.sh" "$sdk" "$expected")
[[ $selected == "$sdk/ndk/$expected" ]]
printf 'PASS: inherited NDK 27 cannot override declared NDK 29\n'
selected=$(NDK_ROOT="$sdk/ndk/$expected" bash "$root/select-ndk.sh" "$sdk" "$expected")
[[ $selected == "$sdk/ndk/$expected" ]]
expect_failure 'explicit wrong NDK' 'NDK version mismatch' env NDK_ROOT="$sdk/ndk/27.3.13750724" bash "$root/select-ndk.sh" "$sdk" "$expected"
expect_failure 'missing NDK evidence' 'NDK version evidence missing' env NDK_ROOT="$sdk/missing" bash "$root/select-ndk.sh" "$sdk" "$expected"
printf 'Pkg.Revision = %s\n' "$expected" >> "$sdk/ndk/$expected/source.properties"
expect_failure 'duplicate NDK evidence' 'exactly one Pkg.Revision' env NDK_ROOT="$sdk/ndk/$expected" bash "$root/select-ndk.sh" "$sdk" "$expected"
printf 'PASS: release guard regressions; %s negative cases and positive controls\n' "$passed"
