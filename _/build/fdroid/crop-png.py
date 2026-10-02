#!/usr/bin/env python3
"""Crop an 8-bit RGB/RGBA non-interlaced PNG without image-library dependencies."""

from __future__ import annotations

import argparse
import binascii
import struct
import zlib
from pathlib import Path


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def chunks(data: bytes):
    offset = len(PNG_SIGNATURE)
    while offset < len(data):
        length = struct.unpack(">I", data[offset:offset + 4])[0]
        kind = data[offset + 4:offset + 8]
        payload = data[offset + 8:offset + 8 + length]
        yield kind, payload
        offset += 12 + length


def paeth(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def unfilter(scanlines: bytes, width: int, height: int, bpp: int) -> list[bytearray]:
    row_bytes = width * bpp
    expected = height * (row_bytes + 1)
    if len(scanlines) != expected:
        raise SystemExit(f"unexpected decompressed PNG size: {len(scanlines)} != {expected}")

    rows: list[bytearray] = []
    offset = 0
    previous = bytearray(row_bytes)
    for _ in range(height):
        filter_type = scanlines[offset]
        raw = scanlines[offset + 1:offset + 1 + row_bytes]
        offset += row_bytes + 1
        current = bytearray(row_bytes)

        for i, value in enumerate(raw):
            left = current[i - bpp] if i >= bpp else 0
            up = previous[i]
            upper_left = previous[i - bpp] if i >= bpp else 0

            if filter_type == 0:
                decoded = value
            elif filter_type == 1:
                decoded = value + left
            elif filter_type == 2:
                decoded = value + up
            elif filter_type == 3:
                decoded = value + ((left + up) // 2)
            elif filter_type == 4:
                decoded = value + paeth(left, up, upper_left)
            else:
                raise SystemExit(f"unsupported PNG filter: {filter_type}")
            current[i] = decoded & 0xFF

        rows.append(current)
        previous = current

    return rows


def make_chunk(kind: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + kind
        + payload
        + struct.pack(">I", binascii.crc32(kind + payload) & 0xFFFFFFFF)
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("x", type=int)
    parser.add_argument("y", type=int)
    parser.add_argument("width", type=int)
    parser.add_argument("height", type=int)
    args = parser.parse_args()

    data = args.source.read_bytes()
    if not data.startswith(PNG_SIGNATURE):
        raise SystemExit("source is not a PNG")

    ihdr = None
    idat_parts: list[bytes] = []
    preserved: list[tuple[bytes, bytes]] = []
    for kind, payload in chunks(data):
        if kind == b"IHDR":
            ihdr = payload
        elif kind == b"IDAT":
            idat_parts.append(payload)
        elif kind in {b"sRGB", b"gAMA", b"cHRM", b"iCCP", b"pHYs"}:
            preserved.append((kind, payload))

    if ihdr is None or len(ihdr) != 13:
        raise SystemExit("missing or invalid IHDR")

    width, height, bit_depth, color_type, compression, filtering, interlace = struct.unpack(
        ">IIBBBBB", ihdr
    )
    if bit_depth != 8 or color_type not in {2, 6}:
        raise SystemExit(f"expected 8-bit RGB/RGBA PNG, got depth={bit_depth} type={color_type}")
    if compression != 0 or filtering != 0 or interlace != 0:
        raise SystemExit("unsupported PNG compression/filter/interlace mode")

    bpp = 3 if color_type == 2 else 4
    rows = unfilter(zlib.decompress(b"".join(idat_parts)), width, height, bpp)

    if (
        args.x < 0
        or args.y < 0
        or args.width <= 0
        or args.height <= 0
        or args.x + args.width > width
        or args.y + args.height > height
    ):
        raise SystemExit(
            f"crop {args.width}x{args.height}+{args.x}+{args.y} outside {width}x{height}"
        )

    cropped = bytearray()
    left = args.x * bpp
    right = (args.x + args.width) * bpp
    for row in rows[args.y:args.y + args.height]:
        cropped.append(0)
        cropped.extend(row[left:right])

    output_ihdr = struct.pack(
        ">IIBBBBB",
        args.width,
        args.height,
        bit_depth,
        color_type,
        compression,
        filtering,
        interlace,
    )
    result = bytearray(PNG_SIGNATURE)
    result.extend(make_chunk(b"IHDR", output_ihdr))
    for kind, payload in preserved:
        result.extend(make_chunk(kind, payload))
    result.extend(make_chunk(b"IDAT", zlib.compress(bytes(cropped), level=9)))
    result.extend(make_chunk(b"IEND", b""))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(result)


if __name__ == "__main__":
    main()
