# Third-party and earlier material

`LICENSE` applies to copyrightable material in this repository only where the repository's contributors have authority to grant that license. Third-party dependencies, platform components, and separately identified material retain their own licenses.

## Repository-created assets

The Wegert application icon in `artwork/`, the launcher image derivatives under `app/src/main/res/`, the Fastlane copy of the icon, and screenshots generated from the Wegert application are licensed under `GPL-3.0-or-later` as stated in `LICENSE`, unless a file-specific notice says otherwise.

The release APK does not download or bundle third-party fonts, stock artwork, advertising assets, analytics SDKs, or proprietary service libraries.

## Earlier Wegert implementation

The phase-colouring constants documented in `README.md` also appear in an earlier Wegert R implementation published by the same GitHub account:

- https://gist.github.com/isomorphisms/5a30e61fb305ee52bcff

That provenance is recorded here rather than assuming that every historical implementation or source of the mathematical ideas is covered by this repository's GPL grant.

## Android native_app_glue

`CMakeLists.txt` compiles `sources/android/native_app_glue/android_native_app_glue.c` from the installed Android NDK into `libwegert.so`. The Android Open Source Project licenses that source under `Apache-2.0`. Its attribution is preserved in `NOTICE`. No prebuilt native_app_glue object is stored in Wegert.

The resulting APK therefore combines GPL-3.0-or-later Wegert code with Apache-2.0 native glue, which is compatible with distribution of the combined work under GPL-3.0-or-later while preserving the Apache notices.

## DEX generation

`classes.dex` is generated during the direct build from source in `isomorphisms/idric-arm-thumb`; it is not committed to this repository and is not accepted as a prebuilt release input. The direct encoder emits `org.isomorphisms.wegert.WegertActivity`, whose JNI method is implemented by `wegert_jni.c`.

The direct Wegert production path does not use javac, Kotlin, d8, or smali assembly. Smali/baksmali may be used only in the compiler backend's oracle tests and are not production inputs to the APK.

## Platform and build tools

The Android SDK/NDK, CMake, AAPT2, zipalign, apksigner, Chez Scheme, Python, and system libraries are build/platform tools and remain under their upstream terms. Android platform shared libraries such as `libandroid`, `liblog`, `libEGL`, and `libGLESv3` are not copied into the APK.

The checked `complex_math_ick.o` is not a release input for the direct/F-Droid path: `android-direct/build-native.sh` forces `WEGERT_USE_ICK_PREBUILT=OFF` for every ABI and recompiles the fallback source instead.
