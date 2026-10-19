#!/usr/bin/env bash
# Read-only collection for release promotion; no tag or release writes here.
set -euo pipefail
[[ $# == 1 ]] || { echo 'usage: collect-release-context.sh OUTPUT_DIRECTORY' >&2; exit 2; }
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
: "${AICI_POLICY_CHECKOUT:?pinned AICI checkout required}"
: "${SOURCE_SHA:?exact source required}"
: "${ANDROID_RUN_ID:?Android run required}"
: "${FDROID_RUN_ID:?F-Droid run required}"
[[ $SOURCE_SHA =~ ^[0-9a-f]{40}$ && $ANDROID_RUN_ID =~ ^[1-9][0-9]*$ && $FDROID_RUN_ID =~ ^[1-9][0-9]*$ ]]
[[ ${GITHUB_REPOSITORY:-} == isomorphismes/wegert && ${GITHUB_REF:-} == refs/heads/main && ${GITHUB_EVENT_NAME:-} == workflow_dispatch ]]
[[ $(git -C "$root" rev-parse HEAD) == "${GITHUB_WORKFLOW_SHA:?workflow policy revision required}" ]]
git -C "$root" merge-base --is-ancestor "$SOURCE_SHA" HEAD
git -C "$AICI_POLICY_CHECKOUT" fetch origin main
git -C "$AICI_POLICY_CHECKOUT" merge-base --is-ancestor HEAD origin/main
gate="$AICI_POLICY_CHECKOUT/release-promotion/gate.sh"
[[ ! -e $1 ]] || { echo 'Evidence output must be new; stale files are not reusable' >&2; exit 1; }
mkdir -p "$1"
out=$(cd "$1" && pwd)
for lane in android fdroid; do
    if [[ $lane == android ]]; then id=$ANDROID_RUN_ID; path=.github/workflows/android.yml
    else id=$FDROID_RUN_ID; path=.github/workflows/fdroid-build.yml; fi
    gh api "repos/$GITHUB_REPOSITORY/actions/runs/$id" > "$out/$lane-run.json"
    bash "$gate" verify-run "$out/$lane-run.json" "$GITHUB_REPOSITORY" "$SOURCE_SHA" "$id" "$path"
done
git -C "$root" show "$SOURCE_SHA:_/build/fdroid/release.properties" > "$out/release.properties"
value() { bash "$gate" property "$1" "$2"; }
version=$(value "$out/release.properties" versionName)
code=$(value "$out/release.properties" versionCode)
[[ $version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ && $code =~ ^[1-9][0-9]*$ ]]
gh run download "$FDROID_RUN_ID" --repo "$GITHUB_REPOSITORY" --name wegert-fdroid-device-candidate --dir "$out/candidate"
gh run download "$FDROID_RUN_ID" --repo "$GITHUB_REPOSITORY" --name wegert-fdroid-unsigned --dir "$out/unsigned"
properties="$out/candidate/candidate.properties"
candidate="$out/candidate/zero-infinity-$version-device-candidate.apk"
unsigned="$out/unsigned/wegert-$version-fdroid-unsigned.apk"
[[ -s $candidate && -f $candidate && ! -L $candidate && -s $unsigned && -f $unsigned && ! -L $unsigned ]]
[[ $(value "$properties" source_sha) == "$SOURCE_SHA" ]]
[[ $(value "$properties" version_name) == "$version" && $(value "$properties" version_code) == "$code" ]]
[[ $(value "$properties" ndk_revision) == "$(value "$out/release.properties" ndk)" ]]
[[ $(value "$properties" signing) == test-only && $(value "$properties" payload_comparison) == PASS ]]
signed_hash=$(sha256sum "$candidate" | awk '{print $1}')
unsigned_hash=$(sha256sum "$unsigned" | awk '{print $1}')
[[ $(value "$properties" candidate_apk_sha256) == "$signed_hash" && $(value "$properties" unsigned_apk_sha256) == "$unsigned_hash" ]]
attempt=$(jq -r '.run_attempt|tostring' "$out/fdroid-run.json")
# Detect a rerun started while artifacts were being collected.
gh api "repos/$GITHUB_REPOSITORY/actions/runs/$FDROID_RUN_ID" > "$out/fdroid-run-after.json"
bash "$gate" verify-run "$out/fdroid-run-after.json" "$GITHUB_REPOSITORY" "$SOURCE_SHA" "$FDROID_RUN_ID" .github/workflows/fdroid-build.yml
[[ $(jq -r '.run_attempt|tostring' "$out/fdroid-run-after.json") == "$attempt" ]]
jq -n --arg repo "$GITHUB_REPOSITORY" --arg source "$SOURCE_SHA" \
    --arg run "$FDROID_RUN_ID" --arg attempt "$attempt" --arg version "$version" --arg code "$code" \
    --arg signed "$signed_hash" --arg unsigned "$unsigned_hash" \
    --arg built "$(jq -r '.created_at' "$out/fdroid-run.json")" \
    '{schema:"release-context-v1",built_at:$built,
      binding:{repository:$repo,source_sha:$source,fdroid_run_id:$run,fdroid_run_attempt:$attempt,
        package_id:"org.isomorphisms.wegert",version_name:$version,version_code:$code,
        candidate_apk_sha256:$signed,unsigned_apk_sha256:$unsigned},
      required_checks:["replacement_install","launcher_identity","zero_placement","pole_placement",
        "factor_drag","pan","pinch","clear","android_back","render","background_resume","relaunch"]}' > "$out/context.json"
jq '{schema:"physical-acceptance-v1",binding,decision:"NOT_RUN",observed_by:"",report_reference:"",
    observed_at:null,device:{kind:"physical",model:"",android_version:""},
    checks:(.required_checks | map({key:.,value:{result:"NOT_RUN",detail:""}}) | from_entries)}' \
    "$out/context.json" > "$out/acceptance-draft.json"
printf 'Collected exact candidate %s; physical acceptance remains NOT_RUN\n' "$signed_hash"
