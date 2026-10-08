# F-Droid release path

This directory is build/release machinery. From the repository root, either run commands with the full `_/build/...` path or `cd _/build` first.

Wegert remains the internal project/package name. F-Droid publishes it as **zero & infinity**. Release identity comes from `fdroid/release.properties`; the F-Droid recipe builds directly with NDK/CMake, `aapt2`, and `zipalign`, without Gradle.

## Release contract

1. Keep `fdroid/release.properties`, `app/build.gradle.kts`, and `fdroid/org.isomorphisms.wegert.yml.template` on the same version name/code.
2. Merge the release candidate to `main` and record the exact successful Android and F-Droid workflow runs.
3. Install the tested Android artifact on the release phone/tablet and exercise factor placement/dragging, pan, pinch, clear, and Android Back.
4. Only then run `Tag tested F-Droid release` for that exact source SHA and the two successful run IDs.
5. `Publish tested APK` requires the same tested source and immutable tag.
6. Copy `fdroid/org.isomorphisms.wegert.yml.template` into fdroiddata and submit the upstream merge request.

The canonical upstream store metadata is Triple-T under `app/src/main/play`. `fdroid/verify-metadata.sh` rejects the old `fastlane/metadata/android` tree if it reappears, so CI cannot silently fall back to a second metadata source.

The F-Droid workflow makes two clean direct builds and requires byte-identical unsigned APKs. It also runs the recipe inside F-Droid's production-like buildserver image and checks metadata, scanner output, ABI packaging, and upstream Triple-T extraction through F-Droid's legacy-named `tools/check-fastlane.py`.

## Store screenshot provenance

The checked phone screenshot is generated from Wegert runtime evidence rather than copied from an undocumented image. Run `Refresh F-Droid store screenshot` on a non-main branch and give it a successful main-branch Android workflow run ID. The workflow downloads that run's `wegert-miro-a1-emulator-evidence`, crops only the Android navigation-button rail, and commits the resulting Triple-T screenshot plus `fdroid/store-screenshot.provenance` and `fdroid/store-screenshot.sha256`.

`fdroid/verify-metadata.sh` verifies the screenshot hash and the provenance fields. The refresh workflow refuses to commit directly to `main`; the generated asset goes through ordinary review with the rest of the release source.

## Local checks

From the repository root:

```sh
cd _/build
fdroid/verify-metadata.sh
fdroid/reproducible-build.sh
fdroid/run-fdroiddata-tests.sh
```

The first check is cheap. The reproducible build needs the pinned Android SDK/NDK/CMake inputs. The fdroiddata test additionally needs Docker and a public source ref.

F-Droid performs its own source build and signs the resulting APK. The upstream unsigned APK is only a build/inspection artifact.

## Publication status

From `_/build`:

```sh
ysh fdroid/gitlab-status.grease
ysh fdroid/store-status.grease
```

The first command reports the public fdroiddata merge-request state and comments. The second checks F-Droid's public package API/store page against `fdroid/release.properties`. A package 404 is a normal pending state; transport/API failures still fail the check.

## Release versus prerelease

- **Candidate / prerelease:** an artifact from GitHub Actions, an ICK qualification
  branch, or a GitHub prerelease. It is meant for inspection and testing, not
  F-Droid update distribution. The repository's public debug signer is
  **not** the F-Droid release signer.
- **Tagged upstream release:** `v0.2.0` (versionCode 101) is created only after
  the exact main-branch candidate and both Android/F-Droid runs are successful
  **and** the matching APK has been exercised on physical Android hardware.
  A tag alone is not proof of publication.
- **F-Droid submission:** an *open* fdroiddata merge request with the exact
  tagged source, passing F-Droid CI and review. A closed or merely drafted
  request does not count.
- **Published F-Droid release:** F-Droid has built and signed the APK and
  the official package API confirms availability of that version. No Google
  Play account, tax paperwork, screenshots, or weekly submission cadence is
  needed for this release track.

The current GitHub debug APK and an eventual F-Droid-signed APK will normally
have different signing certificates. Android cannot update one in place to the
other. Back up any user data before uninstalling a test build to install the
F-Droid version. Do not call GitHub test APKs interchangeable F-Droid releases.
