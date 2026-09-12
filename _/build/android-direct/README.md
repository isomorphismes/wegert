# Direct DEX/JNI Android path

This directory is build and qualification machinery for the direct DEX/JNI Android path.

`classes.dex` is generated from the exact source-pinned Idriç and Android/DEX backend revisions recorded as gitlinks under `_/build/_deps/`. The same exact-head CI job builds `libwegert.so` for arm64-v8a, armeabi-v7a, and x86_64 with the direct JNI boundary enabled, packages and signs the test APK, and records source and artifact hashes. A dependent ART job installs that exact artifact and requires the generated `WegertActivity` to cross JNI and return the sentinel from `code/wegert_jni.c`.

The F-Droid release path remains separate under `_/build/fdroid/`. Direct DEX/JNI qualification does not certify, replace, or alter the F-Droid source-build recipe. F-Droid continues to build from its own declared source boundary and sign its own APK.
