# F-Droid release path

Wegert stays the internal project and package name. F-Droid publishes it as **zero & infinity**. The F-Droid build must come from a tagged source commit and must not depend on GitHub Actions run numbers or private signing material.

## Upstream release contract

1. Keep `versionCode` and `versionName` in `app/build.gradle.kts` source-controlled and identical to `org.isomorphisms.wegert.yml.template`.
2. Merge the release candidate to `main`. Record the exact commit SHA and the successful main-branch runs of `Android native build and device emulation` and `F-Droid release build`. The F-Droid workflow makes two clean `assembleRelease` builds with `-PfdroidBuild=true`, requires byte-identical APKs, and checks the package/version, public label, and all three native ABIs.
3. Download `wegert-debug-apk` from that exact Android run. Install that APK on the release phone and tablet, then complete the interaction checklist: add and drag factors, pan, pinch, continuation success and rejection, clear in both views, and Android Back.
4. Only after accepting that exact APK on real hardware, manually run `Tag tested F-Droid release` with the merged SHA and both run IDs. The workflow rejects unsuccessful runs, mismatched commits, non-`main` runs, and a pre-existing tag that points elsewhere.
5. Manually run `Publish tested APK` with the same SHA and Android run ID. It inspects the downloaded artifact and requires the matching immutable tag before publishing the prerelease APK and checksum.
6. Keep the repository license and F-Droid metadata aligned on `GPL-3.0-or-later`. `THIRD_PARTY.md` records material that is not relicensed by that grant.
7. Copy `org.isomorphisms.wegert.yml.template` to `fdroiddata/metadata/org.isomorphisms.wegert.yml`, run `fdroid lint org.isomorphisms.wegert`, then submit the fdroiddata merge request.

The normal build remains named Wegert; only the F-Droid build uses the public label **zero & infinity**. Neither tagging nor APK publication is automatic: real-hardware acceptance is the release gate.

The same workflow also runs the submitted recipe in F-Droid's production-like `registry.gitlab.com/fdroid/fdroidserver:buildserver-trixie` image. `fdroid/run-fdroiddata-tests.sh` copies the relevant current fdroiddata checks: metadata lint and canonical rewriting, schema validation, redirected-Git checks, upstream Fastlane extraction, `fdroid build --on-server`, the binary scanner, and the Gradle audit. The script replaces the recipe's release tag with the exact public CI commit while testing a branch; the release recipe itself remains pinned to the immutable `v<versionName>` tag.

`gradle/wrapper/gradle-wrapper.properties` pins Gradle 9.5.1 and its SHA-256 checksum. F-Droid's `gradlew-fdroid` reads that file because it cannot infer the Gradle version from this project's modern plugins DSL.

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
