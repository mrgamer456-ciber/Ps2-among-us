#!/usr/bin/env python3
from PIL import Image
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = root / "tyrarooms/src/Skin maior.png"
out_header = root / "tyrarooms/src/minecraft_skin.h"
out_preview = root / "texture-previews/minecraft_skin_preview.png"

img = Image.open(source).convert("RGBA")
if img.width != img.height:
    raise SystemExit(f"Skin must be square; got {img.size}")
# Minecraft skins are pixel art. Nearest-neighbour preserves every hard pixel edge.
skin = img.resize((64, 64), Image.Resampling.NEAREST).convert("RGB")
pixels = []
for y in range(64):
    for x in range(64):
        r, g, b = skin.getpixel((x, y))
        pixels.append(f"0x{r:02X}{g:02X}{b:02X}FFu")
rows = []
for i in range(0, len(pixels), 8):
    rows.append("    " + ", ".join(pixels[i:i+8]))
header = """#pragma once
#include <stdint.h>
// Generated from tyrarooms/src/Skin maior.png; 64x64 RGB skin atlas.
static const uint32_t minecraftSkinPixels[64 * 64] = {
""" + ",\n".join(rows) + """
};
"""
out_header.parent.mkdir(parents=True, exist_ok=True)
out_header.write_text(header)
out_preview.parent.mkdir(parents=True, exist_ok=True)
skin.resize((512, 512), Image.Resampling.NEAREST).save(out_preview)
print(f"Generated {out_header} and {out_preview} from {source}; source={img.size}")
