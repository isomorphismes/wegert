# F-Droid release path

Wegert stays the internal project and package name. F-Droid publishes it as **zero & infinity**. The F-Droid build must come from a tagged source commit and must not depend on GitHub Actions run numbers or private signing material.

## Upstream release contract

1. Keep `versionCode` and `versionName` in `app/build.gradle.kts` source-controlled.
2. Run the `F-Droid release and fdroiddata build` workflow. It makes two clean `assembleRelease` builds with `-PfdroidBuild=true` and requires byte-identical unsigned APKs. It also checks the package/version, public label `zero & infinity`, and all three native ABIs.
3. Keep the repository license and F-Droid metadata aligned on `GPL-3.0-or-later`. `THIRD_PARTY.md` records material that is not relicensed by that grant.
4. Merging a release-version change to `main` creates the matching `v<versionName>` tag automatically. The normal build remains named Wegert; only the F-Droid build uses the public label.
5. Copy `org.isomorphisms.wegert.yml.template` to `fdroiddata/metadata/org.isomorphisms.wegert.yml`, run `fdroid lint org.isomorphisms.wegert`, then submit the fdroiddata merge request.

The same workflow also runs the submitted recipe in F-Droid's production-like `registry.gitlab.com/fdroid/fdroidserver:buildserver-trixie` image. `fdroid/run-fdroiddata-tests.sh` copies the relevant current fdroiddata checks: metadata lint and canonical rewriting, schema validation, redirected-Git checks, upstream Fastlane extraction, `fdroid build --on-server`, the binary scanner, and the Gradle audit. The script replaces the recipe's release tag with the exact public CI commit while testing a branch; the release recipe itself remains pinned to the immutable `v<versionName>` tag.

No fdroidserver submodule is used. The production buildserver image is the meaningful dependency because it contains the surrounding Debian, Android SDK, NDK, Gradle, and scanner environment; a source-only submodule would not reproduce that environment.

## Native APK packaging

Wegert publishes one universal APK. That APK contains `arm64-v8a`, `armeabi-v7a`, and `x86_64` variants of `libwegert.so`. There is no Gradle `splits` configuration and therefore no set of separate APKs for native ABIs. The fdroiddata checklist item “Multiple apks for native code” does not apply and should remain unchecked.

## Local F-Droid checks

The cheap metadata and packaging-contract check does not require the Android SDK:

```sh
fdroid/verify-metadata.sh
```

With Gradle and the pinned Android toolchain installed, build twice and compare the APK bytes:

```sh
fdroid/reproducible-build.sh
```

With Docker installed and the current commit pushed to a public branch, run the fdroiddata production-build test:

```sh
fdroid/run-fdroiddata-tests.sh
```

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
