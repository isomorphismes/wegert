# Direct division source and actual producers

Twenty maintained C/header expressions use `÷` directly. Canonical source remains under `code/`; all 65 existing C/header symlinks are preserved. URLs, shader division, literal text, and frozen compiler controls are separate from maintained C arithmetic.

The selected compiler is ICK `c61e448251744a2f40ad743ebef1a027bdcd2f9d`, through shared interface `isomorphisms/ai-ci@4ea071a96239f3a29ca6d98454feb59947d87cfe`. `OwnedC.cmake` compiles every selected application C unit to assembly. NDK r29 assembles that output, compiles its unchanged NativeActivity glue and links the application. The independently selected complex-math object/source option remains explicit. The direct DEX/JNI path adds its existing JNI C unit through the same ICK producer.

The complete inherited NDK CMake flags remain: API26, Fortify2, stack protection, build-type optimization/debug intent and existing warnings. The bounded checked header adapter preserves Bionic checking for the functions used here, including snprintf/vsnprintf. Unsupported calls fail. No source normalization or stock-C retry exists.

The Android, source-built-complex, direct DEX/JNI, F-Droid and icon workflows restore three checked ABI stages into `_/build/ick-stages` using `Restore.mk`. F-Droid independently builds those stages from pinned source libraries, retains its clean double-build and byte-identical APK criterion, and still packages without Gradle. Existing signed APK, ART, emulator, source-built complex/QEMU, release identity and physical-device gates remain distinct.

The native Android workflow and headless renderer select qualified native ICK. `Host.mk` preserves all nine original test programs and their original optimization choices. The renderer is the actual existing EGL/GLES frame driver, not an app-runtime substitute.

Local source qualification passed all nine host programs. Complete ARMv7, AArch64 and x86_64 r29/API26 Debug libraries with direct JNI, Cartesian fallback and Fortify2 compile and link. The actual native frame driver rendered the original two-frame CI fixture (18,432 RGB bytes); its digest is recorded in `local-evidence.tsv`. Existing metadata/screenshot hash checks pass. These checks do not claim a fresh signed APK, ART/emulator, F-Droid reproducibility run or physical-device acceptance; those exact workflows remain required.
