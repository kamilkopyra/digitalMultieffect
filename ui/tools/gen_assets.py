# Generuje ui/assets/fonts.h i ui/assets/icons.h.
#
#   python ui/tools/gen_assets.py [folder_z_png]
#
# Grafiki: PNG pedałów z wersji Qt (gui/assets/effects/*.png). Bez argumentu
# pobierane prosto z gałęzi qt-gui repo ../Multieffect (git show).
# Fonty: Source Han Sans JP (SIL Open Font License) z C:/Windows/Fonts —
# wygładzane, 4 bity alfa na piksel.
import io
import os
import subprocess
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "assets")
REPO_PC = os.path.join(HERE, "..", "..", "..", "Multieffect")

# kolejność = indeksy efektów w ui.cpp (kEffects)
EFFECTS = ["fuzz", "tremolo", "noisegate", "rat", "tubescreamer", "delay", "compressor",
           "vibrato", "flanger", "phaser", "autowah", "equalizer", "boost"]
ICON_W, ICON_H = 80, 96

FONT_DIR = "C:/Windows/Fonts"
FONTS = [
    # nazwa,       plik,                        px, znaki
    ("small",  "SourceHanSansJP-Normal.otf", 13, None),
    ("body",   "SourceHanSansJP-Normal.otf", 16, None),
    ("label",  "SourceHanSansJP-Bold.otf",   15, None),
    ("value",  "SourceHanSansJP-Bold.otf",   19, None),
    ("title",  "SourceHanSansJP-Bold.otf",   30, None),
    ("big",    "SourceHanSansJP-Bold.otf",   88, "0123456789ABCDEFGe#-"),
]


def load_png(name, folder):
    if folder:
        return Image.open(os.path.join(folder, name + ".png"))
    data = subprocess.check_output(
        ["git", "-C", REPO_PC, "show", f"qt-gui:gui/assets/effects/{name}.png"])
    return Image.open(io.BytesIO(data))


def rgb565(r, g, b):
    return ((int(r) >> 3) << 11) | ((int(g) >> 2) << 5) | (int(b) >> 3)


def gen_icons(folder):
    lines = [
        "// WYGENEROWANE przez ui/tools/gen_assets.py — nie edytować ręcznie",
        "// grafiki pedałów z wersji Qt (gui/assets/effects), wycięty obrazek, RGB565",
        "#pragma once",
        '#include "../font.h"   // UI_ASSET',
        "",
        f"#define ICON_W {ICON_W}",
        f"#define ICON_H {ICON_H}",
        f"#define ICON_COUNT {len(EFFECTS)}",
        "",
        "static const uint16_t kIcons[ICON_COUNT][ICON_W * ICON_H] UI_ASSET = {",
    ]
    for n in EFFECTS:
        im = load_png(n, folder).convert("RGBA")
        bg = Image.new("RGBA", im.size, (42, 42, 42, 255))
        im = Image.alpha_composite(bg, im).convert("RGB")
        w, h = im.size
        art = im.crop((int(w * 0.10), int(h * 0.24), int(w * 0.90), int(h * 0.78)))
        art = art.resize((ICON_W, ICON_H), Image.LANCZOS)
        px = np.array(art).reshape(-1, 3)
        vals = [rgb565(*p) for p in px]
        lines.append(f"  {{ // {n}")
        for i in range(0, len(vals), 16):
            lines.append("    " + ", ".join(f"0x{v:04X}" for v in vals[i:i + 16]) + ",")
        lines.append("  },")
    lines.append("};")
    with open(os.path.join(OUT, "icons.h"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def gen_fonts():
    lines = [
        "// WYGENEROWANE przez ui/tools/gen_assets.py — nie edytować ręcznie",
        "// Source Han Sans JP (SIL OFL 1.1), wygładzane, 4 bity alfa/piksel",
        "#pragma once",
        '#include "../font.h"',
        "",
    ]
    for name, file, px, chars in FONTS:
        font = ImageFont.truetype(os.path.join(FONT_DIR, file), px)
        codes = range(32, 127)
        # wysokość linii z łacińskich znaków (metryki fontu CJK są za wysokie)
        top = min(font.getbbox(chr(c), anchor="ls")[1] for c in codes if chr(c).strip())
        bottom = max(font.getbbox(chr(c), anchor="ls")[3] for c in codes if chr(c).strip())
        ascent = -top
        height = ascent + bottom

        bitmap = bytearray()
        glyphs = []
        for c in codes:
            ch = chr(c)
            adv = int(round(font.getlength(ch)))
            if (chars is not None and ch not in chars and ch != " ") or not ch.strip():
                glyphs.append((0, 0, 0, 0, 0, adv if ch == " " or chars is None else 0))
                continue
            x0, y0, x1, y1 = font.getbbox(ch, anchor="ls")
            gw, gh = x1 - x0, y1 - y0
            img = Image.new("L", (gw, gh), 0)
            ImageDraw.Draw(img).text((-x0, -y0), ch, font=font, fill=255, anchor="ls")
            a = (np.array(img).astype(np.int32) * 15 + 127) // 255
            off = len(bitmap)
            for row in a:
                if len(row) % 2:
                    row = np.append(row, 0)
                for i in range(0, len(row), 2):
                    bitmap.append((row[i] << 4) | row[i + 1])
            glyphs.append((off, gw, gh, x0, y0 + ascent, adv))

        lines.append(f"static const uint8_t kFont_{name}_bits[] UI_ASSET = {{")
        for i in range(0, len(bitmap), 24):
            lines.append("  " + ", ".join(f"0x{b:02X}" for b in bitmap[i:i + 24]) + ",")
        lines.append("};")
        lines.append(f"static const Glyph kFont_{name}_glyphs[] UI_ASSET = {{")
        for g in glyphs:
            lines.append(f"  {{ {g[0]}, {g[1]}, {g[2]}, {g[3]}, {g[4]}, {g[5]} }},")
        lines.append("};")
        lines.append(f"static const Font kFont_{name} = {{ kFont_{name}_bits, kFont_{name}_glyphs, {height}, {ascent} }};")
        lines.append("")
        print(f"font {name}: {px}px, height {height}, {len(bitmap)} B", file=sys.stderr)

    with open(os.path.join(OUT, "fonts.h"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    gen_icons(sys.argv[1] if len(sys.argv) > 1 else None)
    gen_fonts()
