# Agent instructions

Apply the shared evidence and acceptance guardrails in
`isomorphisms/ai-ci/AGENTS.md`. The rules below are Wegert-specific.

## Rendering evidence must come from the running app

When a task asks for a screenshot, image, video, animation, or other visual
proof of this app, capture the exact requested build while its process is
running. A successful build, shader compile, unit test, fixture, or standalone
render is not app-runtime evidence.

Do not substitute AI-generated frames, Python/ffmpeg/HTML renderers, test
harnesses, lookalike reconstructions, cross-fades, interpolation, optical flow,
or other synthesized motion while claiming to show app behavior. If behavior is
missing, change the app and capture the changed app.

If a compiler or rendering backend is named, the executed artifact must contain
and select code produced by that backend. Handwritten C is not Idriç-backend
evidence; handwritten GLSL is not `idris-shader-backend` evidence; a CPU fallback
is not GPU evidence; SwiftShader is not physical PowerVR evidence; shader
emission or validation without a live draw is not rendering evidence. Make
backend selection and fallback status observable.

## Preserve continuous mathematical motion

Use one continuous running process for a continuous demonstration. Persistent
objects and deformation state must evolve continuously; do not rerandomize,
respawn, retype, or reset them per frame. A delayed frame must not become an
unintended teleport.

If candidate steps are scored and a different blended or larger step is applied,
validate the applied motion itself. Do not certify arbitrary intervening motion
from sparse candidate checks.

Poles remain poles and zeros/holes remain zeros/holes unless the mathematical
model explicitly changes. If both the field/background and markers are supposed
to evolve, both must evolve in the running app rather than being synthesized in
post-production.

Preserve the established Wegert color mapping for the same function, viewport,
and state unless the task explicitly changes the visual language. Do not alter
hue, saturation, brightness, contrast, gamma, or exposure merely to make motion
more visible.

## Capture without fabricating behavior

Record enough provenance to reproduce visual evidence: repository revision,
artifact identity, compiler/backend revision when relevant, device/emulator and
GPU/renderer identity, package/process identity, launch path, and observed
backend/fallback selection.

Post-processing may trim, convert containers, remove audio, resize ordinarily,
or crop non-content device chrome. It may not invent motion, repair jumps,
change colors, stabilize behavior, or replace frames. Keep an uncropped/raw
runtime capture when it is needed for provenance. Unless interaction or device
UI is the subject, the presentation artifact should show the visualization
rather than status bars, navigation chrome, debug overlays, or setup controls.

## Preserve the source-facing layout

Build systems, packaging, generated files, test harnesses, compiler machinery,
receipts, and other build-time support belong under `_` in this repository.
Keep maintained source canonical in its source location and preserve intended
top-level soft links. Do not create duplicate writable copies merely to satisfy
a familiar Android or build-system layout.
