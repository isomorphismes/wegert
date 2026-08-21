# Wegert

Interactive Wegert phase portraits of complex rational functions.

This first Android slice is deliberately small: a C `NativeActivity` owns touch input and the EGL/OpenGL ES 3 context, and a fragment shader computes the portrait directly on the GPU. There is no JavaScript layer.

## First playable controls

- one-finger tap: add a simple zero
- two-finger tap: add a simple pole at the midpoint
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

GitHub Actions builds the same APK and uploads it as `wegert-debug-apk`.

## Shader/compiler boundary

The repository already has an Idris2 -> GLSL ES backend at `isomorphisms/idris-shader-backend`. The working portrait shader is kept as direct GLSL for this first slice because the current backend does not yet expose the `atan`, `log`, uniform-array, and bounded-loop operations used by this renderer. Those are a narrow next step; the Android host does not need to change when the shader source becomes Idris-generated.
