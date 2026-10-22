# użycie: python make_icons.py <folder z PNG z qt-gui gui/assets/effects> <podgląd.png> > ../icons.h
# PNG pedałów z Qt -> ikonki 2 bit/piksel (4 kolory z własnej palety każdej grafiki)
import sys
from PIL import Image
import numpy as np

src, out_preview = sys.argv[1], sys.argv[2]
names = ["fuzz","tremolo","noisegate","rat","tubescreamer","delay","compressor",
         "vibrato","flanger","phaser","autowah","equalizer","boost"]
W, H = 40, 48
icons = []
for n in names:
    im = Image.open(f"{src}/{n}.png").convert("RGBA")
    bg = Image.new("RGBA", im.size, (42, 42, 42, 255))
    im = Image.alpha_composite(bg, im).convert("RGB")
    w, h = im.size
    # obrazek między gałkami a footswitchem
    art = im.crop((int(w*0.10), int(h*0.24), int(w*0.90), int(h*0.78)))
    art = art.resize((W, H), Image.LANCZOS)
    q = art.quantize(colors=4, method=Image.MAXCOVERAGE, kmeans=8, dither=Image.NONE)
    pal = np.array(q.getpalette()[:12]).reshape(4, 3)
    idx = np.array(q)
    icons.append((n, idx, pal))

scale = 4
prev = Image.new("RGB", ((W+4)*7*scale, (H+4)*2*scale), (30, 30, 30))
for i, (n, idx, pal) in enumerate(icons):
    rgb = pal[idx].astype("uint8")
    img = Image.fromarray(rgb).resize((W*scale, H*scale), Image.NEAREST)
    prev.paste(img, ((i % 7)*(W+4)*scale, (i//7)*(H+4)*scale))
prev.save(out_preview)

def rgb565(c):
    r, g, b = (int(v) for v in c)
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

print("// wygenerowane z gui/assets/effects/*.png (gałąź qt-gui)")
print("// 2 bity/piksel, 4 piksele na bajt (najstarsze bity = lewy piksel), paleta RGB565 na ikonę")
print("// kolejność jak kEffects w Multieffect_UI.ino")
print(f"#define ICON_W {W}\n#define ICON_H {H}\n")
print("const uint16_t kIconPalette[][4] PROGMEM = {")
for n, idx, pal in icons:
    print(f"  {{ {', '.join(f'0x{rgb565(c):04X}' for c in pal)} }},   // {n}")
print("};\n")
print("const uint8_t kIcons[][ICON_W / 4 * ICON_H] PROGMEM = {")
for n, idx, pal in icons:
    flat = idx.flatten()
    packed = [(flat[i] << 6) | (flat[i+1] << 4) | (flat[i+2] << 2) | flat[i+3] for i in range(0, len(flat), 4)]
    print(f"  {{ // {n}")
    for r in range(0, len(packed), 20):
        print("    " + ", ".join(f"0x{v:02X}" for v in packed[r:r+20]) + ",")
    print("  },")
print("};")
