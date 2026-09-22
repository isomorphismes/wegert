#!/usr/bin/env bash
set -euo pipefail

readonly repo_root=/workspace/wegert
readonly source_repo="${SOURCE_REPO:-https://github.com/isomorphismes/wegert.git}"
readonly source_revision="${SOURCE_REVISION:?SOURCE_REVISION is required}"
readonly fdroiddata_revision="${FDROIDDATA_REVISION:?FDROIDDATA_REVISION is required}"
readonly fdroidserver_revision="${FDROIDSERVER_REVISION:?FDROIDSERVER_REVISION is required}"

source "$repo_root/fdroid/release-values.sh"
readonly appid="$WEGERT_PACKAGE_ID"
readonly build_id="$appid:$WEGERT_VERSION_CODE"
readonly work_root="$(mktemp -d /tmp/wegert-fdroiddata.XXXXXX)"
readonly data="$work_root/fdroiddata"
readonly server="$work_root/fdroidserver"
readonly original_metadata="$work_root/original-metadata.yml"

for revision in "$source_revision" "$fdroiddata_revision" "$fdroidserver_revision"; do
    [[ "$revision" =~ ^[0-9a-f]{40}$ ]]
done

chmod 0755 "$work_root"
cleanup() {
    if [[ -L /home/vagrant/fdroiddata ]]; then rm /home/vagrant/fdroiddata; fi
    rm -f "/home/vagrant/metadata/$appid.yml"
    rm -rf "/home/vagrant/build/$appid" "$work_root"
}
trap cleanup EXIT

mkdir -p /output
if [[ -s /home/vagrant/buildserverid ]]; then
    cat /home/vagrant/buildserverid > /output/buildserver-image-revision.txt
else
    echo "buildserver image does not expose /home/vagrant/buildserverid" >&2
    exit 1
fi

git clone --filter=blob:none https://gitlab.com/fdroid/fdroiddata.git "$data"
git -C "$data" checkout --detach "$fdroiddata_revision"
git clone --filter=blob:none https://gitlab.com/fdroid/fdroidserver.git "$server"
git -C "$server" checkout --detach "$fdroidserver_revision"

"$repo_root/fdroid/render-metadata.sh" "$source_revision" "$data/metadata/$appid.yml"
sed -i "s|^Repo: .*|Repo: $source_repo|" "$data/metadata/$appid.yml"
cp "$data/metadata/$appid.yml" "$original_metadata"
cp "$original_metadata" /output/submitted-metadata.yml

export PATH="$server:$PATH"
export PYTHONPATH="$server:$server/examples"
export PYTHONUNBUFFERED=true
export serverwebroot=/tmp
export ANDROID_HOME=/opt/android-sdk

cd "$data"
chmod 0600 config.yml
find config -type f -name '*.yml' -exec chmod 0600 {} +

apt-get update
apt-get install -y --no-install-recommends     jq     libimage-exiftool-perl     openjdk-21-jdk-headless     python3-markdown-it     python3-pip     python3-yaml     sudo     unzip

run_logged() {
    local witness="$1"
    shift
    {
        printf '$'
        printf ' %q' "$@"
        printf '\n'
        "$@"
    } 2>&1 | tee "/output/$witness"
}

run_logged metadata-read.txt fdroid readmeta
run_logged metadata-lint.txt fdroid lint "$appid"
run_logged metadata-rewrite.txt fdroid rewritemeta "$appid"
cmp "$original_metadata" "metadata/$appid.yml" |& tee /output/metadata-rewrite-no-diff.txt

python3 -m pip install --quiet --break-system-packages check-jsonschema
run_logged metadata-schema.txt check-jsonschema --schemafile schemas/metadata.json "metadata/$appid.yml"

cp "metadata/$appid.yml" "$work_root/before-redirect.yml"
run_logged git-redirect.txt tools/rewrite-git-redirects.py "$appid"
cmp "$work_root/before-redirect.yml" "metadata/$appid.yml" |& tee /output/git-redirect-no-diff.txt

{
    ./tools/check-exif-in-images.sh
    ./tools/check-localized-metadata.py
    ./tools/check-keyalias-collision.py
    ./tools/check-metadata-summary-whitespace.py
    ./tools/check-for-unattached-signatures.py
    ./tools/make-summary-translatable.py
} 2>&1 | tee /output/metadata-tools.txt

python3 tools/check-fastlane.py "$appid" > "$work_root/fastlane.json"
python3 - "$work_root/fastlane.json" <<'PY' | tee /output/fastlane.txt
import json
import sys
reports = json.load(open(sys.argv[1], encoding="utf-8"))
bad = [r for r in reports if r.get("severity") in {"critical", "major"}]
for report in reports:
    print(f"{report.get('severity')}: {report.get('description')}")
if bad:
    raise SystemExit("F-Droid source-metadata checks reported critical or major problems")
PY

