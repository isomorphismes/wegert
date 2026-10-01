#!/usr/bin/env python3
"""
Sequence rational-function scenes and render their still frames.

This file deliberately does not evaluate the complex function, implement
Wegert colouring, or contain movie-encoding machinery. Each scene is sent to
the C offscreen renderer, which uses the same GLSL portrait renderer as the
Android app; the generic movie writer consumes the resulting RGB24 frames.
"""

import math
import os
import subprocess
import sys
from pathlib import Path

from movie import write_rgb24_movie

TAU = 2.0 * math.pi
FPS = 18
WIDTH = 400
HEIGHT = 400
FRAME_COUNT = 72

OUT = Path(sys.argv[1] if len(sys.argv) > 1 else ".")
OUT.mkdir(parents=True, exist_ok=True)

FRAME_RENDERER = Path(
    os.environ.get("WEGERT_FRAME_RENDERER", "/tmp/wegert-render-frames")
)
FRAGMENT_SHADER = Path(
    os.environ.get("WEGERT_FRAGMENT_SHADER", "/tmp/wegert.frag")
)


def orbit(center, radius, time, phase=0.0):
    angle = TAU * time + phase
    return center + radius * complex(math.cos(angle), math.sin(angle))


def factors_for_frame(kind, time):
    if kind == "two_poles":
        return (
            [(-0.25 + 0.10j, 1)],
            [
                (orbit(-0.85 - 0.15j, 0.42, time), 1),
                (orbit(0.85 + 0.15j, 0.42, time, math.pi), 1),
            ],
        )

    if kind == "three_roots":
        return (
            [
                (orbit(-0.95 + 0.20j, 0.35, time), 1),
                (orbit(0.75 + 0.55j, 0.40, time, 2.1), 1),
                (orbit(0.10 - 0.90j, 0.32, time, 4.2), 1),
            ],
            [],
        )

    if kind == "simple_repeated":
        return (
            [
                (orbit(-0.65 + 0.15j, 0.36, time), 1),
                (orbit(0.55 - 0.35j, 0.30, time, 2.4), 2),
            ],
            [],
        )

    if kind == "simple_double_poles":
        return (
            [(-0.15 + 0.25j, 1)],
            [
                (orbit(-0.75 + 0.00j, 0.32, time), 1),
                (orbit(0.65 + 0.05j, 0.30, time, math.pi), 2),
            ],
        )

    if kind == "two_two":
        return (
            [
                (orbit(-0.75 + 0.45j, 0.28, time), 1),
                (orbit(0.45 - 0.65j, 0.30, time, 1.6), 1),
            ],
            [
                (orbit(0.75 + 0.55j, 0.28, time, 3.2), 1),
                (orbit(-0.45 - 0.55j, 0.30, time, 4.8), 1),
            ],
        )

    if kind == "repeated_both":
        return (
            [
                (orbit(-0.75 + 0.35j, 0.30, time), 1),
                (orbit(0.05 - 0.55j, 0.30, time, 2.0), 2),
            ],
            [
                (orbit(0.75 + 0.35j, 0.30, time, 4.0), 1),
                (orbit(-0.05 + 0.75j, 0.25, time, 5.0), 2),
            ],
        )

    raise ValueError(f"unknown movie case: {kind}")


def expand_multiplicity(factors):
    expanded = []
    for position, multiplicity in factors:
        if multiplicity < 1:
            raise ValueError("factor multiplicity must be positive")
        expanded.extend([position] * multiplicity)
    return expanded


def scene_line(zeros_with_multiplicity, poles_with_multiplicity):
    zeros = expand_multiplicity(zeros_with_multiplicity)
    poles = expand_multiplicity(poles_with_multiplicity)

    fields = [str(len(zeros)), str(len(poles))]
    for position in zeros:
        fields.extend((repr(position.real), repr(position.imag)))
    for position in poles:
        fields.extend((repr(position.real), repr(position.imag)))
    return " ".join(fields)


def scene_stream(kind):
    lines = []
    for frame in range(FRAME_COUNT):
        time = frame / FRAME_COUNT
        zeros, poles = factors_for_frame(kind, time)
        lines.append(scene_line(zeros, poles))
    return "\n".join(lines) + "\n"


def render_rgb_frames(kind):
    if not FRAME_RENDERER.is_file():
        raise SystemExit(
            f"missing C frame renderer: {FRAME_RENDERER}\n"
            "Build code/wegert_render_frames.c first."
        )
    if not FRAGMENT_SHADER.is_file():
        raise SystemExit(
            f"missing assembled fragment shader: {FRAGMENT_SHADER}\n"
            "Run _/build/android-direct/assemble-shader.sh first."
        )

    environment = os.environ.copy()
    environment.setdefault("EGL_PLATFORM", "surfaceless")

    completed = subprocess.run(
        [
            str(FRAME_RENDERER),
            str(FRAGMENT_SHADER),
            str(WIDTH),
            str(HEIGHT),
            "0",
            "0",
            "2.35",
        ],
        input=scene_stream(kind).encode("ascii"),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=environment,
        check=False,
    )
    if completed.returncode != 0:
        raise SystemExit(
            f"C frame renderer failed for {kind}:\n"
            + completed.stderr.decode("utf-8", errors="replace")
        )

    frame_size = WIDTH * HEIGHT * 3
    expected = FRAME_COUNT * frame_size
    if len(completed.stdout) != expected:
        raise SystemExit(
            f"C frame renderer produced {len(completed.stdout)} bytes for "
            f"{kind}; expected {expected}"
        )

    for start in range(0, expected, frame_size):
        yield completed.stdout[start : start + frame_size]


CASES = [
    ("two_moving_simple_poles.mp4", "two_poles"),
    ("three_moving_simple_roots.mp4", "three_roots"),
    ("moving_simple_and_repeated_roots.mp4", "simple_repeated"),
    ("moving_simple_and_double_poles.mp4", "simple_double_poles"),
    ("moving_two_simple_zeros_and_two_simple_poles.mp4", "two_two"),
    ("moving_repeated_zeros_and_poles.mp4", "repeated_both"),
]

for output_name, case in CASES:
    output = write_rgb24_movie(
        OUT / output_name,
        render_rgb_frames(case),
        WIDTH,
        HEIGHT,
        FPS,
    )
    print(output)
