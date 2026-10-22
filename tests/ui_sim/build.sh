#!/bin/sh
# Buduje symulator GUI i robi zrzuty + PDF:  ./build.sh [folder_wyjściowy]
# Potrzebny g++ (np. C:/Qt/Tools/mingw1310_64/bin na PATH) i Python z Pillow.
set -e
cd "$(dirname "$0")"
OUT="${1:-out}"
mkdir -p "$OUT"
g++ -std=gnu++17 -O2 -Wall -I../../ui -o "$OUT/ui_sim.exe" sim.cpp ../../ui/gfx.cpp ../../ui/ui.cpp ../../ui/demo.cpp
"$OUT/ui_sim.exe" "$OUT"
python -I to_pdf.py "$OUT"
