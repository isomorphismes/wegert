#!/usr/bin/env bash
# Exercise actual collection/wiring with local Git and synthetic CI artifacts.
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
: "${AICI_POLICY_CHECKOUT:?AICI policy checkout required}"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/app/_/build/fdroid" "$work/policy/release-promotion" "$work/bin" "$work/artifacts"
cp "$here/collect-release-context.sh" "$work/app/_/build/fdroid/"
cp "$AICI_POLICY_CHECKOUT/release-promotion/gate.sh" "$work/policy/release-promotion/"
printf 'versionName=0.2.0\nversionCode=101\nndk=29.0.14206865\n' > "$work/app/_/build/fdroid/release.properties"
for repo in app policy; do
    git -C "$work/$repo" init -q -b main
    git -C "$work/$repo" add .
    git -C "$work/$repo" -c user.name=Fixture -c user.email=fixture@example.invalid commit -qm 'Synthetic fixture only'
    git -C "$work/$repo" remote add origin "$work/$repo"
done
export AICI_POLICY_CHECKOUT="$work/policy"
export SOURCE_SHA=$(git -C "$work/app" rev-parse HEAD)
export GITHUB_WORKFLOW_SHA="$SOURCE_SHA" GITHUB_REPOSITORY=isomorphismes/wegert GITHUB_REF=refs/heads/main GITHUB_EVENT_NAME=workflow_dispatch
export ANDROID_RUN_ID=101 FDROID_RUN_ID=102
export FIXTURE="$work/artifacts" CALLS="$work/calls"
printf 'synthetic signed bytes\n' > "$FIXTURE/candidate.apk"
printf 'synthetic unsigned bytes\n' > "$FIXTURE/unsigned.apk"
{
    printf 'source_sha=%s\n' "$SOURCE_SHA"
    printf 'candidate_apk_sha256=%s\n' "$(sha256sum "$FIXTURE/candidate.apk"|awk '{print $1}')"
    printf 'unsigned_apk_sha256=%s\n' "$(sha256sum "$FIXTURE/unsigned.apk"|awk '{print $1}')"
    printf 'version_name=0.2.0\nversion_code=101\nndk_revision=29.0.14206865\nsigning=test-only\npayload_comparison=PASS\nphysical_device=NOT_RUN\n'
} > "$FIXTURE/candidate.properties"
cat > "$work/bin/gh" <<'FAKE'
#!/usr/bin/env bash
set -euo pipefail
if [[ $1 == api && $2 == */actions/runs/* && $# == 2 ]]; then
    id=${2##*/}; path=.github/workflows/android.yml; attempt=1
    if [[ $id == 102 ]]; then
        path=.github/workflows/fdroid-build.yml
        echo fdroid >> "$CALLS"
        if [[ $SCENARIO == rerun && $(wc -l < "$CALLS") -gt 1 ]]; then attempt=2; fi
    fi
    jq -n --arg source "$SOURCE_SHA" --argjson id "$id" --arg path "$path" --arg s "$SCENARIO" --argjson attempt "$attempt" \
      '{id:$id,head_sha:$source,repository:{full_name:"isomorphismes/wegert"},head_repository:{full_name:(if $s=="fork-run" then "fork/wegert" else "isomorphismes/wegert" end)},event:"push",head_branch:"main",path:$path,status:"completed",conclusion:(if $s=="failed-run" then "failure" else "success" end),run_attempt:$attempt,created_at:((now-120)|floor|todateiso8601)}'
elif [[ $1 == run && $2 == download ]]; then
    name=''; out=''
    while [[ $# -gt 0 ]]; do
        case $1 in --name) name=$2; shift 2 ;; --dir) out=$2; shift 2 ;; *) shift ;; esac
    done
    mkdir -p "$out"
    case $name in
      wegert-fdroid-device-candidate)
        cp "$FIXTURE/candidate.apk" "$out/zero-infinity-0.2.0-device-candidate.apk"
        cp "$FIXTURE/candidate.properties" "$out/candidate.properties"
        case $SCENARIO in
          wrong-source) sed -i 's/^source_sha=.*/source_sha=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb/' "$out/candidate.properties" ;;
          wrong-version) sed -i 's/^version_code=.*/version_code=102/' "$out/candidate.properties" ;;
          wrong-ndk) sed -i 's/^ndk_revision=.*/ndk_revision=27.3.13750724/' "$out/candidate.properties" ;;
          changed-bytes) echo altered >> "$out/zero-infinity-0.2.0-device-candidate.apk" ;;
          duplicate-hash) grep '^candidate_apk_sha256=' "$FIXTURE/candidate.properties" >> "$out/candidate.properties" ;;
        esac ;;
      wegert-fdroid-unsigned) cp "$FIXTURE/unsigned.apk" "$out/wegert-0.2.0-fdroid-unsigned.apk" ;;
      *) echo 'Unexpected download' >&2; exit 99 ;;
    esac
else echo 'Unexpected non-read GitHub command' >&2; exit 99
fi
FAKE
chmod +x "$work/bin/gh"
export PATH="$work/bin:$PATH"
count=0
for scenario in good wrong-source wrong-version wrong-ndk changed-bytes duplicate-hash failed-run fork-run rerun; do
    export SCENARIO=$scenario
    : > "$CALLS"
    if bash "$work/app/_/build/fdroid/collect-release-context.sh" "$work/$scenario" > "$work/log" 2>&1; then
        [[ $scenario == good ]] || { echo "Wrongly accepted $scenario"; exit 1; }
    else
        [[ $scenario != good ]] || { cat "$work/log"; exit 1; }
        [[ ! -f $work/$scenario/acceptance-draft.json ]]
    fi
    count=$((count+1))
done
jq -e --arg source "$SOURCE_SHA" '.binding.source_sha == $source and .decision == "NOT_RUN" and all(.checks[]; .result == "NOT_RUN")' "$work/good/acceptance-draft.json" >/dev/null
if bash "$AICI_POLICY_CHECKOUT/release-promotion/gate.sh" physical "$work/good/context.json" "$work/good/acceptance-draft.json" "$work/good/candidate/zero-infinity-0.2.0-device-candidate.apk" "$work/good/unsigned/wegert-0.2.0-fdroid-unsigned.apk" > "$work/log" 2>&1; then exit 1; fi
grep -Fq 'AICI-RELEASE-ACCEPTANCE:' "$work/log"
count=$((count+1))
export SCENARIO=good
if bash "$work/app/_/build/fdroid/collect-release-context.sh" "$work/good" > "$work/log" 2>&1; then exit 1; fi
grep -Fq 'Evidence output must be new' "$work/log"
count=$((count+1))
printf 'PASS: %s consumer collection cases; synthetic Git/run/artifact fixtures only\n' "$count"
