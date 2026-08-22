# F-Droid release path

Wegert's F-Droid build must come from a tagged source commit and must not depend on GitHub Actions run numbers or private signing material.

## Upstream release contract

1. Keep `versionCode` and `versionName` in `app/build.gradle.kts` source-controlled.
2. Run the `F-Droid release build` workflow. It builds `assembleRelease`, checks the package/version and all three native ABIs, and retains the unsigned APK as evidence.
3. Choose and add a FLOSS `LICENSE` file before submission. Replace `CHOOSE-A-FLOSS-SPDX-ID` in the metadata template with its SPDX identifier.
4. Tag the exact release commit `v<versionName>`.
5. Replace `FULL_COMMIT_HASH` in `org.isomorphisms.wegert.yml.template` with the full hash of that tagged commit.
6. Copy the template to `fdroiddata/metadata/org.isomorphisms.wegert.yml`, run `fdroid lint org.isomorphisms.wegert`, then submit the fdroiddata merge request.

F-Droid performs its own source build and signs the resulting APK. The upstream unsigned artifact is only a build gate and inspection artifact.

After first inclusion, `UpdateCheckMode: Tags` plus `AutoUpdateMode: Version` lets F-Droid detect later release tags automatically, provided each tag contains the matching source-controlled Android version.
