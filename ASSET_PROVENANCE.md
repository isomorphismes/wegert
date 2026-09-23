# Bundled artwork and media provenance

This file records provenance facts supported by the checked-in files and repository history. It deliberately does **not** infer copyright ownership from commit authorship. `LICENSE` applies only to material for which the repository's contributors have authority to grant GPL-3.0-or-later.

## Launcher icon family

Canonical/current copies:

- `rendered_images/wegert-icon-512.png`
- `_/build/artwork/wegert-icon-512.png`
- `_/build/app/src/main/play/listings/en-US/graphics/icon/icon.png`
- root `wegert-icon-512.png` is a symlink to the canonical rendered image

The current icon is a generated Wegert portrait of the right-handed trefoil Jones
polynomial convention `V(z) = z + z^3 - z^4`.  The checked workflow
`.github/workflows/jones-trefoil-icon.yml` builds an Android capture APK, runs
the actual GLES Wegert renderer, and uses the canonical
`code/wegert_color.glsl` colour mapping.  The polynomial is represented by its
four zeros; the leading `-1` is applied as a phase rotation by pi.

The workflow captures a 512x512 runtime frame.  The only presentation
post-processing is the documented `460x460+26+0` crop that removes Android
navigation-button chrome, followed by resizing back to 512x512.  It then derives
the Android density, round, and adaptive-foreground files from that captured
image.  The canonical/store copies and Android launcher family are therefore
reproducible from checked source rather than an undocumented imported image.

The previous phase-portrait icon introduced by commit
`a9a28fd0ea5277406ba7d5c4807eec29d28dbb1a` is preserved at
`rendered_images/icon-candidates/original-phase-portrait.png`.  Its original
source/holder/licensing authority remains unresolved as documented in repository
history; retaining it as an alternate candidate does not infer ownership.

The Jones-trefoil candidate and its generation notes live under
`rendered_images/icon-candidates/`.

## Store screenshot

Current store screenshot:

- `_/build/app/src/main/play/listings/en-US/graphics/phone-screenshots/1.png`

Commit `944adcce4927b84e795354cff8e75be7eada7417` added the first Fastlane phone screenshot. Commit `a685c58adf7f6b1d99f46727ea65334937a97b45` (`Fix F-Droid phone screenshot asset`) replaced it with the current image; later history only moved/renamed the store path.

No checked-in capture command, source file, import URL, or attribution identifies where the current screenshot came from or who captured it.

**Copyright/license status:** unresolved. The capture/import source and licensing authority need a copyright-holder statement; commit authorship alone is not used as proof.

## Recorded touch demonstrations

Current files:

- `rendered_images/add-and-drag-zero-and-pole.mp4`
- `rendered_images/add-and-drag-two-zeros-and-two-poles.mp4`

Their root-level names are symlinks to these canonical files. Commit `b1c17ce62067069b93c2e1901a3a2f12acaa2818` (`Add zero and pole touch demonstrations`) introduced both recordings. The contemporaneous README change states that both recordings use the tested Wegert `v0.1.50` APK.

That establishes what application/version was recorded, but the checked history does not identify the recorder or copyright holder.

**Copyright/license status:** recorder/holder authority remains to be confirmed. If the recordings are contributor-owned or otherwise authorized for this repository, the root GPL-3.0-or-later grant applies.

## Programmatically generated demonstrations

`_/build/render_root_videos.py` programmatically renders and encodes these checked-in videos:

- `rendered_images/two_moving_simple_poles.mp4`
- `rendered_images/three_moving_simple_roots.mp4`
- `rendered_images/moving_simple_and_repeated_roots.mp4`
- `rendered_images/moving_simple_and_double_poles.mp4`
- `rendered_images/moving_two_simple_zeros_and_two_simple_poles.mp4`
- `rendered_images/moving_repeated_zeros_and_poles.mp4`
- `rendered_images/wegert_meromorphic_15s.mp4`

The renderer uses the checked-in mathematical/rendering logic plus external NumPy, Pillow and FFmpeg tooling. It may use a system DejaVu Sans font when present. Those tools/fonts are build inputs and are not vendored into these MP4 files as repository source.

These MP4s are therefore **generated outputs, not imported media**. Generation provenance does not by itself prove who owns every copyrightable element of an output; the repository license applies only to the extent contributors have authority over the resulting material.

## R-generated codomain-phase media

`code/Wegert_g_codomain_phase.R` is the checked-in source used by `.github/workflows/render-root-videos.yml` to generate:

- `rendered_images/wegert_g.png`
- `rendered_images/wegert_g_codomain_phase.mp4`
- `rendered_images/wegert_g_codomain_phase_preview.gif`

The root `wegert_g_codomain_phase.mp4` is a copy of the canonical rendered MP4.

The R source itself records that its core colour calculation was copied from the earlier Wegert R gist listed in `THIRD_PARTY.md`. The workflow makes the output derivation reproducible, but does not erase that earlier-source provenance or decide its copyright status.

## Copies, aliases and moves

Exact store copies, symlinks and path-only moves do not create independent provenance. When one canonical asset is unresolved, its byte-identical copies and aliases remain under the same unresolved provenance until the underlying source/holder/licensing authority is identified.
