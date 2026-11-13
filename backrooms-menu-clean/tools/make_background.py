from pathlib import Path
from PIL import Image, ImageOps

root = Path(__file__).resolve().parents[1]
source = root / "assets/main.tga"
out = root / "src/background.h"

if not source.exists():
    raise SystemExit("assets/main.tga missing")

# Prepare a power-of-two texture. The PS2 program reads this array directly.
im = ImageOps.fit(
    Image.open(source).convert("RGB"),
    (256, 256),
    method=Image.Resampling.LANCZOS
)

pixels = []
for y in range(256):
    for x in range(256):
        r, g, b = im.getpixel((x, y))
        # GS CT32 word layout used by the C++ texture upload.
        pixels.append(f"0x{(0x80000000 | (b << 16) | (g << 8) | r):08X}u")

rows = ["    " + ", ".join(pixels[i:i + 8]) for i in range(0, len(pixels), 8)]
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(
    "#pragma once\n#include <stdint.h>\n"
    "static const uint32_t cleanMenuPixels[256*256] = {\n"
    + ",\n".join(rows)
    + "\n};\n"
)
print("Generated 256x256 menu texture for direct C++ loading")
