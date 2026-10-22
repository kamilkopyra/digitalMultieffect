// GUI multiefektu na ekran 480x320 — wspólne dla Daisy i symulatora PC.
// Platforma (platform.h) daje ekran, pamięć nieulotną i dane z audio,
// a woła ui_touch() / ui_encoder() z wejść i ui_tick() w pętli głównej.
#pragma once
#include <stdint.h>
#include <string.h>

#define UI_NUM_SLOTS    3
#define UI_NUM_PRESETS  8

struct Preset {
	char    name[9];
	int8_t  effect[UI_NUM_SLOTS];    // indeks efektu (kolejność jak w ui.cpp kEffects)
	uint8_t enabled;                 // bit i = slot i włączony
	uint8_t params[UI_NUM_SLOTS][3]; // 0..100
};

// cały stan zapisywany w pamięci nieulotnej (na Daisy: PersistentStorage w QSPI)
struct Stored {
	uint32_t magic;
	uint8_t  brightness;   // %
	uint8_t  fsMode;       // 0 = footswitche przełączają efekty, 1 = presety
	uint8_t  master;       // 0..100
	uint8_t  curPreset;
	Preset   presets[UI_NUM_PRESETS];

	bool operator==(const Stored& o) const { return memcmp(this, &o, sizeof(Stored)) == 0; }
	bool operator!=(const Stored& o) const { return !(*this == o); }
};

// Enkodery 0..2 (od lewej, pod ekranem). Zasada na wszystkich ekranach poza głównym:
//   E1 kręcenie = wybór, E1 wciśnięcie = OK, E2 kręcenie = wartość, E3 wciśnięcie = wstecz.
// Na głównym: E1..E3 kręcą 3 parametry efektu w fokusie, wciśnięcia opisuje pasek na dole.
enum UiButtonEvent {
	UI_PRESS,   // krótkie wciśnięcie (po puszczeniu)
	UI_HOLD,    // przytrzymanie (~0,5 s), zamiast UI_PRESS
};

void ui_init(uint16_t* framebuffer);
void ui_touch(bool pressed, int x, int y);   // co pętlę, aktualny stan dotyku
void ui_encoder(int index, int delta);       // enkoder 0..2 obrócony o delta kroków
void ui_button(int index, UiButtonEvent ev); // przycisk enkodera 0..2
void ui_tick();                              // odświeżanie mierników + wysłanie zmian na ekran

// aktualny łańcuch (efekty w slotach, włączniki, parametry) — platforma
// przepisuje go do toru audio
const Preset& ui_current();
