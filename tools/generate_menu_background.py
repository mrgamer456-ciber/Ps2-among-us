#!/usr/bin/env python3
"""Convert the user-supplied menu BMP into a PS2-ready 512x512 CT32 header."""
from pathlib import Path
from PIL import Image, ImageOps

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "tyrarooms/assets/menu/menu_bg.bmp"
OUTPUT = ROOT / "tyrarooms/src/menu_background.h"

if not SOURCE.is_file():
    raise SystemExit(f"Menu background not found: {SOURCE}")

# Preserve the original BMP. Crop only the generated in-memory copy to 4:3,
# then downsample to 512x384 and pad the 512x512 GS texture with black.
image = Image.open(SOURCE).convert("RGB")
image = ImageOps.fit(image, (512, 384), method=Image.Resampling.LANCZOS,
                     centering=(0.5, 0.5))
pixels = []
for y in range(512):
    for x in range(512):
        if y < 384:
            r, g, b = image.getpixel((x, y))
            pixels.append(f"0x80000000u | ((uint32_t){b}u << 16) | "
                          f"((uint32_t){g}u << 8) | (uint32_t){r}u")
        else:
            pixels.append("0x80000000u")

rows = []
for i in range(0, len(pixels), 8):
    rows.append("    " + ", ".join(pixels[i:i + 8]))
header = (
    "#pragma once\n#include <stdint.h>\n"
    "// Generated from assets/menu/menu_bg.bmp; 512x384 visible image in a 512x512 texture.\n"
    "static const uint32_t menuBackgroundPixels[512 * 512] = {\n"
    + ",\n".join(rows) + "\n};\n"
)
OUTPUT.parent.mkdir(parents=True, exist_ok=True)
OUTPUT.write_text(header)
print(f"Generated {OUTPUT} from {SOURCE}; source={Image.open(SOURCE).size}, output=512x384")
