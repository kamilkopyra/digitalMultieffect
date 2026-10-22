// Font wygładzany: 4 bity alfa na piksel, znaki ASCII 32..126 (generator: ui/tools/gen_assets.py)
#pragma once
#include <stdint.h>

// Grafiki i fonty (ui/assets) w głównym projekcie leżą w QSPI pod 0x90400000
// (sekcja .ui_assets, ld/STM32H750IB_sram_assets.lds) i są czytane wprost z niego
// (tryb memory-mapped), żeby nie zajmowały SRAM z programem. Bez UI_ASSETS_QSPI
// (symulator, tests/ui_daisy) — zwykła pamięć. Nie czytać ich z przerwań:
// podczas zapisu presetu QSPI na chwilę przestaje być zmapowany.
#if defined(UI_ASSETS_QSPI)
#define UI_ASSET __attribute__((section(".ui_assets")))
#else
#define UI_ASSET
#endif

struct Glyph {
	uint32_t offset;   // w bits, wiersze dopełnione do pełnego bajtu
	uint8_t  w, h;
	int8_t   x;        // przesunięcie od pozycji pióra
	int8_t   y;        // od górnej krawędzi linii
	uint8_t  adv;      // przesunięcie pióra
};

struct Font {
	const uint8_t* bits;
	const Glyph*   glyphs;   // [0] = ' '
	uint8_t        height;   // wysokość linii
	uint8_t        ascent;
};
