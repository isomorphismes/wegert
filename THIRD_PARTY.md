# Third-party and earlier material

`LICENSE` applies to copyrightable material in this repository only where the repository's contributors have authority to grant that license. Third-party dependencies, platform components, and separately identified material retain their own licenses.

## Earlier Wegert implementation

The phase-colouring constants documented in `README.md` also appear in an earlier Wegert R implementation published by the same GitHub account:

- https://gist.github.com/isomorphisms/5a30e61fb305ee52bcff

That provenance is recorded here rather than assuming that every historical implementation or source of the mathematical ideas is covered by this repository's GPL grant.

## Android native_app_glue

`_/build/CMakeLists.txt` compiles `sources/android/native_app_glue/android_native_app_glue.c` from the installed Android NDK into `libwegert.so`. The Android Open Source Project licenses that source under `Apache-2.0`. Its attribution is preserved in `NOTICE`. No prebuilt native_app_glue object is stored in Wegert.

## Direct DEX generation

The direct Android lane generates `classes.dex` during the build from the exact source-pinned revisions of `isomorphisms/Idric` and `isomorphisms/idric-arm-thumb` recorded under `_/build/_deps/`. It is not committed to this repository and is not accepted as a prebuilt release input. The direct encoder emits `org.isomorphisms.wegert.WegertActivity`; its JNI probe is implemented by `code/wegert_jni.c`.

The direct path does not use javac, Kotlin, d8, or smali assembly to produce the application DEX. Smali/baksmali may be used only by compiler-backend oracle tests and are not production inputs to the APK.

The checked `_/build/complex_math_ick.o` is not an input to the direct DEX/JNI APK: `_/build/android-direct/build-native.sh` forces `WEGERT_USE_ICK_PREBUILT=OFF` for every ABI. This statement does not change or certify the separate F-Droid release boundary under `_/build/fdroid/`.

## Platform and toolchain

Android SDK/NDK components, Gradle, CMake, AAPT2, zipalign, apksigner, Chez Scheme, Python, system libraries, and OpenGL ES interfaces are external to this repository and remain under their upstream terms. Android platform shared libraries such as `libandroid`, `liblog`, `libEGL`, and `libGLESv3` are not copied into the APK.
