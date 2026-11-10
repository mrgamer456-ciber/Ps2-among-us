#!/usr/bin/env python3
"""Recreate the exact 64x64 procedural texture pixels used by tyrarooms/src/main.cpp.

This is a preview/export tool only: it does not modify the PS2 game or its assets.
Writes uncompressed 32-bit BMP files that can be opened on a phone or computer.
"""
import os
import struct

SIZE = 64
OUT = "texture-previews"

def noise(x, y, seed):
    n = (x * 374761393 + y * 668265667 + seed * 1442695041) & 0xFFFFFFFF
    n = ((n ^ (n >> 13)) * 1274126177) & 0xFFFFFFFF
    return (n ^ (n >> 16)) & 31

def pixel(kind, x, y):
    n = noise(x, y, kind + 7)
    if kind == 0:  # wall
        seam = 9 if x % 32 == 0 else 0
        r, g, b = 190 + n // 3 - seam, 174 + n // 3 - seam, 91 + n // 4 - seam
        if y % 32 == 0:
            r -= 4
            g -= 4
    elif kind == 1:  # carpet
        fleck = 9 if n > 24 else 0
        r, g, b = 65 + n // 2 + fleck, 61 + n // 2 + fleck, 43 + n // 3
        if (x * 3 + y * 5) % 29 == 0:
            r -= 8
            g -= 8
            b -= 5
    elif kind == 2:  # ceiling
        r, g, b = 174 + n // 3, 169 + n // 3, 148 + n // 3
        if x % 32 < 2 or y % 32 < 2:
            r -= 5
            g -= 5
            b -= 4
    else:  # character suit
        seam = 10 if (x % 32 < 2 or y % 32 < 2) else 0
        r, g, b = 205 + n // 4 - seam, 174 + n // 4 - seam, 53 + n // 5 - seam // 2
        if 42 < x < 59 and 36 < y < 57:
            r, g, b = 190 + n // 4, 158 + n // 4, 43 + n // 5
    return max(0, min(255, r)), max(0, min(255, g)), max(0, min(255, b))

def write_bmp(path, kind):
    # 32-bit BI_RGB BMP; rows are stored bottom-up, pixels in BGRA byte order.
    row_bytes = SIZE * 4
    pixel_bytes = row_bytes * SIZE
    header = b"BM" + struct.pack("<IHHI", 14 + 40 + pixel_bytes, 0, 0, 54)
    dib = struct.pack("<IiiHHIIiiII", 40, SIZE, SIZE, 1, 32, 0,
                      pixel_bytes, 2835, 2835, 0, 0)
    with open(path, "wb") as f:
        f.write(header)
        f.write(dib)
        for y in range(SIZE - 1, -1, -1):
            for x in range(SIZE):
                r, g, b = pixel(kind, x, y)
                f.write(bytes((b, g, r, 255)))

def main():
    os.makedirs(OUT, exist_ok=True)
    textures = [
        ("wall.bmp", 0),
        ("floor_carpet.bmp", 1),
        ("ceiling.bmp", 2),
        ("character_suit.bmp", 3),
    ]
    for filename, kind in textures:
        path = os.path.join(OUT, filename)
        write_bmp(path, kind)
        print(f"Generated {path} ({SIZE}x{SIZE}, 32-bit BMP)")
    print("Done. Pixel formulas mirror makeTexture() in tyrarooms/src/main.cpp.")

if __name__ == "__main__":
    main()
