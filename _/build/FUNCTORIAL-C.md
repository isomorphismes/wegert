# Functorial C / ICK qualification — 2026-10-06

**FUNCTORIAL + ICK BLOCKED:** application C meets current Bionic headers that
ICK cannot parse (`_Nonnull`, `_Nullable`, Android availability annotations).
The successful freestanding math leaf is not full application qualification.
The existing source-built workflow keeps its required check identifier, but
its receipt explicitly says `application_c=NDK-clang`, `polynomial_leaf=ICK`
(arm64 only) and `full_icky=0`. It retains the stableDebug key, NDK r29 and ABI
set while pinning the current ICK source. Its AArch64/QEMU qualification is a
separate hosted lane, not new A1 physical evidence.

Complex values own zeros, poles, view centres, projected material points,
snapping and dragging. `wegert_function_evaluate` composes numerator and
denominator factor products with complex difference/product/quotient.
Root expansion now composes named multiplication by a linear factor on complex
polynomial coefficients. Its scalar buffer is an intercompiler adapter.
`code/complex_math.c` is the single implementation owner for both compiler
lanes. The existing `complex_math_ick.c` and `complex_math_fallback.c` paths are
source aliases, so their arithmetic cannot drift independently. The maintained
model checks require both paths to resolve to that owner. The exported ABI and
double-precision coefficient arithmetic are retained.

`factor_state.c` owns exact factor cancellation and insertion, with private
search/removal helpers. `factor_snap.c` owns the screen-radius selection rule.
`polynomial_text.c` hides coefficient expansion and formatting behind one useful
interface. Callers no longer include their private implementations.
Canonical source remains in `code/`; the build directory uses soft links.

The GLES portrait path retains its existing shaders and colour mapping.
`complex_values_pack` derives Cartesian uniform buffers from a const model;
polynomial text uses the same explicit adapter. No cast assumes struct packing.
CPU evaluation and packing tests check an independent rational-function value
and bitwise unchanged source state.

Executed: all nine maintained model tests via real Grease and NDK host Clang;
the existing CMake Android native target links at API 26 in A32 as ELF32 ARM,
using explicit `WEGERT_USE_ICK_PREBUILT=OFF`. This diagnostic used NDK r27c;
the maintained product workflows keep their r29 pin and original signer.
Current source-built ICK compiled the real root-expansion leaf at A32/softfp,
but the actual `wegert.c` application probe failed on Bionic declarations.
Neither compilation was a failed ICK attempt followed by automatic Clang.

No GPU runtime, shader-driver acceptance, new APK, replacement installation or
physical MIRO result is claimed by these native diagnostics.
