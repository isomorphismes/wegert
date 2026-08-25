# F-Droid release path

Wegert stays the internal project and package name. F-Droid publishes it as **zero & infinity**. The F-Droid build must come from a tagged source commit and must not depend on GitHub Actions run numbers or private signing material.

## Upstream release contract

1. Keep `versionCode` and `versionName` in `app/build.gradle.kts` source-controlled and identical to `org.isomorphisms.wegert.yml.template`.
2. Merge the release candidate to `main`. Record the exact commit SHA and the successful main-branch runs of `Android native build and device emulation` and `F-Droid release build`. The F-Droid workflow builds `assembleRelease` with `-PfdroidBuild=true`, checks the package/version and public label, and checks all three native ABIs.
3. Download `wegert-debug-apk` from that exact Android run. Install that APK on the release phone and tablet, then complete the interaction checklist: add and drag factors, pan, pinch, continuation success and rejection, clear in both views, and Android Back.
4. Only after accepting that exact APK on real hardware, manually run `Tag tested F-Droid release` with the merged SHA and both run IDs. The workflow rejects unsuccessful runs, mismatched commits, non-`main` runs, and a pre-existing tag that points elsewhere.
5. Manually run `Publish tested APK` with the same SHA and Android run ID. It inspects the downloaded artifact and requires the matching immutable tag before publishing the prerelease APK and checksum.
6. Keep the repository license and F-Droid metadata aligned on `GPL-3.0-or-later`. `THIRD_PARTY.md` records material that is not relicensed by that grant.
7. Copy `org.isomorphisms.wegert.yml.template` to `fdroiddata/metadata/org.isomorphisms.wegert.yml`, run `fdroid lint org.isomorphisms.wegert`, then submit the fdroiddata merge request.

The normal build remains named Wegert; only the F-Droid build uses the public label **zero & infinity**. Neither tagging nor APK publication is automatic: real-hardware acceptance is the release gate.

F-Droid performs its own source build and signs the resulting APK. The upstream unsigned artifact is only a build gate and inspection artifact.

After first inclusion, `UpdateCheckMode: Tags` plus `AutoUpdateMode: Version` lets F-Droid detect later release tags automatically, provided each tag contains the matching source-controlled Android version.

## Publication status

The `F-Droid publication status` workflow runs the read-only Grease checks after the release workflow, once a day, on relevant pull requests, and on demand.

```sh
ysh fdroid/gitlab-status.grease
ysh fdroid/store-status.grease
```

`gitlab-status.grease` searches the public `fdroid/fdroiddata` merge requests for **zero & infinity** from the submission branch, including closed and merged MRs. It reports whether the MR was accepted, merge/pipeline state, labels, dates, and submitted non-system comments from GitLab's public GraphQL notes feed. It checks source visibility against the `SourceCode` URL in the F-Droid metadata, not the submitter's fdroiddata fork, so neither check requires access to that fork or a GitLab token. Unpublished draft notes are intentionally outside this public-status check. The source branch defaults to `master`; pass another branch as the first argument or set `GITLAB_BRANCH`. Override the search text with `GITLAB_MR_SEARCH`.

`store-status.grease` checks F-Droid's public package API and store page for `org.isomorphisms.wegert`. It reads the expected `versionCode` and `versionName` directly from `app/build.gradle.kts`, then reports whether the app is visible, what F-Droid currently suggests, whether the exact expected version is published, and whether that version is current. A package 404 is reported as `pending` and exits successfully; transport/API errors still fail CI.
