#!/usr/bin/env python3
"""Generate tiny RGB/RGBA PNG fixtures using only the Python standard library."""

from __future__ import annotations

import binascii
import pathlib
import struct
import sys
import zlib


def chunk(kind: bytes, data: bytes) -> bytes:
    body = kind + data
    return struct.pack(">I", len(data)) + body + struct.pack(">I", binascii.crc32(body))


def write_png(
    path: pathlib.Path,
    width: int,
    height: int,
    channels: int,
    bit_depth: int,
    values: list[int],
) -> None:
    color_type = {3: 2, 4: 6}[channels]
    expected = width * height * channels
    if len(values) != expected:
        raise ValueError(f"expected {expected} values, got {len(values)}")

    rows: list[bytes] = []
    stride = width * channels
    for y in range(height):
        samples = values[y * stride : (y + 1) * stride]
        if bit_depth == 8:
            row = bytes(samples)
        elif bit_depth == 16:
            row = struct.pack(">" + "H" * len(samples), *samples)
        else:
            raise ValueError("unsupported fixture depth")
        rows.append(b"\x00" + row)  # PNG filter type 0

    ihdr = struct.pack(">IIBBBBB", width, height, bit_depth, color_type, 0, 0, 0)
    data = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", ihdr)
        + chunk(b"IDAT", zlib.compress(b"".join(rows)))
        + chunk(b"IEND", b"")
    )
    path.write_bytes(data)


def main() -> None:
    output = pathlib.Path(sys.argv[1])
    output.mkdir(parents=True, exist_ok=True)

    rgb8_a = [
        0, 128, 255,
        255, 64, 0,
        10, 20, 30,
        1, 2, 3,
    ]
    rgb8_b = rgb8_a.copy()
    rgb8_b[1] = 129
    rgba8 = [
        0, 128, 255, 255,
        255, 64, 0, 128,
        10, 20, 30, 0,
        1, 2, 3, 255,
    ]
    rgb16 = [
        0, 32768, 65535,
        65535, 16384, 0,
        1000, 2000, 3000,
        1, 2, 3,
    ]
    rgba16 = [
        0, 32768, 65535, 65535,
        65535, 16384, 0, 32768,
        1000, 2000, 3000, 0,
        1, 2, 3, 65535,
    ]

    write_png(output / "rgb8_a.png", 2, 2, 3, 8, rgb8_a)
    write_png(output / "rgb8_b.png", 2, 2, 3, 8, rgb8_b)
    write_png(output / "rgba8.png", 2, 2, 4, 8, rgba8)
    write_png(output / "rgb16.png", 2, 2, 3, 16, rgb16)
    write_png(output / "rgba16.png", 2, 2, 4, 16, rgba16)


if __name__ == "__main__":
    main()
