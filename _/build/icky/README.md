# Direct division source and actual producers

Twenty maintained C/header expressions use `÷` directly. Canonical source remains under `code/`; all 65 existing C/header symlinks are preserved. URLs, shader division, literal text, and frozen compiler controls are separate from maintained C arithmetic.

The selected compiler is ICK `fbe86e23d55cfec2000c08e61deea2a407fd7175`, through shared interface `isomorphisms/ai-ci@f6d825d15cd3c0c34ae1b43240a463ff5d343090`. `OwnedC.cmake` compiles every selected application C unit to assembly. NDK r29 assembles that output, compiles its unchanged NativeActivity glue and links the application. The independently selected complex-math object/source option remains explicit. The direct DEX/JNI path adds its existing JNI C unit through the same ICK producer.

The complete inherited NDK CMake flags remain: API26, Fortify2, stack protection, build-type optimization/debug intent and existing warnings. The bounded checked header adapter preserves Bionic checking for the functions used here, including snprintf/vsnprintf. Unsupported calls fail. No source normalization or stock-C retry exists.

The Android, source-built-complex, direct DEX/JNI, F-Droid and icon workflows restore three checked ABI stages into `_/build/ick-stages` using `Restore.mk`. F-Droid independently builds those stages from pinned source libraries, retains its clean double-build and byte-identical APK criterion, and still packages without Gradle. Existing signed APK, ART, emulator, source-built complex/QEMU, release identity and physical-device gates remain distinct.

The native Android workflow and headless renderer select qualified native ICK. `Host.mk` preserves all nine original test programs and their original optimization choices. The renderer is the actual existing EGL/GLES frame driver, not an app-runtime substitute.

Local source qualification passed all nine host programs. Complete ARMv7, AArch64 and x86_64 r29/API26 Debug libraries with direct JNI, Cartesian fallback and Fortify2 compile and link. The actual native frame driver rendered the original two-frame CI fixture (18,432 RGB bytes); its digest is recorded in `local-evidence.tsv`. Existing metadata/screenshot hash checks pass. These checks do not claim a fresh signed APK, ART/emulator, F-Droid reproducibility run or physical-device acceptance; those exact workflows remain required.

The compiler pin now includes the complex-member extraction repair at
`fbe86e23d55cfec2000c08e61deea2a407fd7175`. The canonical complex source uses
`__real__`/`__imag__`, so its producer is covered by the repaired extraction
class. Wegert's existing arithmetic-rich complex test passed with the previous
compiler; it is not described as an observed failure. The historical checked
AArch64 object remains a separate, unchanged input with its existing provenance.

With the repaired native frontend, all nine original host programs pass. Both
complete x86-64 r29/API26 Debug JNI libraries compile and link: the default
Cartesian source selection and the explicit owned-complex source selection.
`complex-extraction-evidence.tsv` records their digests. Fresh ARM/AArch64
compiler, source-built-object/QEMU, APK and emulator results remain obligations
of the updated exact-head workflows; prior receipts are not relabeled.

The hosted F-Droid reproducibility lane keeps both application trees as clean
`git archive` extractions. It passes the explicitly pinned shared interface and
current-run compiler stages as external immutable build inputs, just as it
passes the declared NDK r29. Before each application build, `Restore.mk verify`
checks the shared Git revision and all consumed shared files, every archived
compiler-stage member and the exact member inventory (only `.restored` may be
extra), source/ABI/API26/Fortify2 receipts, and relocated driver/frontend hashes.
The independent production F-Droid lane still bootstraps and qualifies all three
compilers from its pinned source libraries. Metadata normalization, APK scanning,
the two-build byte comparison and branch-specific release gates are unchanged.
