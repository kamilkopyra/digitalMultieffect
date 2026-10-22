// Udawane dane dla ekranów, dopóki nie ma prawdziwego źródła
// (tuner na Daisy — efekt Tuner jeszcze nieprzeniesiony; wszystko w symulatorze).
#pragma once
#include <stdint.h>

float demo_input_peak(uint32_t now);                  // "szarpnięcie struny" co ~2,2 s
bool  demo_tuner(uint32_t now, float peak, float* hz); // struna + odstrojenie, dociągane do zera
