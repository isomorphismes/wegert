# F-Droid release path

This directory is build/release machinery. From the repository root, either run commands with the full `_/build/...` path or `cd _/build` first.

Wegert remains the internal project/package name. The intended F-Droid listing is **zero & infinity**. Release identity comes from `fdroid/release.properties`; the F-Droid recipe builds directly with NDK/CMake, `aapt2`, and `zipalign`, without Gradle.

## Release contract

1. Keep `fdroid/release.properties`, `app/build.gradle.kts`, and `fdroid/org.isomorphisms.wegert.yml.template` on the same version name/code.
2. Merge the release candidate to `main` and record the exact successful main-branch push Android and F-Droid workflow runs. PR runs are not main-branch release receipts.
3. From that F-Droid run, install the APK in **`wegert-fdroid-device-candidate`**, not the unrelated Gradle/ICK or direct-DEX APK. Verify its `candidate.properties` source and APK hash. Replace the existing test installation without uninstalling first; do not hide a signer or version downgrade error by deleting the app.
4. On physical hardware, check the launcher name, zero/pole placement and dragging, pan, pinch, clear, Android Back, background/resume, and relaunch. Record the device, exact candidate hash, source SHA and observed pass/fail results in release issue #68. An emulator or earlier APK test does not supply these observations.
5. Only then run `Tag tested F-Droid release` for that exact source SHA and the two successful run IDs. Never create the tag merely to unblock a build.
6. Copy `fdroid/org.isomorphisms.wegert.yml.template` into fdroiddata, submit the upstream merge request, address review, and confirm actual official package publication. The existing `Publish tested APK` workflow is a separate GitHub direct-DEX prerelease path, not a prerequisite for F-Droid submission.

The canonical upstream store metadata is Triple-T under `app/src/main/play`. `fdroid/verify-metadata.sh` rejects the old `fastlane/metadata/android` tree if it reappears, so CI cannot silently fall back to a second metadata source.

## Build and artifact checks

The F-Droid workflow checks out the explicit source revision in both build jobs. It requires two clean direct builds from different source directories, then compares their unsigned APK with the independently source-built fdroiddata buildserver APK. All three must have identical bytes. This is reproducibility with the declared compiler, not diverse double compilation or physical-device acceptance.

`select-ndk.sh` selects the declared NDK version rather than inheriting a runner's `ANDROID_NDK_HOME` or `ANDROID_NDK`. An explicit F-Droid `NDK_ROOT` is accepted only when its actual `source.properties` identifies the required revision. Both direct builds retain that version evidence alongside their APKs.

`verify-apk.sh` inspects the actual archive, ABI inventory, package/version, launcher, SDK levels, absence of DEX/network permissions/debug flags, and ZIP alignment. `verify-unsigned.sh` rejects APK signing blocks and JAR signature records structurally. A failed `apksigner verify` is not proof that an APK is unsigned. The supported ZIP envelope is deliberately restricted to this lane's ordinary single-disk, non-ZIP64, comment-free output; unexpected envelopes fail closed.

The production-like fdroiddata job independently checks metadata, scanner output, manifest flags and Triple-T extraction through F-Droid's legacy-named `tools/check-fastlane.py`. Its update check is explicitly deferred until the matching public release tag exists. It does not represent F-Droid's official pipeline or reviewer acceptance.

`prepare-device-candidate.sh` signs a copy of the verified unsigned APK with the existing stable public **test-only** key. It compares the complete application-entry inventory and every entry's bytes before and after signing. It leaves the source-built unsigned APK unchanged, rechecks the signer, and emits source/unsigned/candidate hashes. Its receipt deliberately says `physical_device=NOT_RUN`. The central AICI signer registry independently verifies the finished candidate before artifact upload.

## Store screenshot provenance

The checked phone screenshot is generated from Wegert runtime evidence rather than copied from an undocumented image. Run `Refresh F-Droid store screenshot` on a non-main branch and give it a successful main-branch Android workflow run ID. The workflow downloads that run's `wegert-miro-a1-emulator-evidence`, crops only the Android navigation-button rail, and commits the resulting Triple-T screenshot plus `fdroid/store-screenshot.provenance` and `fdroid/store-screenshot.sha256`.

`fdroid/verify-metadata.sh` verifies the screenshot hash and the provenance fields. The refresh workflow refuses to commit directly to `main`; the generated asset goes through ordinary review with the rest of the release source. Provenance checks do not resolve unverified copyright-holder facts for other assets.

## Local checks

From the repository root:

```sh
cd _/build
bash fdroid/test-release-guards.sh
fdroid/verify-metadata.sh
fdroid/reproducible-build.sh
fdroid/run-fdroiddata-tests.sh
```

The guard test uses synthetic ZIP/SDK fixtures to prove rejection behavior, not Android execution. The reproducible build needs the pinned Android SDK/NDK/CMake inputs. The fdroiddata test additionally needs Docker and a public source ref. Existing inline Python in the inherited packaging scripts remains migration debt; this change adds no new Python program.

F-Droid performs its own source build and signs the resulting APK. Neither the unsigned build artifact nor its test-signed device candidate is a published F-Droid release.

## Release versus prerelease

- **Candidate / prerelease:** an artifact for inspection and testing. The repository's public debug signer is **not** the F-Droid release signer. ICK and direct-DEX experiments remain separate candidates.
- **Tagged upstream release:** `v0.2.0` (versionCode 101) is created only after exact main-branch checks and physical acceptance of the corresponding F-Droid device candidate. A tag alone is not publication.
- **Submission:** opening a fdroiddata MR starts review. Passing F-Droid's official pipeline and review establishes submission acceptance; a closed, draft or merely open request does not.
- **Published release:** F-Droid has built and signed the APK and the public package index confirms that version. No Google Play account or Google Play paperwork is required. F-Droid's own store metadata and screenshot requirements still apply. No weekly submission cadence is imposed by this project.

The `F-Droid publication status` workflow reports the GitLab review and official package API state. A successful status query can report **not published**; its green workflow badge is not publication evidence.

The current GitHub debug APK and an eventual F-Droid-signed APK normally have different certificates. Android cannot update between those signers in place. Preserve any user data before an explicitly planned channel migration. Do not call GitHub test APKs interchangeable F-Droid releases.
