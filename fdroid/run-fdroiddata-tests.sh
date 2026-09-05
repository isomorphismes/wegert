#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

# Run the relevant fdroiddata merge-request checks in the same production-like
# buildserver image used by fdroid/fdroiddata.

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output_dir="${FDROID_OUTPUT_DIR:-$repo_root/build/fdroiddata}"
image="${FDROID_BUILDSERVER_IMAGE:-registry.gitlab.com/fdroid/fdroidserver:buildserver-trixie}"
source_revision="${SOURCE_REVISION:-$(git -C "$repo_root" rev-parse HEAD)}"
source_repo="${SOURCE_REPO:-https://github.com/isomorphismes/wegert.git}"

command -v docker >/dev/null 2>&1 || {
    echo "docker is required to run F-Droid's production buildserver image" >&2
    exit 1
}
[[ "$source_revision" =~ ^[0-9a-f]{40}$ ]]

# Fail before pulling the large build image when the exact immutable source is
# not available from the repository that fdroidserver will clone.
remote_refs="$(git ls-remote "$source_repo")"
grep -Fq "$source_revision" <<< "$remote_refs" || {
    echo "source revision is not the tip of a public ref: $source_revision" >&2
    echo "push the branch first, then rerun this test" >&2
    exit 1
}

mkdir -p "$output_dir"
output_dir="$(cd "$output_dir" && pwd)"

docker run --rm \
    --volume "$repo_root:/workspace/wegert:ro" \
    --volume "$output_dir:/output" \
    --env SOURCE_REPO="$source_repo" \
    --env SOURCE_REVISION="$source_revision" \
    "$image" \
    /workspace/wegert/fdroid/test-inside-buildserver.sh
