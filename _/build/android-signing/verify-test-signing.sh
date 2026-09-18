#!/usr/bin/env bash
set -Eeuo pipefail

build_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
keystore="$build_root/app/wegert-debug.keystore"
expected_sha256=de9b1d47c5a65e6d46a204b79dd9ee566b9d3c9832ba81ebc4213d3392e92ff9

fail() {
  printf 'stable Wegert test signing identity check failed: %s\n' "$*" >&2
  exit 1
}

[[ -f $keystore ]] || fail "missing $keystore"
command -v keytool >/dev/null 2>&1 || fail 'keytool is required'

key_sha256=$(
  keytool -exportcert \
    -keystore "$keystore" \
    -storetype PKCS12 \
    -storepass wegert-debug \
    -alias wegert-debug \
    2>/dev/null |
    sha256sum |
    awk '{print $1}'
)

[[ $key_sha256 == "$expected_sha256" ]] ||
  fail "public test key certificate changed: $key_sha256"

if (($# == 0)); then
  printf 'WEGERT_TEST_SIGNER_SHA256\t%s\n' "$key_sha256"
  exit 0
fi

apk=$1
[[ -f $apk ]] || fail "missing APK: $apk"

android_home=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}
build_tools=${ANDROID_BUILD_TOOLS:-}
[[ -n $android_home ]] || fail 'ANDROID_HOME/ANDROID_SDK_ROOT is required'
if [[ -z $build_tools ]]; then
  build_tools=$(find "$android_home/build-tools" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)
fi
apksigner="$build_tools/apksigner"
[[ -x $apksigner ]] || fail "apksigner not found: $apksigner"

apk_sha256=$(
  "$apksigner" verify --verbose --print-certs "$apk" 2>&1 |
    sed -n 's/^.*certificate SHA-256 digest:[[:space:]]*//p' |
    head -n 1 |
    tr '[:upper:]' '[:lower:]' |
    tr -d ':[:space:]'
)

[[ $apk_sha256 == "$expected_sha256" ]] ||
  fail "APK signer changed: ${apk_sha256:-missing}"

printf 'WEGERT_TEST_SIGNER_SHA256\t%s\n' "$apk_sha256"
