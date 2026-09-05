# Wegert

Interactive Wegert phase portraits of complex rational functions.

This Android slice is deliberately small: a C `NativeActivity` owns touch input and the EGL/OpenGL ES 3 context, and a fragment shader computes the portrait directly on the GPU. There is no JavaScript layer.

## Source layout

The renderer code meant to be read and edited lives directly at repository root: `wegert.c`, the supporting `.h` and complex-math `.c` files, `wegert.frag.in`, `wegert_color.glsl`, and `CMakeLists.txt`. `android-direct/` contains the release DEX/JNI manifest, source-build, packaging, signing, and verification scripts. `_deps/` records exact git submodule commits for the direct DEX backend and Idriç compiler. `app/` remains as the older Gradle compatibility/reference packaging path. The checked AArch64 ICK object and its provenance remain in the repository for comparison, but the direct/F-Droid build forces `WEGERT_USE_ICK_PREBUILT=OFF` and does not consume that object.

## First playable controls

- tap the ○ or × control, then tap the portrait to add that kind of factor (up to 64 each)
- one-finger drag from an existing marker: move that factor
- one-finger drag from empty portrait space: move the visible complex domain
- pinch: zoom the visible domain
- `continuation`: switch from the whole portrait to a continuation view; `whole portrait` switches back
- in continuation view, tap inside the preceding Taylor disc to add the next center
- `clear`: remove every zero and pole in the whole portrait, or clear only the continuation path in continuation view
- three-finger tap: restore `g(z) = (z - 1)(z - 2)(z - 5)`, recenter the camera, return to the whole portrait, and clear the continuation path
- Android Back: leave the activity using the system control rather than an in-app exit button

The initial view is centered at the ordinary complex zero. Zeros are shown as dark rings with light centers; poles are shown as dark crosses.

Selecting ○ or × always returns to the whole portrait before editing factors. The formula is hidden in continuation view, while the clear and view controls remain active and block touches from reaching the portrait.

Touching near an opposite marker snaps to that marker's stored coordinate using a density-aware touch target: placing a zero there removes one pole instead of storing the zero, and placing a pole on a zero works the same way. Cancellation is one-for-one, including repeated factors. This screen-space touch aid does not change the exact-coordinate rule for programmatic factor values, so merely nearby stored factors remain distinct.

Clear and the view switch activate only when the first finger is released inside the same control. Additional fingers cancel that pending control action and cannot turn it into a pinch or three-finger reset. Pinch and three-finger reset remain available when the gesture begins on the portrait.

## Continuation view

Wegert uses the normalized model $g(z)=\prod_j(z-a_j)/\prod_k(z-b_k)$, with complex gain fixed to 1. Within that normalization, the stored zeros and poles determine the rational function exactly. Its direct rational evaluation continues to supply the phase-portrait colours; the continuation view does not invent a function by interpolating tapped values.

The first Taylor-disc center is the camera center when it is a regular point. Only a center exactly equal to an uncancelled pole is mathematically rejected. For usable touch input, a continuation-center tap within a pole's screen-space touch target first snaps to that exact pole and is therefore rejected; the planner itself retains exact Taylor geometry with no epsilon. Each disc radius is the distance from its center to the nearest uncancelled pole. A function with no uncancelled finite poles has an unbounded Taylor disc. Interactive factor insertion keeps the stored arrays reduced as described above; the continuation planner also defensively cancels exact opposite pairs with multiplicity if a future imported or raw state contains them.

A newly tapped center is accepted only inside the immediately preceding open disc. The shader retains the rational portrait inside the revealed union, desaturates and darkens the unrevealed region, and draws the disc boundaries, centers, and path. If the path is empty, the portrait remains darkened until a regular point is tapped as a new seed. The path is deliberately bounded to 24 centers for predictable GLES uniform use.

In the whole portrait, dragging from an existing marker moves only that factor; dragging empty space pans. Factor dragging does not snap or retarget after finger-down. In the continuation view, marker touches remain continuation-center input so a touch near a pole can snap to the exact pole and be rejected.

## Touch demonstrations

