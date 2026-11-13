from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[1]
source = root / "assets/main.tga"
out = root / "src/background.h"

if not source.exists():
    raise SystemExit("assets/main.tga missing")

im = Image.open(source).convert("RGB")
if im.size != (512, 512):
    raise SystemExit(f"Expected original 512x512 TGA, got {im.width}x{im.height}")

pixels = []
for y in range(512):
    for x in range(512):
        r, g, b = im.getpixel((x, y))
        pixels.append(f"0x{(0x80000000 | (b << 16) | (g << 8) | r):08X}u")

rows = ["    " + ", ".join(pixels[i:i + 8]) for i in range(0, len(pixels), 8)]
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(
    "#pragma once\n#include <stdint.h>\n"
    "static const uint32_t cleanMenuPixels[512*512] = {\n"
    + ",\n".join(rows)
    + "\n};\n"
)
print("Generated original 512x512 menu texture for direct C++ loading")
