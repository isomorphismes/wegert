#!/usr/bin/env bash
set -euo pipefail

build_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
git_root="$(git -C "$build_root" rev-parse --show-toplevel)"
output_root="${1:-$build_root/build/candidate-v1}"
source_revision="${SOURCE_REVISION:-$(git -C "$git_root" rev-parse HEAD)}"
source_repo="${SOURCE_REPO:-https://github.com/isomorphismes/wegert.git}"

readonly aici_revision=a10b2cb277181f12d379e572eef8d3d674d87d7c
readonly producer_sha256=e0e5a699b1dd2ceae31fb4469eaaaede9788b3dc9d16fc4e867f814698ecf85b
readonly fdroiddata_revision=4498e27635a1c3b737510342c1f2355c25ce0211
readonly fdroidserver_revision=6af4c4216e43d0fcb29e33919cd0fe8fef7e7400
readonly buildserver_image=registry.gitlab.com/fdroid/fdroidserver@sha256:9cb68105642ca4e7b295f0ceab10f069f5b3247dc18fa7c36046e9d81aa469a8
readonly producer_url="https://raw.githubusercontent.com/isomorphisms/ai-ci/$aici_revision/fdroid/produce-candidate-v1.sh"

[[ "$source_revision" =~ ^[0-9a-f]{40}$ ]]
mkdir -p "$output_root"
output_root="$(cd "$output_root" && pwd)"
work_root="$(mktemp -d "${RUNNER_TEMP:-${TMPDIR:-/tmp}}/wegert-candidate-v1.XXXXXX")"
producer="$work_root/produce-candidate-v1.sh"
contract="$output_root/contract.tsv"
receipt="$output_root/receipt.tsv"
receipt_started=false

finish_receipt() {
    local status=$?
    if [[ "$receipt_started" = true && -x "$producer" && -s "$contract" ]]; then
        "$producer" finish "$contract" "$output_root" "$receipt" >/dev/null || true
    fi
    rm -rf "$work_root"
    exit "$status"
}
trap finish_receipt EXIT

command -v curl >/dev/null 2>&1
command -v git >/dev/null 2>&1
command -v docker >/dev/null 2>&1
command -v sha256sum >/dev/null 2>&1

curl --fail --silent --show-error --location "$producer_url" --output "$producer"
printf '%s  %s\n' "$producer_sha256" "$producer" | sha256sum --check --status
chmod +x "$producer"

sed "s/@SOURCE_REVISION@/$source_revision/" \
    "$build_root/fdroid/candidate-v1.contract.tsv.in" > "$contract"

"$producer" init "$contract" "$output_root"
receipt_started=true
"$producer" source "$contract" "$output_root" "$git_root" "$source_repo"

git clone --quiet --filter=blob:none https://gitlab.com/fdroid/fdroiddata.git "$work_root/fdroiddata"
git -C "$work_root/fdroiddata" checkout --quiet --detach "$fdroiddata_revision"
"$producer" toolchain-revision "$contract" "$output_root" fdroiddata "$work_root/fdroiddata"

git clone --quiet --filter=blob:none https://gitlab.com/fdroid/fdroidserver.git "$work_root/fdroidserver"
git -C "$work_root/fdroidserver" checkout --quiet --detach "$fdroidserver_revision"
"$producer" toolchain-revision "$contract" "$output_root" fdroidserver "$work_root/fdroidserver"

docker pull "$buildserver_image" >/dev/null
"$producer" buildserver-image "$contract" "$output_root" "$buildserver_image"

(
    cd "$build_root"
    "$producer" check "$contract" "$output_root" fdroid-build -- \
        env SOURCE_REVISION="$source_revision" SOURCE_REPO="$source_repo" \
        bash fdroid/run-fdroiddata-tests.sh
)

apk="$build_root/build/fdroiddata/org.isomorphisms.wegert_101.apk"
"$producer" artifact "$contract" "$output_root" buildserver-one "$apk"

printf 'candidate receipt: %s\n' "$receipt"
printf 'expected blockers remain explicit: buildserver-two, per-check metadata/scanner rows, APK identity, manual policy, install/launch\n'
