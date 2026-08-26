# Wegert

Interactive Wegert phase portraits of complex rational functions.

This first Android slice is deliberately small: a C `NativeActivity` owns touch input and the EGL/OpenGL ES 3 context, and a fragment shader computes the portrait directly on the GPU. There is no JavaScript layer.

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

Both recordings use the tested [v0.1.50 APK](https://github.com/isomorphisms/wegert/releases/tag/v0.1.50).

- [Place and drag one zero and one pole](docs/demos/add-and-drag-zero-and-pole.mp4)
- [Place and drag two zeros and two poles](docs/demos/add-and-drag-two-zeros-and-two-poles.mp4)

## Colouring

The shader preserves the established Wegert palette constants from the earlier R version:

- HCL chroma: `45`
- lightness base: `66`
- log-modulus contribution: `4`
- hue-band contribution: `3`

Hue is the phase of the rational function. Lightness repeats by base-10 log-modulus decades. HCL is converted to display sRGB in the fragment shader.

## Android build

Requirements are Android SDK 36, NDK r29 (`29.0.14206865`), CMake 3.22.1, JDK 17, Gradle 9.5.1, and Android Gradle Plugin 9.3.1.

```sh
gradle :app:assembleDebug
```

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
cc -std=c11 -Wall -Wextra -Werror -pedantic -Iapp/src/main/cpp tests/factor_drag_test.c -lm -o /tmp/wegert-factor-drag-test
/tmp/wegert-factor-drag-test
cc -std=c11 -Wall -Wextra -Werror -pedantic app/src/main/cpp/complex_math_fallback.c tests/complex_math_test.c -lm -o /tmp/wegert-complex-math-test
/tmp/wegert-complex-math-test
cc -std=c11 -Wall -Wextra -Werror -pedantic tests/test_polynomial_text.c app/src/main/cpp/complex_math_fallback.c -lm -o /tmp/wegert-polynomial-text-test
/tmp/wegert-polynomial-text-test
```

The APK is written to:

```text
app/build/outputs/apk/debug/app-debug.apk
```

Debug APKs use the repository's public test-only signing key and the source-controlled `0.2.0` version identity. Builds from before this key was added used disposable runner keys and must be uninstalled once before the first stable-signed APK will install.

The checked-in debug key is deliberately public and must never sign a production release.

The debug APK contains `arm64-v8a` and `armeabi-v7a` for phone/tablet targets and `x86_64` solely for CI emulation.

## Device emulation

GitHub Actions smoke-tests the APK against two constrained virtual-device profiles:

| Target | Android | RAM | logical display | CI CPU/GPU |
| --- | --- | ---: | --- | --- |
| MIRO A1 approximation | 14 / API 34 | 2 GiB | 720x1280 @ 320 dpi | x86_64 / SwiftShader GLES |
| TAB_P10 approximation | 15 / API 35 | 4 GiB | 1280x800 @ 160 dpi | x86_64 / SwiftShader GLES |

Each emulator installs and launches Wegert, drags an existing zero in the whole portrait, places a finite pole away from the camera, and enters continuation view. The smoke test requires positive finite radii for both the camera seed and an accepted center inside that disc, then requires rejection of a tap outside the new disc. It pans after those geometry checks, returns to the whole portrait, activates clear, leaves through Android Back, fails on EGL/shader/link/fatal errors, and saves a screenshot plus application log.

These are compatibility profiles, not cycle-accurate hardware emulations. In particular the CI tablet cannot reproduce the Allwinner A333/Mali-G57 driver. Real-device testing still covers ARM64 code generation, vendor GLES behavior, multi-touch, and device-specific Android quirks. The MIRO profile similarly constrains Android 14 to 2 GiB but is not an Android Go system image.

## Shader/compiler boundary

The repository already has an Idris2 -> GLSL ES backend at `isomorphisms/idris-shader-backend`. The working portrait shader is kept as direct GLSL for this first slice because the current backend does not yet expose the `atan`, `log`, uniform-array, and bounded-loop operations used by this renderer. Those are a narrow next step; the Android host does not need to change when the shader source becomes Idris-generated.
