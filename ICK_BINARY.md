# ICK arm64 complex-math object

`complex_math_ick.o` is the isolated AArch64 object used only by the `arm64-v8a` Android build. Its public ABI is the scalar/array function declared in repository-root `complex_math.h`; `_Complex` never crosses into NDK-compiled code.

Source: repository-root `complex_math_ick.c`.

Compiler provenance:

- ICK repository: `isomorphisms/the-equality-sign-means-equality`
- ICK source commit: `7458b3c29fe535eb7dda3b1c756b362cee5c889d`
- merged by PR #1 into `circles-are-balanced` as `5fe6f6d1259b0b4ae9adf99d354e49e2a01afbf9`
- successful qualification run: `32538975306`
- compiler Actions artifact id: `9466706242`
- artifact digest: `sha256:d4299db53e415f2f6da519fd08fe10415f65755b7024a60cc23a4fede83d36fb`
- compiler archive SHA-256 inside the artifact: `8c19eed5aeda0035afe9852cb1e65cde5325f4c031b2f0b39377dc53cb028051`

The source was compiled by that ICK compiler at `-O2 -fPIC -fvisibility=hidden` to AArch64 assembly. The checked object was assembled from the ICK output with Clang's AArch64 integrated assembler because the local generation environment did not contain `aarch64-linux-gnu-as`; the C lowering and `_Complex` code generation are ICK's. The upstream qualification run uses GNU AArch64 binutils for the same isolated-object boundary.

Object SHA-256: `818de69db8978fa53a99e12af96f0a607d3e0b9dfda412db90b23fd50f275dec`.

Undefined symbols are only `atan2`, `hypot`, and `sincos`, used while converting between ICK's polar physical representation and the Cartesian scalar/array boundary. There are no `__mul*complex` or `__div*complex` helper references; complex multiplication is emitted by ICK itself.
