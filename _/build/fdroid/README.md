# F-Droid release path

This directory is build/release machinery. From the repository root, either run commands with the full `_/build/...` path or `cd _/build` first.

Wegert remains the internal project/package name. F-Droid publishes it as **zero & infinity**. Release identity comes from `fdroid/release.properties`; the F-Droid recipe builds directly with NDK/CMake, `aapt2`, and `zipalign`, without Gradle.

## Release contract

1. Keep `fdroid/release.properties`, `app/build.gradle.kts`, and `fdroid/org.isomorphisms.wegert.yml.template` on the same version name/code.
2. Merge the release candidate to `main` and record the exact successful Android and F-Droid workflow runs.
3. Install the tested Android artifact on the release phone/tablet and exercise factor placement/dragging, pan, pinch, continuation, clear, and Android Back.
4. Only then run `Tag tested F-Droid release` for that exact source SHA and the two successful run IDs.
5. `Publish tested APK` requires the same tested source and immutable tag.
6. Copy `fdroid/org.isomorphisms.wegert.yml.template` into fdroiddata and submit the upstream merge request.

The F-Droid workflow makes two clean direct builds and requires byte-identical unsigned APKs. It also runs the recipe inside F-Droid's production-like buildserver image and checks metadata, scanner output, ABI packaging, and the upstream Fastlane metadata.

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
