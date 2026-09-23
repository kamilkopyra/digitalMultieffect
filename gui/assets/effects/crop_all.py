"""
Przycina wszystkie pliki PNG w tym folderze do faktycznej (nieprzezroczystej)
treści, z niewielkim marginesem. Odpal po każdej podmianie/dodaniu obrazka:

    python crop_all.py

Wymaga: pip install pillow
"""
import os
from PIL import Image

PAD = 20
HERE = os.path.dirname(os.path.abspath(__file__))

for fn in sorted(os.listdir(HERE)):
    if not fn.lower().endswith(".png"):
        continue
    path = os.path.join(HERE, fn)
    im = Image.open(path).convert("RGBA")
    bbox = im.getbbox()
    if not bbox:
        print(fn, "- pusty plik, pomijam")
        continue

    l, t, r, b = bbox
    l = max(0, l - PAD)
    t = max(0, t - PAD)
    r = min(im.width, r + PAD)
    b = min(im.height, b + PAD)
    cropped = im.crop((l, t, r, b))

    if cropped.size != im.size:
        cropped.save(path)
        print(fn, im.size, "->", cropped.size)
    else:
        print(fn, "bez zmian")
