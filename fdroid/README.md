# F-Droid release path

Wegert stays the internal project and package name. F-Droid publishes it as **zero & infinity**. The F-Droid build must come from a tagged source commit and must not depend on GitHub Actions run numbers or private signing material.

## Upstream release contract

1. Keep `versionCode` and `versionName` in `app/build.gradle.kts` source-controlled.
2. Run the `F-Droid release build` workflow. It builds `assembleRelease` with `-PfdroidBuild=true`, checks the package/version, checks the public label `zero & infinity`, and checks all three native ABIs.
3. Keep the repository license and F-Droid metadata aligned on `GPL-3.0-or-later`. `THIRD_PARTY.md` records material that is not relicensed by that grant.
4. Merging a release-version change to `main` creates the matching `v<versionName>` tag automatically. The normal build remains named Wegert; only the F-Droid build uses the public label.
5. Copy `org.isomorphisms.wegert.yml.template` to `fdroiddata/metadata/org.isomorphisms.wegert.yml`, run `fdroid lint org.isomorphisms.wegert`, then submit the fdroiddata merge request.

F-Droid performs its own source build and signs the resulting APK. The upstream unsigned artifact is only a build gate and inspection artifact.

After first inclusion, `UpdateCheckMode: Tags` plus `AutoUpdateMode: Version` lets F-Droid detect later release tags automatically, provided each tag contains the matching source-controlled Android version.
