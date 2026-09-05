#!/usr/bin/env python3
"""Render palette-dance MP4s using the Wegert R gist's domain-colour rule.

Reference rule (copied semantically from Wegert_g_codomain_phase.R):
  hue = Arg(f(z)) in degrees mod 360
  light = 66 + 4*fract(|f(z)|/100) + 3*fract(hue/100)
  colour = HCL(hue, chroma=45, lightness=light)
Animation multiplies the codomain by exp(i*theta); the z-plane is fixed.
"""
from __future__ import annotations

import math
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.animation import FFMpegWriter
import numpy as np

OUT = Path(sys.argv[1] if len(sys.argv) > 1 else "rendered_images")
OUT.mkdir(parents=True, exist_ok=True)
TAU = 2.0 * math.pi


def fract(x: np.ndarray) -> np.ndarray:
    return x - np.floor(x)


def srgb_component(linear: np.ndarray) -> np.ndarray:
    v = np.maximum(linear, 0.0)
    return np.where(v <= 0.0031308, 12.92 * v, 1.055 * np.power(v, 1.0 / 2.4) - 0.055)


def hcl_to_srgb(hue_degrees: np.ndarray, chroma: float, lightness: np.ndarray) -> np.ndarray:
    """Polar CIE L*u*v* HCL -> clipped sRGB, D65.

    This follows the conversion already used by the wegert repository's
    renderer-independent colour core, matching R hcl() closely for this gamut.
    """
    hue = np.deg2rad(hue_degrees)
    u_star = chroma * np.cos(hue)
    v_star = chroma * np.sin(hue)

    white_u_prime = 0.19783982482140777
    white_v_prime = 0.46833630293240974

    y = np.where(
        lightness > 8.0,
        np.power((lightness + 16.0) / 116.0, 3.0),
        lightness / 903.2962962962963,
    )
    u_prime = u_star / (13.0 * lightness) + white_u_prime
    v_prime = v_star / (13.0 * lightness) + white_v_prime

    x = (9.0 * y * u_prime) / (4.0 * v_prime)
    z = y * (12.0 - 3.0 * u_prime - 20.0 * v_prime) / (4.0 * v_prime)

    linear_r = 3.2404542 * x - 1.5371385 * y - 0.4985314 * z
    linear_g = -0.9692660 * x + 1.8760108 * y + 0.0415560 * z
    linear_b = 0.0556434 * x - 0.2040259 * y + 1.0572252 * z

    return np.clip(
        np.stack(
            [srgb_component(linear_r), srgb_component(linear_g), srgb_component(linear_b)],
            axis=-1,
        ),
        0.0,
        1.0,
    )


def wegert_gist_colour(base_phase: np.ndarray, modulus: np.ndarray, theta: float) -> np.ndarray:
    # Multiplication by exp(i*theta) changes phase but not modulus.
    hue = np.mod(np.rad2deg(base_phase + theta), 360.0)
    modulus_band = fract(modulus / 100.0)
    hue_band = fract(hue / 100.0)
    lightness = 66.0 + 4.0 * modulus_band + 3.0 * hue_band
    return hcl_to_srgb(hue, 45.0, lightness)


def setup_grid(limit: float, resolution: int) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    x = np.linspace(-limit, limit, resolution)
    y = np.linspace(-limit, limit, resolution)
    xx, yy = np.meshgrid(x, y)
    return xx, yy, xx + 1j * yy


def render_movie(
    filename: str,
    title: str,
    func,
    zeros: list[complex],
    poles: list[complex],
    *,
    limit: float = 5.6,
    resolution: int = 620,
    frames: int = 120,
    fps: int = 20,
) -> None:
    _, _, z = setup_grid(limit, resolution)
    values = func(z)
    base_phase = np.angle(values)
    modulus = np.abs(values)

    # Singular points may produce inf/nan exactly at a pixel. Give them a
    # harmless finite carrier; the marker drawn afterward shows the pole/zero.
    bad = ~np.isfinite(base_phase) | ~np.isfinite(modulus)
    base_phase = np.where(bad, 0.0, base_phase)
    modulus = np.where(bad, 0.0, modulus)

    fig = plt.figure(figsize=(6, 6), dpi=120, facecolor="#f5f2eb")
    ax = fig.add_axes([0.115, 0.09, 0.80, 0.80], facecolor="#f5f2eb")
    rgb0 = wegert_gist_colour(base_phase, modulus, 0.0)
    image = ax.imshow(
        rgb0,
        origin="lower",
        extent=(-limit, limit, -limit, limit),
        interpolation="bilinear",
        aspect="equal",
    )
    ax.axhline(0.0, color=(1, 1, 1, 0.68), linewidth=0.7)
    ax.axvline(0.0, color=(1, 1, 1, 0.68), linewidth=0.7)
    if zeros:
        ax.scatter(
            [p.real for p in zeros], [p.imag for p in zeros],
            s=34, marker="o", facecolors="#f5f2eb", edgecolors="#181818",
            linewidths=1.2, zorder=3,
        )
    if poles:
        ax.scatter(
            [p.real for p in poles], [p.imag for p in poles],
            s=42, marker="x", c="#181818", linewidths=1.5, zorder=3,
        )
    ax.set_xlabel("Re z")
    ax.set_ylabel("Im z")
    ax.set_xlim(-limit, limit)
    ax.set_ylim(-limit, limit)
    fig.suptitle(title, y=0.965, fontsize=12)
    phase_text = fig.text(0.5, 0.925, r"$\theta = 0.00\pi$", ha="center", va="center", fontsize=11)

    writer = FFMpegWriter(
        fps=fps,
        codec="libx264",
        bitrate=-1,
        extra_args=["-crf", "18", "-preset", "slow", "-pix_fmt", "yuv420p", "-movflags", "+faststart"],
    )
    out_path = OUT / filename
    with writer.saving(fig, str(out_path), dpi=120):
        for frame in range(frames):
            theta = TAU * frame / frames
            image.set_data(wegert_gist_colour(base_phase, modulus, theta))
            phase_text.set_text(rf"$\theta = {theta / math.pi:.2f}\pi$")
            writer.grab_frame(facecolor=fig.get_facecolor())
    plt.close(fig)


MOVIES = [
    dict(
        filename="wegert_gist_palette_dance_cubic_spread.mp4",
        title=r"Wegert palette dance: $(z+2.6)(z-0.7)(z-2.9)$",
        func=lambda z: (z + 2.6) * (z - 0.7) * (z - 2.9),
        zeros=[-2.6 + 0j, 0.7 + 0j, 2.9 + 0j],
        poles=[],
    ),
    dict(
        filename="wegert_gist_palette_dance_zero_pole_cross.mp4",
        title=r"Wegert palette dance: scaled two-zero / two-pole map",
        func=lambda z: 300.0 * ((z - (-1.8 + 1.2j)) * (z - (1.6 - 0.9j)))
        / ((z - (-1.2 - 1.7j)) * (z - (1.8 + 1.4j))),
        zeros=[-1.8 + 1.2j, 1.6 - 0.9j],
        poles=[-1.2 - 1.7j, 1.8 + 1.4j],
    ),
    dict(
        filename="wegert_gist_palette_dance_fifth_roots.mp4",
        title=r"Wegert palette dance: $z^5-1$",
        func=lambda z: z**5 - 1.0,
        zeros=[np.exp(2j * math.pi * k / 5.0) for k in range(5)],
        poles=[],
        limit=3.2,
    ),
]


if __name__ == "__main__":
    for spec in MOVIES:
        print("rendering", spec["filename"], flush=True)
        render_movie(**spec)
