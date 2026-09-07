# Bundled artwork and media provenance

This file records provenance facts supported by the checked-in files and repository history. It deliberately does **not** infer copyright ownership from commit authorship. `LICENSE` applies only to material for which the repository's contributors have authority to grant GPL-3.0-or-later.

## Launcher icon family

Canonical/current copies:

- `rendered_images/wegert-icon-512.png`
- `_/build/artwork/wegert-icon-512.png`
- `_/build/fastlane/metadata/android/en-US/images/icon.png`
- root `wegert-icon-512.png` is a symlink to the canonical rendered image

The first three files above are the same Git blob (`7eb3b96f1ed5c82fd3891ead080c838aca08aedd`), so the Fastlane/F-Droid store icon is an exact copy, not an independently sourced asset.

Commit `a9a28fd0ea5277406ba7d5c4807eec29d28dbb1a` (`Add a phase-portrait Android launcher icon`) introduced the 512px icon together with the Android launcher family under:

- `_/build/app/src/main/res/drawable-{mdpi,hdpi,xhdpi,xxhdpi,xxxhdpi}/ic_launcher_foreground.png`
- `_/build/app/src/main/res/mipmap-{mdpi,hdpi,xhdpi,xxhdpi,xxxhdpi}/ic_launcher.png`
- `_/build/app/src/main/res/mipmap-{mdpi,hdpi,xhdpi,xxhdpi,xxxhdpi}/ic_launcher_round.png`
- `_/build/app/src/main/res/mipmap-anydpi-v26/ic_launcher.xml`
- `_/build/app/src/main/res/mipmap-anydpi-v26/ic_launcher_round.xml`
- `_/build/app/src/main/res/values/ic_launcher_background.xml`

No checked-in source file or transformation recipe records how the PNG density/foreground/round variants were produced. Repository history therefore supports treating these as one introduced icon family, but does not establish the original source, creator, copyright holder, or permission to license the family.

**Copyright/license status:** unresolved until the copyright holder or authorized contributor identifies the source/holder and confirms licensing authority. If that authority is confirmed, the existing repository GPL-3.0-or-later grant applies; this file does not make that confirmation on the holder's behalf.

## Store screenshot

Current store screenshot:

- `_/build/fastlane/metadata/android/en-US/images/phoneScreenshots/1.png`

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
