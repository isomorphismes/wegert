# Rendering mistake journal

The point of this file is to keep the mistakes visible.

## 2026-08-29 — I substituted a movie for the renderer

I was wrong.

I produced early Wegert movies with quick Python/NumPy-style rendering instead
of making the repository's actual rendering path produce the frames.  At least
some of the early attempts used generic HSV-like colouring rather than the
established Wegert HCL rule.  The motion could look convincing while the
rendering evidence was false.

The mistake was not merely a palette mismatch.  I had changed the question
from:

> Can Wegert render this animation?

to:

> Can I make a movie that looks roughly like the intended animation?

Those are different tasks.

The user explicitly objected that the resulting movies were not actual Wegert.
That objection was correct.

### Why it happened

The Android renderer was difficult to reuse outside the Android application.
The fastest route to an MP4 was therefore to reconstruct the mathematics and
colouring in Python and feed pixels to ffmpeg.

That shortcut removed renderer identity from the evidence.  It also created
multiple sources of truth for the same mathematics and colour map.

### Lesson

When the real renderer is inconvenient to call, fix the renderer boundary.
Do not route around it.

---

## 2026-08-24 — branch points were presented as ordinary zeros

A separate rendering experiment fed branch points such as
`1, -1, k, -k` through Wegert's ordinary-zero array while also modifying the
phase/log-modulus behavior for a square root.

That was semantically wrong.  The picture marked branch structure as if those
points were ordinary zeros of the represented function.

### Lesson

A renderer data structure is part of the semantics.  Reusing a convenient slot
for a mathematically different object is not harmless just because it produces
a visible marker.

Branch points, zeros, poles, continuation boundaries, and other structures need
distinct representation when their meanings differ.

---

## 2026-08-29 to 2026-09-23 — duplicated rational evaluation and colouring

The later Python movie generator improved the palette by copying the constants
and formulas from `wegert_color.glsl`, but it still had its own:

- rational-function evaluator;
- phase/log-modulus computation;
- HCL-to-sRGB implementation;
- zero/pole marker drawing;
- text/annotation drawing.

This was closer numerically but still architecturally wrong.

A copied implementation can drift while continuing to look plausible.  It also
cannot establish that the production GLSL, GLES runtime, compiler-generated
shader, or physical GPU produced the image.

The six rational MP4s archived here are from that duplicated path.

### The worst divergence

`wegert_meromorphic_15s.mp4` used a second colouring function,
`dark_rgb()`, based on HSV/rings rather than the canonical Wegert HCL colour
core.  It also included `exp(0.18 z)`, which the rational zero/pole scene model
did not represent.

That file is therefore not merely weak evidence.  Its rendering semantics are
different.

---

## Evidence mistake — handwritten output is not compiler evidence

Another recurring mistake was treating a correctly rendered handwritten
C/GLSL path as if it demonstrated the Idriç/Idris shader backend.

It does not.

A useful separation is:

```text
source language
    ↓
shader backend
    ↓
GLSL ES
    ↓
GLES / driver
    ↓
GPU
    ↓
framebuffer
```

A successful handwritten GLSL render can test the GLES/driver/GPU layers while
saying nothing about whether the shader backend produced correct code.

The shader-backend work, including structured-control-flow work, must be
accepted with artifacts actually emitted by that backend.

---

## 2026-09-23 — duplicated UI geometry exposed another version of the same bug

The zero/pole placement controls originally computed their geometry twice:

- C computed positions for touch hit-testing;
- GLSL independently computed positions for drawing.

During the refactor this exposed the Android top-origin versus OpenGL
bottom-origin Y-coordinate distinction.

This was caught and fixed before accepting the refactor, but it is the same
class of architectural failure: two implementations of one fact.

### Lesson

Compute semantic/layout state once and let drawing and hit-testing consume it.

---

# What we changed

The refactor on branch `refactor` changed the dependency structure instead of
adding another renderer.

## Mathematical and view state

```text
wegert_function
      +
wegert_view
      ↓
wegert_scene
```

The scene can exist without Android.

## One portrait operation

`wegert_portrait_renderer_gles_render_frame(renderer, scene)` now renders one
mathematical portrait into the currently bound GLES framebuffer.

The portrait renderer does not know about Android activities, touch input,
movies, icons, or ffmpeg.

## Shared control geometry

Placement-control layout is calculated in C once and consumed both by
hit-testing and the control shader.

## UI separated from portrait rendering

Equation display and clear-button rendering moved out of `struct engine` into
a separate GLES UI component.  The current on-screen placement is only one
composition choice; equation input/output, controls, feedback, and future
console-like interaction should not be defined by the word "overlay."

## Persistent offscreen rendering

The headless path creates one EGL pbuffer and one GLES context, compiles/links
the portrait shader once, then renders successive `wegert_scene` values.

```text
create context once

scene 0 → render → RGB
scene 1 → render → RGB
scene 2 → render → RGB
...

destroy context once
```

The movie sequencer now chooses scene trajectories and hands them to the C
renderer.  It no longer implements rational evaluation or Wegert colouring.

This directly fixes the architectural reason the Python duplicate renderer was
created in the first place.

---

# Rules going forward

1. A rendered artifact must identify the renderer that actually produced its
   pixels.
2. A movie must use the same mathematical rendering path as a still frame unless
   it is explicitly labeled as a different reference renderer.
3. Compiler-backend acceptance requires backend-produced code.
4. Mesa or SwiftShader receipts are compatibility evidence, not physical
   PowerVR/Mali evidence.
5. Do not duplicate geometry, mathematical evaluation, or colour semantics just
   to make another output format convenient.
6. Preserve mistakes and document them rather than silently replacing history.
