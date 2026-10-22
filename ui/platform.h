// To, co GUI potrzebuje od platformy. Implementacje:
//   src/main.cpp            — Daisy Seed: ekran po SPI, QSPI, łańcuch efektów
//   tests/ui_daisy/         — Daisy Seed: samo GUI z przelotem audio (test)
//   tests/ui_sim/sim.cpp    — symulator na PC (PNG/PDF)
#pragma once
#include <stdint.h>
#include <stddef.h>

uint32_t plat_millis();

// wyślij prostokąt [x0,x1) x [y0,y1) z framebuffera (stride = gfx::W) na ekran
void plat_lcd_write(int x0, int y0, int x1, int y1, const uint16_t* fb);

void plat_backlight(uint8_t percent);

// pamięć nieulotna: presety + ustawienia jako jeden blok
void plat_load(void* dst, size_t size);
void plat_save(const void* src, size_t size);

float plat_input_peak();          // max |próbka| od ostatniego wywołania (1.0 = pełna skala)
float plat_cpu_load();            // 0..1
void  plat_set_master(float gain);

// tuner: true + częstotliwość w Hz, false = brak sygnału; demo = dane udawane
bool plat_tuner(float* hz, bool* demo);

// tap tempo dla efektu w slocie (ms między uderzeniami) -> nowa wartość
// parametru 1 (0..100) ustawiona przez efekt, -1 = efekt nie obsługuje tapu
int plat_tap(int slot, float ms);
