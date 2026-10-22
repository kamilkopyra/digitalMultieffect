#!/bin/sh
# buduje symulator i robi zrzuty: ./build.sh <folder_wyjściowy>
# potrzebny g++ (np. C:/Qt/Tools/mingw1310_64/bin) i Python z Pillow
set -e
cd "$(dirname "$0")"
OUT="${1:-out}"
mkdir -p "$OUT"
SK=../..
for f in font8 font12 font16 font20 font24; do
  g++ -x c++ -c -O1 -Istub -I$SK $SK/$f.c -o "$OUT/$f.o"
done
g++ -O1 -Istub -I$SK -o "$OUT/sim.exe" sim.cpp $SK/LCD_Driver.cpp $SK/LCD_GUI.cpp $SK/DEV_Config.cpp "$OUT"/font*.o
"$OUT/sim.exe" "$OUT"
python -I to_pdf.py "$OUT"