Both recordings use the tested [v0.1.50 APK](https://github.com/isomorphismes/wegert/releases/tag/v0.1.50).

- [Place and drag one zero and one pole](docs/demos/add-and-drag-zero-and-pole.mp4)
- [Place and drag two zeros and two poles](docs/demos/add-and-drag-two-zeros-and-two-poles.mp4)

## Colouring

The shader preserves the established Wegert palette constants from the earlier R version:

- HCL chroma: `45`
- lightness base: `66`
- log-modulus contribution: `4`
- hue-band contribution: `3`

Hue is the phase of the rational function. Lightness repeats by base-10 log-modulus decades. HCL is converted to display sRGB in the fragment shader.

## Android build: direct DEX/JNI

The release path generates `classes.dex` directly and rebuilds `libwegert.so` from source for `arm64-v8a`, `armeabi-v7a`, and `x86_64`. It does not use javac, Kotlin, d8, or smali assembly. Smali/baksmali are permitted only as backend oracle tools and are not APK production inputs.

Required build inputs are Android SDK platform 36, build-tools 36.0.0, NDK r29 (`29.0.14206865`), CMake 3.22.1 or compatible, Chez Scheme, a host C toolchain, GMP development headers, Python 3, zip, and unzip.

Initialize the source-pinned compiler inputs and build:

```sh
git submodule update --init --recursive
bash android-direct/generate-dex.sh build/direct/classes.dex

export ANDROID_NDK_HOME="$ANDROID_HOME/ndk/29.0.14206865"
export ANDROID_BUILD_TOOLS="$ANDROID_HOME/build-tools/36.0.0"
bash android-direct/build.sh \
  build/direct/classes.dex \
  build/direct/wegert-direct-unsigned.apk
```

For a local installable test APK:

```sh
bash android-direct/sign-debug.sh \
  build/direct/wegert-direct-unsigned.apk \
  build/direct/wegert-direct-debug.apk
```

The debug key is deliberately public and must never sign a production release. F-Droid consumes the unsigned direct APK and signs it independently.

`classes.dex` defines `org.isomorphisms.wegert.WegertActivity`; its JNI probe is implemented in `wegert_jni.c`, and the manifest still loads the native renderer as `libwegert.so`. The current `armeabi-v7a` library is produced by the Android NDK toolchain. A future ARM Thumb-v7 backend can replace that native compilation stage while leaving the DEX/JNI Android bootstrap unchanged.

The older Gradle project remains only as a compatibility/reference path. Release identity is canonical in `fdroid/release-values.sh`, and the Gradle declarations are checked for drift against it.

The host-side continuation, gesture, pinch-zoom, factor-drag, canonical-factor, touch-snap, complex-arithmetic, and formula-formatting rules can be checked without an Android toolchain:

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic tests/test_continuation_path.c -lm -o /tmp/wegert-continuation-test
/tmp/wegert-continuation-test
cc -std=c11 -Wall -Wextra -Werror -pedantic tests/test_gesture_state.c -lm -o /tmp/wegert-gesture-test
/tmp/wegert-gesture-test
cc -std=c11 -Wall -Wextra -Werror -pedantic tests/test_factor_state.c -o /tmp/wegert-factor-test
/tmp/wegert-factor-test
cc -std=c11 -Wall -Wextra -Werror -pedantic tests/test_factor_snap.c -lm -o /tmp/wegert-factor-snap-test
/tmp/wegert-factor-snap-test
cc -std=c11 -Wall -Wextra -Werror -pedantic tests/factor_drag_test.c -lm -o /tmp/wegert-factor-drag-test
/tmp/wegert-factor-drag-test
cc -std=c11 -Wall -Wextra -Werror -pedantic complex_math_fallback.c tests/complex_math_test.c -lm -o /tmp/wegert-complex-math-test
/tmp/wegert-complex-math-test
cc -std=c11 -Wall -Wextra -Werror -pedantic tests/test_polynomial_text.c complex_math_fallback.c -lm -o /tmp/wegert-polynomial-text-test
/tmp/wegert-polynomial-text-test
```

## Release qualification

`Direct DEX and JNI Android build` rebuilds the DEX and all three native ABIs, verifies the JNI and `ANativeActivity_onCreate` symbols, installs the x86-64 APK on ART, starts `WegertActivity`, requires the JNI receipt, and rejects verifier/class/link failures.

`F-Droid release build` performs a second release-oriented path: it checks the license/Fastlane metadata, generates the DEX twice, rebuilds the unsigned direct APK twice with normalized timestamps and requires byte identity, then runs the metadata/scanner/build recipe in F-Droid's buildserver image.

The existing broader Android workflow remains useful for renderer/gesture emulator coverage, but it is not the source of release APKs.

## Device emulation

The broader Android smoke tests use constrained virtual-device profiles for the renderer and gestures. These are compatibility profiles, not cycle-accurate hardware emulations; real-device testing still covers ARM code generation, vendor GLES behavior, multi-touch, and device-specific Android quirks.

The direct release qualification separately uses an x86-64 Android emulator as an ART/JNI receipt. That test establishes that the generated DEX loads the native library and launches the application; it does not substitute for physical ARM/GPU acceptance.

## Shader/compiler boundary

The repository already has an Idris2 -> GLSL ES backend at `isomorphisms/idris-shader-backend`. The working portrait shader is kept as direct GLSL for this slice because the current backend does not yet expose the `atan`, `log`, uniform-array, and bounded-loop operations used by this renderer. Those are a narrow next step; the Android host does not need to change when the shader source becomes Idris-generated.
