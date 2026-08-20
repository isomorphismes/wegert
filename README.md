# Wegert

Interactive Wegert phase portraits of complex rational functions, designed first for a touch tablet.

## Tablet prototype

The current prototype is on `tablet-touch-game`.

A touch adds a factor to

`f(z) = product(z - zero) / product(z - pole)`

so the portrait changes immediately:

- `O zero` mode places a zero.
- `X infinity` mode places a pole, a point sent to infinity.
- `move` mode drags the visible domain over the complex plane.
- `zoom out` enlarges the visible domain.
- `zoom in` shrinks the visible domain.
- `center 0` returns the camera center to the ordinary complex zero.
- `undo` removes the last placed point.
- `clear` removes all points.
- `pause` freezes or resumes the color animation.

The complex plane uses equal horizontal and vertical scale. The initial view is centered at `0 + 0i` with vertical half-extent `2.5`; the horizontal extent follows the tablet aspect ratio. This rectangular view is only a camera onto the complex plane. The intended larger model is the Riemann sphere, so the camera state is kept separate from the zeros and poles.

## Color mapping

The palette is intentionally perceptual HCL/polar CIELUV, matching R's `hcl()` family rather than HSV or HSL:

- hue: `Arg(f(z))` in degrees, modulo 360;
- chroma: exactly `45`;
- logarithmic modulus band: `frac(log10(abs(f(z))))`;
- hue band: `frac(hue / 100)`;
- lightness: `66 + 4*frac(log10(abs(f(z)))) + 3*frac(hue / 100)`;
- final color: polar CIELUV/HCL converted to sRGB.

Thus each decade of `abs(f(z))` repeats the same lightness structure while argument continues to determine hue.

The continuous six-second animation is codomain phase rotation,

`f_t(z) = exp(i t) f(z)`,

so zeros, poles, and modulus structure remain fixed while the phase colors cycle.

## Run

Open the repository in Godot 4.7.1 and run the main scene.
