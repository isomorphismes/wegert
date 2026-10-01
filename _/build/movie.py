#!/usr/bin/env python3
"""Encode already-rendered RGB24 frames as a movie.

This module deliberately knows nothing about Wegert scenes or rendering.  A
caller chooses how state varies over time and how each state becomes a still
frame; this file only consumes the resulting ordered RGB24 frames.
"""

import subprocess
from pathlib import Path


def write_rgb24_movie(output, frames, width, height, fps):
    """Write an iterable of complete RGB24 frames to an H.264 MP4."""

    output = Path(output)
    frame_size = width * height * 3

    command = [
        "ffmpeg",
        "-y",
        "-loglevel",
        "error",
        "-f",
        "rawvideo",
        "-pix_fmt",
        "rgb24",
        "-s",
        f"{width}x{height}",
        "-r",
        str(fps),
        "-i",
        "-",
        "-an",
        "-c:v",
        "libx264",
        "-preset",
        "slow",
        "-crf",
        "30",
        "-pix_fmt",
        "yuv420p",
        "-movflags",
        "+faststart",
        str(output),
    ]

    process = subprocess.Popen(command, stdin=subprocess.PIPE)
    if process.stdin is None:
        process.terminate()
        process.wait()
        raise RuntimeError("ffmpeg stdin was not created")

    try:
        for frame_number, frame in enumerate(frames):
            if len(frame) != frame_size:
                raise ValueError(
                    f"frame {frame_number} has {len(frame)} bytes; "
                    f"expected {frame_size}"
                )
            process.stdin.write(frame)
        process.stdin.close()
        returncode = process.wait()
    except BaseException:
        if not process.stdin.closed:
            process.stdin.close()
        if process.poll() is None:
            process.terminate()
        process.wait()
        raise

    if returncode != 0:
        raise RuntimeError(f"ffmpeg failed with status {returncode}")

    return output
