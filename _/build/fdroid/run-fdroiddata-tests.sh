#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$repo_root/fdroid/release-values.sh"
toolchain_file="$repo_root/fdroid/toolchain.properties"

toolchain_value() {
    local key="$1"
    sed -n "s/^${key}=//p" "$toolchain_file"
}

output_dir="${FDROID_OUTPUT_DIR:-$repo_root/build/fdroiddata}"
image_ref="${FDROID_BUILDSERVER_IMAGE:-$(toolchain_value buildserverImage)}"
fdroiddata_revision="${FDROIDDATA_REVISION:-$(toolchain_value fdroiddata)}"
fdroidserver_revision="${FDROIDSERVER_REVISION:-$(toolchain_value fdroidserver)}"
source_revision="${SOURCE_REVISION:-$(git -C "$repo_root" rev-parse HEAD)}"
source_repo="${SOURCE_REPO:-https://github.com/isomorphismes/wegert.git}"

command -v docker >/dev/null 2>&1 || {
    echo "docker is required to run F-Droid's production buildserver image" >&2
    exit 1
}
for revision in "$source_revision" "$fdroiddata_revision" "$fdroidserver_revision"; do
    [[ "$revision" =~ ^[0-9a-f]{40}$ ]]
done
test "$(git -C "$repo_root" rev-parse HEAD)" = "$source_revision"

remote_refs="$(git ls-remote "$source_repo")"
grep -Fq "$source_revision" <<< "$remote_refs" || {
    echo "source revision is not the tip of a public ref: $source_revision" >&2
    echo "push the branch first, then rerun this test" >&2
    exit 1
}

mkdir -p "$output_dir"
output_dir="$(cd "$output_dir" && pwd)"
printf 'source_repo=%s\nsource_revision=%s\n' "$source_repo" "$source_revision" > "$output_dir/source-public.txt"

docker pull "$image_ref" >/dev/null
image_digest="$(
    docker image inspect --format '{{range .RepoDigests}}{{println .}}{{end}}' "$image_ref" |
        awk '/@sha256:[0-9a-f]{64}$/ { print; exit }'
)"
[[ "$image_digest" =~ @sha256:[0-9a-f]{64}$ ]] || {
    echo "could not resolve buildserver image to an immutable digest: $image_ref" >&2
    exit 1
}

cat > "$output_dir/toolchain.txt" <<EOF
fdroiddata=$fdroiddata_revision
fdroidserver=$fdroidserver_revision
buildserver_image=$image_digest
package=$WEGERT_PACKAGE_ID
version_name=$WEGERT_VERSION_NAME
version_code=$WEGERT_VERSION_CODE
EOF

for run in 1 2; do
    run_dir="$output_dir/run-$run"
    rm -rf "$run_dir"
    mkdir -p "$run_dir"
    docker run --rm         --volume "$repo_root:/workspace/wegert:ro"         --volume "$run_dir:/output"         --env SOURCE_REPO="$source_repo"         --env SOURCE_REVISION="$source_revision"         --env FDROIDDATA_REVISION="$fdroiddata_revision"         --env FDROIDSERVER_REVISION="$fdroidserver_revision"         "$image_digest"         /workspace/wegert/fdroid/test-inside-buildserver.sh
done

apk_name="${WEGERT_PACKAGE_ID}_${WEGERT_VERSION_CODE}.apk"
first="$output_dir/run-1/$apk_name"
second="$output_dir/run-2/$apk_name"
test -s "$first"
test -s "$second"

if ! cmp -s "$first" "$second"; then
    sha256sum "$first" "$second" >&2
    echo "two clean F-Droid buildserver runs produced different APK bytes" >&2
    exit 1
fi

cmp "$output_dir/run-1/buildserver-image-revision.txt"     "$output_dir/run-2/buildserver-image-revision.txt"

cp "$first" "$output_dir/$apk_name"
sha256sum "$output_dir/$apk_name" > "$output_dir/$apk_name.sha256"
sha256sum "$first" "$second" > "$output_dir/reproducibility.txt"
printf 'byte-identical clean F-Droid buildserver rebuilds: %s\n' "$output_dir/$apk_name"
