# Icon candidates

`jones-trefoil.png` is rendered by the running Android Wegert renderer from the
right-handed trefoil Jones polynomial convention

`V(z) = z + z^3 - z^4`.

The capture build represents the polynomial by its four zeros and applies the
leading `-1` as a phase rotation by pi.  Colour still comes from the canonical
`code/wegert_color.glsl` path; the icon does not have a separate palette.

`.github/workflows/jones-trefoil-icon.yml` builds and launches the capture APK,
records a 512x512 runtime screenshot, removes only the Android navigation-button
strip by cropping `460x460+26+0`, resizes that crop to 512x512, and commits the
candidate here.
