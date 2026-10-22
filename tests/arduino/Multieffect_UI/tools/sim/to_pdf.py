# zrzuty PPM z symulatora -> PNG (2x) + PDF ze stronami z podpisami
import sys, glob, os
from PIL import Image, ImageDraw, ImageFont

CAPTIONS = {
    "01_start":           "Start: preset P1 Clean, fokus na pierwszym efekcie",
    "02_edit_modified":   "Edycja: zmieniony parametr i Delay ON -> gwiazdka przy presecie (niezapisane)",
    "03_master":          "VOL: fokus na master volume, miernik wejścia w panelu",
    "04_presets":         "Presety: 8 w EEPROM, zaznaczony P3, Load / Save",
    "05_lead":            "Wczytany P3 Lead",
    "06_tuner":           "Tuner na cały ekran: struna, częstotliwość, centy",
    "07_tuner_in_tune":   "Tuner: nastrojone",
    "08_settings":        "Settings: jasność, footswitche, CPU, info, reset (2x dotknięcie)",
    "09_picker":          "Change effect: wybór efektu dla slotu w fokusie",
}

out = sys.argv[1]
font = ImageFont.load_default(size=26)
title_font = ImageFont.load_default(size=34)
pages = []
for p in sorted(glob.glob(os.path.join(out, "*.ppm"))):
    name = os.path.basename(p)[:-4]
    im = Image.open(p).convert("RGB")
    big = im.resize((im.width * 2, im.height * 2), Image.NEAREST)
    big.save(os.path.join(out, name + ".png"))

    page = Image.new("RGB", (big.width + 120, big.height + 200), (245, 245, 245))
    d = ImageDraw.Draw(page)
    d.text((60, 40), "Multieffect UI — 480x320 (podgląd 2x)", fill=(30, 30, 30), font=title_font)
    d.rectangle((52, 112, 60 + big.width + 7, 120 + big.height + 7), fill=(10, 10, 10))
    page.paste(big, (60, 120))
    d.text((60, 120 + big.height + 24), CAPTIONS.get(name, name), fill=(60, 60, 60), font=font)
    # PDF bez JPEG: paleta 256 kolorów (UI ma ich mniej), zapis bezstratny
    pages.append(page.quantize(colors=256, method=Image.MAXCOVERAGE, dither=Image.NONE))

if pages:
    pdf = os.path.join(out, "Multieffect_UI.pdf")
    pages[0].save(pdf, save_all=True, append_images=pages[1:], resolution=150)
    print(pdf)
