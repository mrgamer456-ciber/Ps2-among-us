#!/usr/bin/env python3
"""Recreate the exact 64x64 procedural texture pixels used by tyrarooms/src/main.cpp.

This is a preview/export tool only: it does not modify the PS2 game or its assets.
Writes uncompressed 32-bit BMP files that can be opened on a phone or computer.
"""
import os
import struct

SIZE = 64
WALL_SIZE = 128
OUT = "texture-previews"

def noise(x, y, seed):
    n = (x * 374761393 + y * 668265263 + seed * 1442695041) & 0xFFFFFFFF
    n = ((n ^ (n >> 13)) * 1274126177) & 0xFFFFFFFF
    return (n ^ (n >> 16)) & 31

def cdiv(value, divisor):
    return value // divisor if value >= 0 else -((-value) // divisor)

def wallpaper_hash(x, y, seed):
    n = (x * 374761393 + y * 668265263 + seed * 2147483647) & 0xFFFFFFFF
    n = ((n ^ (n >> 13)) * 1274126177) & 0xFFFFFFFF
    return (n ^ (n >> 16)) & 0xFFFFFFFF

def wallpaper_noise(x, y, period, seed):
    import math
    x0, y0 = math.floor(x), math.floor(y)
    fx, fy = x - x0, y - y0
    fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
    def value(ix, iy):
        return (wallpaper_hash(ix % period, iy % period, seed) & 0xFFFF) / 65535.0
    a, b = value(x0, y0), value(x0 + 1, y0)
    c, d = value(x0, y0 + 1), value(x0 + 1, y0 + 1)
    return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy

def pixel(kind, x, y, size=SIZE):
    n = noise(x, y, kind + 7)
    if kind == 0:  # Exact Claude-era v0.3 Backrooms wallpaper
        cell = size // 4
        base_r, base_g, base_b = 205.0, 186.0, 84.0
        k = 1.0
        sx = x % cell
        if sx < 4:
            k *= 0.86
        elif sx == 4:
            k *= 1.05
        row, oy = y // cell, y % cell
        ox = (sx + (row & 1) * (cell // 2)) % cell
        d = abs(ox - cell // 2) + abs(oy - cell // 2)
        if d < 7:
            k *= 0.90
        elif d == 7:
            k *= 0.96
        stain = wallpaper_noise(x * 8.0 / size, y * 8.0 / size, 8, 3)
        k *= 0.86 + 0.14 * stain
        grain = ((wallpaper_hash(x, y, 7) & 255) / 255.0 - 0.5) * 10.0
        if y >= size - 10:
            base_r, base_g, base_b = 96.0, 74.0, 42.0
            k = 1.2 if y == size - 10 else 0.92
        elif y < 4:
            k *= 0.80
        r = int(max(0, min(255, base_r * k + grain)))
        g = int(max(0, min(255, base_g * k + grain)))
        b = int(max(0, min(255, base_b * k + grain))
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
    size = WALL_SIZE if kind == 0 else SIZE
    row_bytes = size * 4
    pixel_bytes = row_bytes * size
    header = b"BM" + struct.pack("<IHHI", 14 + 40 + pixel_bytes, 0, 0, 54)
    dib = struct.pack("<IiiHHIIiiII", 40, size, size, 1, 32, 0,
                      pixel_bytes, 2835, 2835, 0, 0)
    with open(path, "wb") as f:
        f.write(header)
        f.write(dib)
        for y in range(size - 1, -1, -1):
            for x in range(size):
                r, g, b = pixel(kind, x, y, size)
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
        size = WALL_SIZE if kind == 0 else SIZE
        print(f"Generated {path} ({size}x{size}, 32-bit BMP)")
    print("Done. Pixel formulas mirror makeTexture() in tyrarooms/src/main.cpp.")

if __name__ == "__main__":
    main()
