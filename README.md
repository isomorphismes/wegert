# Wegert

Interactive Wegert phase portraits of complex rational functions.

This first Android slice is deliberately small: a C `NativeActivity` owns touch input and the EGL/OpenGL ES 3 context, and a fragment shader computes the portrait directly on the GPU. There is no JavaScript layer.

## First playable controls

- tap the ○ or × control, then tap the portrait to add that kind of factor (up to 64 each)
- one-finger drag: move the visible complex domain
- pinch: zoom the visible domain
- three-finger tap: reset to `g(z) = (z - 1)(z - 2)(z - 5)`

The initial view is centered at the ordinary complex zero. Zeros are shown as dark rings with light centers; poles are shown as dark crosses.

## Colouring

The shader preserves the established Wegert palette constants from the earlier R version:

- HCL chroma: `45`
- lightness base: `66`
- log-modulus contribution: `4`
- hue-band contribution: `3`

Hue is the phase of the rational function. Lightness repeats by base-10 log-modulus decades. HCL is converted to display sRGB in the fragment shader.

## Android build

Requirements are Android SDK 36, NDK r29 (`29.0.14206865`), CMake 3.22.1, JDK 17, Gradle 9.5, and Android Gradle Plugin 9.3.1.

```sh
gradle :app:assembleDebug
```

The APK is written to:

```text
app/build/outputs/apk/debug/app-debug.apk
```

The debug APK contains `arm64-v8a` for the actual phone/tablet targets and `x86_64` solely for CI emulation.

## Device emulation

GitHub Actions smoke-tests the APK against two constrained virtual-device profiles:

| Target | Android | RAM | logical display | CI CPU/GPU |
| --- | --- | ---: | --- | --- |
| MIRO A1 approximation | 14 / API 34 | 2 GiB | 720x1280 @ 320 dpi | x86_64 / SwiftShader GLES |
| TAB_P10 approximation | 15 / API 35 | 4 GiB | 1280x800 @ 160 dpi | x86_64 / SwiftShader GLES |

Each emulator installs and launches Wegert, performs a tap and a drag, checks that the native process survives, fails on EGL/shader/link/fatal errors, and saves a screenshot plus application log.

These are compatibility profiles, not cycle-accurate hardware emulations. In particular the CI tablet cannot reproduce the Allwinner A333/Mali-G57 driver. Real-device testing still covers ARM64 code generation, vendor GLES behavior, multi-touch, and device-specific Android quirks. The MIRO profile similarly constrains Android 14 to 2 GiB but is not an Android Go system image.

## Shader/compiler boundary

The repository already has an Idris2 -> GLSL ES backend at `isomorphisms/idris-shader-backend`. The working portrait shader is kept as direct GLSL for this first slice because the current backend does not yet expose the `atan`, `log`, uniform-array, and bounded-loop operations used by this renderer. Those are a narrow next step; the Android host does not need to change when the shader source becomes Idris-generated.
