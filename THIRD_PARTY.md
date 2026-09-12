# Third-party and earlier material

`LICENSE` applies to copyrightable material in this repository only where the repository's contributors have authority to grant that license. Third-party dependencies, platform components, and separately identified material retain their own licenses.

## Bundled artwork and media

See [`ASSET_PROVENANCE.md`](ASSET_PROVENANCE.md) for the checked provenance of the launcher-icon family, Fastlane/F-Droid store copies, phone screenshot, recorded touch demonstrations, and generated demo media. That audit distinguishes exact copies, repository-generated outputs, app recordings, and material whose source/holder authority is still unresolved; it does not infer ownership from commit authorship.

The media-rendering paths use external NumPy, Pillow, FFmpeg, R, and, when present, a system DejaVu Sans font. Those are build/runtime inputs rather than vendored repository media and retain their upstream terms.

## Earlier Wegert implementation

The phase-colouring constants documented in `README.md` also appear in an earlier Wegert R implementation published by the same GitHub account:

- https://gist.github.com/isomorphisms/5a30e61fb305ee52bcff

That provenance is recorded here rather than assuming that every historical implementation or source of the mathematical ideas is covered by this repository's GPL grant. `code/Wegert_g_codomain_phase.R` explicitly says its core colour calculation was copied from that gist, so the generated codomain-phase PNG/MP4/GIF retain that provenance chain; see `ASSET_PROVENANCE.md`.

## Platform and toolchain

Android SDK/NDK components, Gradle, CMake, system libraries, and OpenGL ES interfaces are external to this repository and remain under their upstream terms.
