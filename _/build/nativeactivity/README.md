# Paired A1/C67 non-Gradle qualification

This candidate produces separately installable APKs from one exact Wegert
revision for MIRO A1 (armeabi-v7a) and C67 (arm64-v8a), preserving
org.isomorphisms.wegert, versionCode, test certificate, icons and the visible
name zero & infinity. Cat Food supplies the companion plan, ai-ci the accepted
signer registry, and android-NDK the direct NativeActivity packager.

The source-built AArch64 ICK complex-math object is interposed only for the
C67 library; the remaining native C code and final link use NDK. A1 uses NDK
pending full ICK Android-runtime qualification. This is NOT a complete ICK
application or proof of either physical device's runtime behavior.

A candidate policy checkout is not publication authority. Physical install/
update/render behavior and F-Droid rebuilding need independent exact receipts.
