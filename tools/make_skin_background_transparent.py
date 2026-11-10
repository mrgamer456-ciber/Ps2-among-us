from PIL import Image
from collections import deque
from pathlib import Path

src = Path("tyrarooms/src/Skin maior.png")
out = Path("texture-previews/Skin maior transparente.png")
out.parent.mkdir(parents=True, exist_ok=True)

im = Image.open(src).convert("RGBA")
w, h = im.size
px = im.load()
seen = bytearray(w * h)
q = deque()

def is_background(x, y):
    r, g, b, a = px[x, y]
    return a > 0 and r <= 12 and g <= 12 and b <= 12

# Only remove near-black pixels connected to the outer image border.
for x in range(w):
    for y in (0, h - 1):
        i = y * w + x
        if not seen[i] and is_background(x, y):
            seen[i] = 1
            q.append((x, y))
for y in range(h):
    for x in (0, w - 1):
        i = y * w + x
        if not seen[i] and is_background(x, y):
            seen[i] = 1
            q.append((x, y))

removed = 0
while q:
    x, y = q.popleft()
    r, g, b, a = px[x, y]
    px[x, y] = (r, g, b, 0)
    removed += 1
    for nx, ny in ((x-1,y),(x+1,y),(x,y-1),(x,y+1)):
        if 0 <= nx < w and 0 <= ny < h:
            i = ny * w + nx
            if not seen[i] and is_background(nx, ny):
                seen[i] = 1
                q.append((nx, ny))

im.save(out, "PNG")
alpha_min, alpha_max = im.getchannel("A").getextrema()
print(f"Saved {out} ({w}x{h}); removed {removed} border-connected black pixels; alpha range {alpha_min}-{alpha_max}")
