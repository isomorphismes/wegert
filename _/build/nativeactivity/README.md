# Paired A1/C67 non-Gradle qualification

This candidate produces separately installable APKs from one exact Wegert
revision for MIRO A1 (armeabi-v7a) and C67 (arm64-v8a), preserving
org.isomorphisms.wegert, versionCode, test certificate, icons and the visible
name zero & infinity. Cat Food supplies the companion plan, ai-ci the accepted
signer registry, and android-NDK the direct NativeActivity packager.

The source-built AArch64 ICK complex-math object remains selected for the
C67 library. Both A1 and C67 now compile every other maintained C unit with
the qualified ICK division frontend through the shared CMake interface.
NDK compiles its unchanged NativeActivity glue, assembles the ICK output and
links the libraries. Three restored compiler stages and the pinned shared
interface are required as described in `../icky/README.md`. This producer
qualification does not establish either physical device's runtime behavior.

A candidate policy checkout is not publication authority. Physical install/
update/render behavior and F-Droid rebuilding need independent exact receipts.
