from pathlib import Path
from PIL import Image, ImageOps
root = Path(__file__).resolve().parents[1]
source = root / "assets/main.tga"
out = root / "src/background.h"
if not source.exists():
    raise SystemExit("assets/main.tga missing")
im = ImageOps.fit(Image.open(source).convert("RGB"), (512, 384), method=Image.Resampling.LANCZOS)
vals=[]
for y in range(512):
    for x in range(512):
        if y < 384:
            r,g,b=im.getpixel((x,y))
            vals.append(f"0x80000000u | ((uint32_t){b}u << 16) | ((uint32_t){g}u << 8) | (uint32_t){r}u")
        else: vals.append("0x80000000u")
rows=["    "+", ".join(vals[i:i+8]) for i in range(0,len(vals),8)]
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text("#pragma once\n#include <stdint.h>\nstatic const uint32_t cleanMenuPixels[512*512] = {\n"+",\n".join(rows)+"\n};\n")
print("Fresh menu background header generated")
