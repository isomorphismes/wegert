# Wegert

Interactive Wegert phase portraits of complex rational functions, designed first for a touch tablet.

## Tablet prototype

The current prototype is on `tablet-touch-game`.

A touch adds a factor to

`f(z) = product(z - zero) / product(z - pole)`

so the portrait changes immediately:

- `O zero` mode places a zero.
- `X infinity` mode places a pole, a point sent to infinity.
- `undo` removes the last placed point.
- `clear` removes all points.
- `pause` freezes or resumes the color animation.

The complex plane uses equal horizontal and vertical scale. The visible vertical range is `[-2.5, 2.5]`; the horizontal range expands with the tablet aspect ratio.

## Color mapping

This ports the old `Wegert.R` palette rather than replacing it with HSV:

- hue: `Arg(f(z))` in degrees, modulo 360;
- chroma: 45;
- `l(x) = frac(x / 100)`;
- modulus term: `l(abs(f(z)))`;
- lightness: `66 + 4*l(abs(f(z))) + 3*l(hue)`;
- final color: polar CIELUV/HCL converted to sRGB.

The continuous six-second animation is codomain phase rotation,

`f_t(z) = exp(i t) f(z)`,

so zeros, poles, and modulus structure remain fixed while the phase colors cycle.

## Run

Open the repository in Godot 4.7.1 and run the main scene.
