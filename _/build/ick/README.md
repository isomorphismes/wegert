# Complete ARMv7 ICK application qualification

This is a build-host Grease procedure for the existing NativeActivity app.
It compiles every application C translation unit, including the complex-math
implementation, and the NDK native-activity glue with ICK. The NDK owns the
Android platform link and APK resource packaging. A second library is compiled
with NDK Clang from the same sources for export/ABI comparison.

Inputs are a verified compiler archive, an ICK checkout containing the shared
application qualifier, NDK r29, SDK 36, a new output directory, and the real
Grease invocation. The procedure pins the compiler archive to ICK
515c0f29fe6e2e96e10495fbaf25da93532e7722 and verifies its published SHA-256.
It checks the compiler driver and cc1 individually after extraction.

The build's domains are source revisions, SHA-256 digests, compiler artifacts,
ARMv7/A32/softfp objects, Android shared libraries, and signed test APKs.
Compilation consumes a source file and explicit platform declarations and
produces an object or a terminal failure. Linking consumes only the objects
already produced. Packaging consumes the linked ICK library, canonical assets,
manifest and stable public test signer. No failure admits another compiler.

Grease is the shell orchestration language. No new general-purpose helper or
Python/JavaScript fallback is introduced. The NativeActivity route requires no
application Java, Gradle or generated DEX.

Invoke the actual Grease runtime with `qualify-armv7.grease` and arguments:

1. ICK checkout containing `qualification/android-boundary/compile-application.grease`;
2. exact compiler ZIP from artifact 11466225121 in
   https://github.com/dilapidated-shed/ick/actions/runs/37581942577;
3. NDK r29 directory;
4. Android SDK directory with build-tools 36.0.0 and platform 36;
5. a new absolute output directory;
6. the Grease executable, followed by any required runtime arguments.

This produces an ARMv7-only qualification APK under the established package,
version and public test signing identity. It does not certify replacement
installation, application behavior, physical MIRO A1 execution, C67 execution,
16 KiB runtime compatibility, numerical/rendered output equivalence, or F-Droid
reproducibility. Those acceptance stages remain separate.
