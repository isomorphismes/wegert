#!/usr/bin/env bash
# Source-built ICK owned C + explicit NDK assembly/link, direct APKs.
set -Eeuo pipefail
repo=$(git -C "$(dirname -- "$0")" rev-parse --show-toplevel)
: "${CATFOOD_CHECKOUT:?pinned Cat Food checkout required}"
: "${ANDROID_NDK_REPO:?pinned android-NDK packager required}"
: "${AICI_CHECKOUT:?pinned AICI policy checkout required}"
: "${ANDROID_NDK_HOME:?NDK r29 required}"
: "${ANDROID_HOME:?Android SDK required}"
: "${WEGERT_ICK_OBJECT:?source-built AArch64 ICK object required}"
out="${1:?output directory required}"
source_sha=$(git -C "$repo" rev-parse HEAD)
mkdir -p "$out"
plan="$out/android-plan.tsv"
sh "$CATFOOD_CHECKOUT/catfood" plan-android-build \
    "$repo/_/build/ci/android-application.tsv" phone "$source_sha" > "$plan"
# The plan must require both targets. Unsupported/unknown C67 may not vanish.
grep -Fxq $'packaging\tsplit' "$plan"
grep -Fxq $'phone\tMIRO_A1\tarmeabi-v7a\trequired\tnot_run' "$plan"
grep -Fxq $'c67\tMIRO_C67\tarm64-v8a\trequired\tnot_run' "$plan"
test -s "$WEGERT_ICK_OBJECT"
test -f "$ANDROID_NDK_REPO/apk/build-nativeactivity-apk.sh"
source "$repo/_/build/fdroid/release-values.sh"
expected_cert=$(awk -F '\t' '
    $1=="org.isomorphisms.wegert" && $2=="test" {n++; v=$3}
    END {if(n!=1) exit 2; print v}
' "$AICI_CHECKOUT/android-signing/identities.tsv")
test -n "$expected_cert"
export ANDROID_PACKAGE_ID=org.isomorphisms.wegert
export ANDROID_VERSION_CODE="$WEGERT_VERSION_CODE"
export ANDROID_VERSION_NAME="$WEGERT_VERSION_NAME"
export ANDROID_MIN_SDK="$WEGERT_MIN_SDK"
export ANDROID_TARGET_SDK="$WEGERT_TARGET_SDK"
export ANDROID_KEYSTORE="$repo/_/build/app/wegert-debug.keystore"
export ANDROID_KEYSTORE_TYPE=PKCS12
export ANDROID_KEY_ALIAS=wegert-debug
export ANDROID_STORE_PASSWORD=wegert-debug
export ANDROID_KEY_PASSWORD=wegert-debug
export ANDROID_EXPECTED_CERT_SHA256="$expected_cert"
export ANDROID_EXPECTED_LABEL='zero & infinity'
export ANDROID_SOURCE_COMMIT="$source_sha"
export ANDROID_RES_DIR="$repo/_/build/app/src/main/res"
export ANDROID_ASSET_DIR="$out/assets"
export ANDROID_BUILD_TOOLS="$ANDROID_HOME/build-tools/$WEGERT_BUILD_TOOLS"
mkdir -p "$ANDROID_ASSET_DIR/licenses"
bash "$repo/_/build/android-direct/assemble-shader.sh" "$ANDROID_ASSET_DIR/wegert.frag"
cp "$repo/LICENSE" "$ANDROID_ASSET_DIR/licenses/WEGERT_LICENSE.txt"
cp "$repo/NOTICE" "$ANDROID_ASSET_DIR/licenses/NOTICE.txt"
ndk_bin="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin"
for tool in llvm-strip llvm-readelf llvm-nm; do test -x "$ndk_bin/$tool"; done
cp "$WEGERT_ICK_OBJECT" "$repo/_/build/complex_math_ick.o"
cmp "$WEGERT_ICK_OBJECT" "$repo/_/build/complex_math_ick.o"
{
    printf 'schema\twegert-paired-nativeactivity-candidate-v1\n'
    printf 'source_sha\t%s\n' "$source_sha"
    printf 'catfood_plan_sha256\t%s\n' "$(sha256sum "$plan" | awk '{print $1}')"
    printf 'catfood_commit\t%s\n' "$(git -C "$CATFOOD_CHECKOUT" rev-parse HEAD)"
    printf 'packager_commit\t%s\n' "$(git -C "$ANDROID_NDK_REPO" rev-parse HEAD)"
    printf 'aici_commit\t%s\n' "$(git -C "$AICI_CHECKOUT" rev-parse HEAD)"
    printf 'ick_object_sha256\t%s\n' "$(sha256sum "$WEGERT_ICK_OBJECT" | awk '{print $1}')"
    printf 'ick_aarch64_object\tPASS\n'
    printf 'owned_c_ick_compile\tREQUIRED\n'
    printf 'ndk_glue_assembly_link\tREQUIRED\n'
    printf 'armv7_ick_physical_runtime\tNOT_VERIFIED\n'
    printf 'a1_physical\tNOT_VERIFIED\nc67_physical\tNOT_VERIFIED\n'
} > "$out/producer.receipt.tsv"
for abi in armeabi-v7a arm64-v8a; do
    with_ick=OFF
    if [ "$abi" = arm64-v8a ]; then with_ick=ON; fi
    dir="$out/cmake/$abi"
    cmake -S "$repo/_/build" -B "$dir" \
        -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$abi" -DANDROID_PLATFORM=android-26 \
        -DANDROID_STL=none -DWEGERT_USE_ICK_PREBUILT="$with_ick" \
        -DWEGERT_ENABLE_DIRECT_JNI=OFF -DCMAKE_BUILD_TYPE=Release
    cmake --build "$dir" --target wegert --parallel 2
    lib="$dir/libwegert.so"
    test -s "$lib"
    "$ndk_bin/llvm-readelf" -h "$lib" > "$out/$abi.elf.txt"
    "$ndk_bin/llvm-nm" -D --defined-only "$lib" |
        grep -q 'ANativeActivity_onCreate'
    case "$abi" in
        armeabi-v7a) grep -Eq 'Machine:.*ARM$' "$out/$abi.elf.txt" ;;
        arm64-v8a) grep -Eq 'Machine:.*AArch64' "$out/$abi.elf.txt" ;;
    esac
    size_before=$(wc -c < "$lib")
    "$ndk_bin/llvm-strip" --strip-debug "$lib"
    size_after=$(wc -c < "$lib")
    printf '%s\t%s\t%s\n' "$abi" "$size_before" "$size_after" >> "$out/native-sizes.tsv"
    apk="$out/wegert-$abi.apk"
    bash "$ANDROID_NDK_REPO/apk/build-nativeactivity-apk.sh" \
        "$repo/_/build/nativeactivity/AndroidManifest.xml" "$lib" "$abi" "$apk"
    test -s "$apk"
    grep -Fxq $'launcher_label\tzero & infinity' "${apk%.apk}.receipt.tsv"
    printf '%s\t%s\n' "$abi" "$(sha256sum "$apk" | awk '{print $1}')" >> "$out/apk-sha256.tsv"
done
test "$(wc -l < "$out/apk-sha256.tsv")" -eq 2
printf '%s\n' 'WEGERT_PAIRED_NATIVEACTIVITY=CANDIDATE_BUILT'
