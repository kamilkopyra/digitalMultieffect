# Grafiki efektów

Wrzuć tu pliki PNG (z przezroczystością), po jednym na efekt. Nazwa pliku =
nazwa efektu małymi literami, bez spacji:

```
fuzz.png
rat.png
tubescreamer.png
boost.png
compressor.png
noisegate.png
delay.png
tremolo.png
vibrato.png
flanger.png
phaser.png
autowah.png
equalizer.png
tuner.png
```

Zalecenia:
- PNG z kanałem alfa (przezroczyste tło)
- kwadratowy kadr (np. 512x512) — GUI skaluje z zachowaniem proporcji
  (`Image.PreserveAspectFit`), więc kwadrat najlepiej wykorzystuje miejsce

Nie trzeba nic zmieniać w CMake ani przebudowywać projektu od zera po
dodaniu plików — wystarczy zwykły build, folder jest kopiowany obok .exe
automatycznie. Dopóki pliku nie ma, GUI pokaże w tym miejscu podpowiedź
z dokładną oczekiwaną nazwą pliku.
