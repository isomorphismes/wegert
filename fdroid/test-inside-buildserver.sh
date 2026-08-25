#!/usr/bin/env bash
set -euo pipefail

# This script runs inside registry.gitlab.com/fdroid/fdroidserver:buildserver-trixie.
# Its build command and environment setup track fdroiddata's `fdroid build` job.

readonly appid=org.isomorphisms.wegert
readonly repo_root=/workspace/wegert
readonly fdroiddata_revision=4498e27635a1c3b737510342c1f2355c25ce0211
readonly fdroidserver_revision=6af4c4216e43d0fcb29e33919cd0fe8fef7e7400
readonly source_repo="${SOURCE_REPO:-https://github.com/isomorphisms/wegert.git}"
readonly source_revision="${SOURCE_REVISION:?SOURCE_REVISION is required}"

source "$repo_root/fdroid/release-values.sh"
readonly build_id="$appid:$WEGERT_VERSION_CODE"
readonly work_root="$(mktemp -d /tmp/wegert-fdroiddata.XXXXXX)"
readonly data="$work_root/fdroiddata"
readonly server="$work_root/fdroidserver"
readonly original_metadata="$work_root/original-metadata.yml"

cleanup() {
    if [[ -L /home/vagrant/fdroiddata ]]; then
        rm /home/vagrant/fdroiddata
    fi
    rm -f "/home/vagrant/metadata/$appid.yml"
    rm -rf "/home/vagrant/build/$appid"
    rm -rf "$work_root"
}
trap cleanup EXIT

git clone --filter=blob:none https://gitlab.com/fdroid/fdroiddata.git "$data"
git -C "$data" checkout --detach "$fdroiddata_revision"
git clone --filter=blob:none https://gitlab.com/fdroid/fdroidserver.git "$server"
git -C "$server" checkout --detach "$fdroidserver_revision"

cp "$repo_root/fdroid/$appid.yml.template" "$data/metadata/$appid.yml"
sed -i "s|^Repo: .*|Repo: $source_repo|" "$data/metadata/$appid.yml"
release_tag_revision="$(git ls-remote "$source_repo" "refs/tags/v$WEGERT_VERSION_NAME" | awk 'NR == 1 { print $1 }')"
if [[ "$release_tag_revision" != "$source_revision" ]]; then
    sed -i "s|^    commit: .*|    commit: $source_revision|" "$data/metadata/$appid.yml"
fi
cp "$data/metadata/$appid.yml" "$original_metadata"

export PATH="$server:$PATH"
export PYTHONPATH="$server:$server/examples"
export PYTHONUNBUFFERED=true
export serverwebroot=/tmp
export ANDROID_HOME=/opt/android-sdk

cd "$data"

# Metadata checks copied from the fdroiddata merge-request pipeline.
fdroid lint "$appid"
fdroid rewritemeta "$appid"
cmp "$original_metadata" "metadata/$appid.yml"

if [[ "$release_tag_revision" = "$source_revision" ]]; then
    cp "metadata/$appid.yml" "$work_root/before-checkupdates.yml"
    fdroid checkupdates --auto -v "$appid"
    cmp "$work_root/before-checkupdates.yml" "metadata/$appid.yml"
else
    echo "checkupdates deferred until public tag v$WEGERT_VERSION_NAME exists"
fi

python3 -m pip install --quiet --break-system-packages check-jsonschema markdown-it-py
check-jsonschema --schemafile schemas/metadata.json "metadata/$appid.yml"

cp "metadata/$appid.yml" "$work_root/before-redirect.yml"
tools/rewrite-git-redirects.py "$appid"
cmp "$work_root/before-redirect.yml" "metadata/$appid.yml"

python3 tools/check-fastlane.py "$appid" > "$work_root/fastlane.json"
python3 - "$work_root/fastlane.json" <<'PY'
import json
import sys

reports = json.load(open(sys.argv[1], encoding="utf-8"))
bad = [report for report in reports if report.get("severity") in {"critical", "major"}]
for report in reports:
    print(f"{report.get('severity')}: {report.get('description')}")
if bad:
    raise SystemExit("F-Droid Fastlane checks reported critical or major problems")
PY

# The following setup and command mirror fdroiddata's production-like build job.
apt-get update
apt-get install -y --no-install-recommends sudo openjdk-21-jdk-headless jq
update-alternatives --set java /usr/lib/jvm/java-21-openjdk-amd64/bin/java
sdkmanager "platform-tools" "build-tools;31.0.0"

if [[ -f /etc/profile.d/bsenv.sh ]]; then
    # shellcheck disable=SC1091
    source /etc/profile.d/bsenv.sh
fi
home_vagrant="${home_vagrant:-/home/vagrant}"
test -d "$home_vagrant"

git -C "$home_vagrant/gradlew-fdroid" pull --ff-only

mkdir -p "$data/build" "$data/logs" "$data/tmp" "$data/unsigned"
mkdir -p "$home_vagrant/.android" "$home_vagrant/.gradle" "$home_vagrant/metadata"
rm -rf "$home_vagrant/build"
cp -R "$data/build" "$home_vagrant/build"
ln -sfn "$data/tmp" "$home_vagrant/tmp"
ln -sfn "$data/srclibs" "$home_vagrant/srclibs"
cp "metadata/$appid.yml" "$home_vagrant/metadata/"
chown -R vagrant "$home_vagrant" "$data"

run_fdroid() {
    sudo --preserve-env --user vagrant env \
        PATH="$server:$PATH" \
        PYTHONPATH="$server:$server/examples" \
        PYTHONUNBUFFERED=true \
        TERM="${TERM:-dumb}" \
        HOME="$home_vagrant" \
        fdroid "$@"
}

cd "$home_vagrant"
ln -s "$data" "$home_vagrant/fdroiddata"
run_fdroid fetchsrclibs "$build_id" --verbose
rm "$home_vagrant/fdroiddata"

# fdroid build --on-server removes sudo, just as it does in fdroiddata CI.
(unset CI; run_fdroid build --verbose --test --refresh-scanner --on-server --no-tarball "$build_id")

readonly apk="$data/tmp/${appid}_${WEGERT_VERSION_CODE}.apk"
test -s "$apk"
fdroid scanner --verbose --exit-code "$apk"

androguard axml "$apk" -o "$work_root/AndroidManifest.xml"
if grep -Eq 'android:debuggable="true"|android:testOnly="true"|android:usesCleartextTraffic="true"' "$work_root/AndroidManifest.xml"; then
    echo "F-Droid APK manifest contains a forbidden release/debug attribute" >&2
    exit 1
fi
cd "$data"
tools/audit-gradle.py "$appid"

mkdir -p /output
cp "$apk" "/output/${appid}_${WEGERT_VERSION_CODE}.apk"
sha256sum "/output/${appid}_${WEGERT_VERSION_CODE}.apk" \
    > "/output/${appid}_${WEGERT_VERSION_CODE}.apk.sha256"
