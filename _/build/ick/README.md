# Complete ARMv7 ICK application qualification

This is a build-host Grease procedure for the existing NativeActivity app.
It compiles every application C translation unit, including the complex-math
implementation, directly with current ICK through the shared CMake boundary.
NDK r29 compiles its unchanged NativeActivity glue, assembles ICK output, links
and packages the APK. A second library is compiled with NDK Clang from the
immutable pre-migration source at `71e6f969b102d70a06ebb2d98b6426f79f287173`
for the existing export/ABI comparison. Current glyph source is never rewritten.

Inputs are an exact ICK `fbe86e23d55cfec2000c08e61deea2a407fd7175` checkout,
its qualified installed ARM compiler stage, NDK r29, SDK 36, a new output
directory and the real Grease invocation. The shared interface checkout under
`_/ai-ci-ick` must be `f6d825d15cd3c0c34ae1b43240a463ff5d343090`.
The procedure verifies source and interface revisions, requires cc1 to remain
inside the installed stage and reruns the shared ARM/Bionic/Fortify2 qualifier.

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

1. Exact current ICK source checkout;
2. qualified installed `armeabi-v7a` stage containing `bin/arm-linux-gnueabi-gcc`;
3. NDK r29 directory;
4. Android SDK directory with build-tools 36.0.0 and platform 36;
5. a new absolute output directory;
6. the Grease executable, followed by any required runtime arguments.

This produces an ARMv7-only qualification APK under the established package,
version and public test signing identity. It does not certify replacement
installation, application behavior, physical MIRO A1 execution, C67 execution,
16 KiB runtime compatibility, numerical/rendered output equivalence, or F-Droid
reproducibility. Those acceptance stages remain separate.

The checked `evidence/` files remain historical qualification of the earlier
ICK515 archive. They are not relabeled as evidence for the current source.
