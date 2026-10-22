#pragma once
#include "Effect.h"

// Łańcuch efektów — wersja na Daisy Seed, 3 sloty jak w GUI.
//
// Wszystkie efekty są statyczne i rejestrowane raz przy starcie (add) do
// puli. Tor sygnału to 3 sloty: każdy wskazuje jeden efekt z puli (albo
// pusty) i ma własny włącznik. Ten sam efekt nie może być w dwóch slotach
// naraz (to jeden obiekt z jednym stanem filtrów/buforów).
// Kolejność w puli = kolejność efektów w GUI (ui.cpp kEffects), żeby
// indeksy z presetów (Preset.effect[]) pasowały wprost.
//
// Co jest w slotach, czy są włączone i jakie mają parametry — ustawia GUI
// (src/main.cpp przepisuje stan GUI do łańcucha).
//
// Współbieżność: process() woła przerwanie audio, reszta metod jest wołana
// z pętli głównej. Przerwanie tylko czyta pojedyncze int/bool, a ich zapis
// na Cortex-M7 jest atomowy — blokady nie są potrzebne.
class EffectChain
{
public:
	static constexpr int maxEffects = 16;   // rozmiar puli
	static constexpr int numSlots = 3;

	// Rejestracja efektu w puli (tylko przy starcie, przed StartAudio).
	// Efekt dostaje domyślne parametry.
	void add(Effect* effect);

	float process(float sample);

	// wstawia efekt z puli (indeks) do slotu, -1 = pusty slot;
	// false, jeśli ten efekt jest już w innym slocie
	bool setSlot(int slot, int effectIndex);
	int  slotEffect(int slot) const;
	Effect* slot(int slot);

	void setEnabled(int slot, bool on);
	bool isEnabled(int slot) const;

	int size() const { return count; }

private:
	Effect* pool[maxEffects] = {};
	int count = 0;

	volatile int  slotIdx[numSlots] = { -1, -1, -1 };
	volatile bool enabled[numSlots] = {};
};