release_tag_revision="$(git ls-remote "$source_repo" "refs/tags/$WEGERT_RELEASE_TAG" | awk 'NR == 1 { print $1 }')"
if [[ "$release_tag_revision" = "$source_revision" ]]; then
    cp "metadata/$appid.yml" "$work_root/before-checkupdates.yml"
    run_logged update-check.txt fdroid checkupdates --auto -v "$appid"
    cmp "$work_root/before-checkupdates.yml" "metadata/$appid.yml" |& tee /output/update-check-no-diff.txt
    printf 'tag=%s\nrevision=%s\n' "$WEGERT_RELEASE_TAG" "$release_tag_revision" > /output/tag-binding.txt
else
    printf 'not-verified: tag %s resolves to %s, expected %s\n'         "$WEGERT_RELEASE_TAG" "${release_tag_revision:-missing}" "$source_revision"         > /output/update-check.txt
fi

update-alternatives --set java /usr/lib/jvm/java-21-openjdk-amd64/bin/java
sdkmanager "platform-tools" "build-tools;31.0.0"

if [[ -f /etc/profile.d/bsenv.sh ]]; then
    source /etc/profile.d/bsenv.sh
fi
home_vagrant="${home_vagrant:-/home/vagrant}"
test -d "$home_vagrant"

git -C "$home_vagrant/gradlew-fdroid" pull --ff-only
gradlew_revision="$(git -C "$home_vagrant/gradlew-fdroid" rev-parse HEAD)"
{
    printf 'fdroiddata=%s\n' "$fdroiddata_revision"
    printf 'fdroidserver=%s\n' "$fdroidserver_revision"
    printf 'image_buildserver_revision=%s\n' "$(cat /home/vagrant/buildserverid)"
    printf 'gradlew_fdroid=%s (not used by Wegert direct recipe)\n' "$gradlew_revision"
    printf 'package=%s\n' "$WEGERT_PACKAGE_ID"
    printf 'version=%s (%s)\n' "$WEGERT_VERSION_NAME" "$WEGERT_VERSION_CODE"
    printf 'sdk=%s target=%s build_tools=%s cmake=%s ndk=%s\n'         "$WEGERT_COMPILE_SDK" "$WEGERT_TARGET_SDK" "$WEGERT_BUILD_TOOLS"         "$WEGERT_CMAKE_VERSION" "$WEGERT_NDK_VERSION"
    java -version
} > /output/build-input-pinning.txt 2>&1

mkdir -p "$data/build" "$data/logs" "$data/tmp" "$data/unsigned"
mkdir -p "$home_vagrant/.android" "$home_vagrant/.gradle" "$home_vagrant/metadata"
rm -rf "$home_vagrant/build"
cp -R "$data/build" "$home_vagrant/build"
ln -sfn "$data/tmp" "$home_vagrant/tmp"
ln -sfn "$data/srclibs" "$home_vagrant/srclibs"
cp "metadata/$appid.yml" "$home_vagrant/metadata/"
chown -R vagrant "$home_vagrant" "$data"

run_fdroid() {
    sudo --preserve-env --user vagrant env         PATH="$server:$PATH"         PYTHONPATH="$server:$server/examples"         PYTHONUNBUFFERED=true         TERM="${TERM:-dumb}"         HOME="$home_vagrant"         fdroid "$@"
}

cd "$home_vagrant"
ln -s "$data" "$home_vagrant/fdroiddata"
run_fdroid fetchsrclibs "$build_id" --verbose 2>&1 | tee /output/fetch-source.txt
rm "$home_vagrant/fdroiddata"

(unset CI; run_fdroid build --verbose --test --refresh-scanner --on-server --no-tarball "$build_id")     2>&1 | tee /output/fdroid-build.txt
printf 'fdroid build completed with --refresh-scanner for %s\n' "$build_id" > /output/source-scan.txt

readonly apk="$data/tmp/${appid}_${WEGERT_VERSION_CODE}.apk"
test -s "$apk"
fdroid scanner --verbose --exit-code "$apk" 2>&1 | tee /output/apk-scan.txt

androguard axml "$apk" -o "$work_root/AndroidManifest.xml"
if grep -Eq 'android:debuggable="true"|android:testOnly="true"|android:usesCleartextTraffic="true"' "$work_root/AndroidManifest.xml"; then
    echo "F-Droid APK manifest contains a forbidden release/debug attribute" >&2
    exit 1
fi
"$repo_root/fdroid/verify-apk.sh" "$apk" 2>&1 | tee /output/apk-identity.txt

cd "$data"
tools/audit-gradle.py "$appid" 2>&1 | tee /output/gradle-audit.txt

if apksigner verify "$apk" > /output/signing-policy.txt 2>&1; then
    echo "F-Droid recipe unexpectedly produced a signed APK" >&2
    exit 1
else
    printf 'unsigned build input for F-Droid signing\n' >> /output/signing-policy.txt
fi

cp "$apk" "/output/${appid}_${WEGERT_VERSION_CODE}.apk"
sha256sum "/output/${appid}_${WEGERT_VERSION_CODE}.apk"     > "/output/${appid}_${WEGERT_VERSION_CODE}.apk.sha256"
