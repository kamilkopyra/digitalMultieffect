# zrzuty PPM z symulatora -> PNG (2x) + PDF ze stronami z podpisami
import glob
import os
import sys
import zlib

from PIL import Image, ImageDraw, ImageFont

CAPTIONS = {
    "01_start":          "Start: P1 Clean. Suwaki 1-3 = enkodery 1-3, pasek na dole = co robi ich wciśnięcie",
    "02_knobs":          "E1/E3 kręcone (Compressor), E1 wciśnięty -> fokus Equalizer, E2 kręci Mid; * = niezapisane",
    "03_master":         "E1 wciśnięty aż do VOL: E1 = głośność, miernik wejścia, CLIP po przesterze",
    "04_menu":           "E3 wciśnięty: Menu (E1 kręci = wybór, E1 = OK, E3 = wstecz)",
    "05_picker":         "Change effect: E1 przesuwa kursor, zielona dioda = aktualny efekt slotu",
    "06_presets":        "Presety: E1 wybiera, E1 = Load, E2 = Save (drugie wciśnięcie nadpisuje)",
    "07_lead":           "Wczytany P3 Lead",
    "08_tuner":          "Tuner na cały ekran: struna, nuta, częstotliwość, odchyłka w centach",
    "09_tuner_in_tune":  "Tuner: nastrojone (±5 centów)",
    "10_settings":       "Settings: E1 wybiera wiersz, E2 kręci wartość, reset = 2x wciśnięcie E1",
    "11_phaser_tap":     "Phaser (modulacja): E3 = TAP tempo, przytrzymanie E3 = menu",
}

NL = b"\n"


def write_pdf(path, images, dpi=150):
    # minimalny PDF: każda strona = jeden obraz RGB skompresowany zlib
    # (bezstratnie; ten Pillow nie ma libjpeg, którego wymaga jego zapis PDF)
    objs = []
    kids = []
    for im in images:
        w, h = im.size
        pw, ph = w * 72 / dpi, h * 72 / dpi
        data = zlib.compress(im.tobytes(), 9)
        img_id = len(objs) + 3
        objs.append(b"<< /Type /XObject /Subtype /Image /Width %d /Height %d /ColorSpace /DeviceRGB "
                    b"/BitsPerComponent 8 /Filter /FlateDecode /Length %d >>" % (w, h, len(data))
                    + NL + b"stream" + NL + data + NL + b"endstream")
        content = b"q %.2f 0 0 %.2f 0 0 cm /Im0 Do Q" % (pw, ph)
        objs.append(b"<< /Length %d >>" % len(content) + NL + b"stream" + NL + content + NL + b"endstream")
        objs.append(b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 %.2f %.2f] /Contents %d 0 R "
                    b"/Resources << /XObject << /Im0 %d 0 R >> >> >>" % (pw, ph, img_id + 1, img_id))
        kids.append(img_id + 2)
    all_objs = [b"<< /Type /Catalog /Pages 2 0 R >>",
                b"<< /Type /Pages /Kids [%s] /Count %d >>" % (b" ".join(b"%d 0 R" % k for k in kids), len(kids))]
    all_objs += objs

    buf = bytearray(b"%PDF-1.4" + NL)
    offsets = []
    for i, o in enumerate(all_objs, 1):
        offsets.append(len(buf))
        buf += b"%d 0 obj" % i + NL + o + NL + b"endobj" + NL
    xref = len(buf)
    buf += b"xref" + NL + b"0 %d" % (len(all_objs) + 1) + NL + b"0000000000 65535 f " + NL
    for off in offsets:
        buf += b"%010d 00000 n " % off + NL
    buf += b"trailer" + NL + b"<< /Size %d /Root 1 0 R >>" % (len(all_objs) + 1) + NL
    buf += b"startxref" + NL + b"%d" % xref + NL + b"%%EOF" + NL
    with open(path, "wb") as f:
        f.write(buf)


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
    d.text((60, 40), "Multieffect UI (Daisy) — 480x320, podgląd 2x", fill=(30, 30, 30), font=title_font)
    d.rectangle((52, 112, 60 + big.width + 7, 120 + big.height + 7), fill=(10, 10, 10))
    page.paste(big, (60, 120))
    d.text((60, 120 + big.height + 24), CAPTIONS.get(name, name), fill=(60, 60, 60), font=font)
    pages.append(page)

if pages:
    pdf = os.path.join(out, "Multieffect_UI_Daisy.pdf")
    write_pdf(pdf, pages)
    print(pdf)
