#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -Eeuo pipefail

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output=${1:-"$repo_root/build/direct/classes.dex"}
backend="$repo_root/_deps/idric-arm-thumb"
idric_repo="$repo_root/_deps/Idric"
support_root="$idric_repo/_"
compiler="$support_root/build/exec/idris2"

for checkout in "$backend" "$idric_repo"; do
  [[ -d $checkout/.git || -f $checkout/.git ]] || {
    echo "missing initialized source submodule: $checkout" >&2
    exit 1
  }
done

expected_backend=$(git -C "$repo_root" ls-tree HEAD _deps/idric-arm-thumb | awk '{print $3}')
expected_compiler=$(git -C "$repo_root" ls-tree HEAD _deps/Idric | awk '{print $3}')
actual_backend=$(git -C "$backend" rev-parse HEAD)
actual_compiler=$(git -C "$idric_repo" rev-parse HEAD)
[[ -n $expected_backend && $actual_backend == "$expected_backend" ]] || {
  echo "DEX backend submodule is not at the source-pinned commit" >&2
  exit 1
}
[[ -n $expected_compiler && $actual_compiler == "$expected_compiler" ]] || {
  echo "Idriç compiler submodule is not at the source-pinned commit" >&2
  exit 1
}

if [[ ! -x $compiler ]]; then
  "$support_root/edric" bootstrap
fi
"$compiler" --version
make -C "$support_root/support/chez" install IDRIS2_VERSION=0.8.0
make -C "$support_root" install-bootstrap-libs IDRIS2="$compiler"
make -C "$support_root" install-api IDRIS2_BOOT="$compiler"

make -C "$backend" check \
  IDRIC="$compiler" \
  IDRIC_REPO="$idric_repo" \
  IDRIC_COMPILER_REF="$actual_compiler"

mkdir -p "$(dirname -- "$output")"
IDRIC="$compiler" \
  bash "$backend/tests/dex/wegert-generate-direct.sh" "$output"
