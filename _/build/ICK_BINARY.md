# ICK arm64 complex-math object

`complex_math_ick.o` is the isolated AArch64 object used only by the
`arm64-v8a` Android build. Build-only object/provenance files live under
`_/build/`; readable source remains at repository root.

Public ABI: repository-root `complex_math.h`.
Source: repository-root `complex_math_ick.c`.
`_Complex` never crosses into NDK-compiled code.

## Checked object provenance

The checked `complex_math_ick.o` predates ICK's move into its dedicated source
repository. Preserve that history rather than pretending the object was produced
by the newer repository layout.

- historical source repository: `isomorphisms/rhs`
- historical ICK source commit:
  `7458b3c29fe535eb7dda3b1c756b362cee5c889d`
- merged into the old `circles-are-balanced` line as
  `5fe6f6d1259b0b4ae9adf99d354e49e2a01afbf9`
- successful qualification run: `32538975306`
- compiler Actions artifact id: `9466706242`
- artifact digest:
  `sha256:d4299db53e415f2f6da519fd08fe10415f65755b7024a60cc23a4fede83d36fb`
- compiler archive SHA-256 inside the artifact:
  `8c19eed5aeda0035afe9852cb1e65cde5325f4c031b2f0b39377dc53cb028051`

The historical source was compiled by ICK at
`-O2 -fPIC -fvisibility=hidden` to AArch64 assembly. The checked object was
assembled from the ICK output with Clang's AArch64 integrated assembler because
the local generation environment did not contain `aarch64-linux-gnu-as`; the C
lowering and `_Complex` code generation are ICK's.

Checked object SHA-256:
`818de69db8978fa53a99e12af96f0a607d3e0b9dfda412db90b23fd50f275dec`.

Its undefined symbols are only `atan2`, `hypot`, and `sincos`, used while
converting between ICK's polar physical representation and the Cartesian
scalar/array boundary. There are no `__mul*complex` or `__div*complex`
helper references; complex multiplication is emitted by ICK itself.

## Current source-built verification

ICK now owns its compiler source in `dilapidated-shed/ick`. The historical RHS
commits above remain provenance only; Wegert must not treat RHS as the active
compiler source.

`.github/workflows/ick-source.yml` pins ICK commit
`6972a59d95feb7292da9b5fd9b67a26706f1b6de`. That lane:

1. checks out ICK and its pinned GCC reference;
2. materializes the ICK-owned `ick/source/` and `ick/PRUNE` source tree;
3. builds the AArch64 ICK compiler from source;
4. removes Wegert's checked object before compiling;
5. rebuilds Wegert's actual `complex_math_ick.c` as AArch64 PIC while
   reserving Android's `x18` register;
6. verifies the object ABI, public symbol, and absence of unexpected complex
   helper calls;
7. executes Wegert's complex-math test against that object under AArch64 QEMU;
8. places that freshly built object into the current `_/build/` tree;
9. builds the complete debug APK and verifies its three supported native ABI
   entries.

The source-built object's hash is recorded as evidence, but it is not required
to equal the older checked object's hash: the current lane intentionally tests
the current ICK source-of-truth and current Android ABI flags rather than
recreating an obsolete build environment byte-for-byte.

This lane is Wegert consumer evidence. It does not claim ARM64 Android runtime
execution, full multi-ABI ICK adoption, or F-Droid source rebuilding with ICK.
Those remain separate qualification boundaries.
