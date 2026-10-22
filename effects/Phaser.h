#pragma once
#include "Effect.h"
#include <cmath>

// Phaser — kaskada filtrów wszechprzepustowych 1. rzędu, których
// częstotliwość załamania jest zamiatana przez LFO. Suma z sygnałem
// suchym tworzy ruchome zafalowania w widmie. Dodane sprzężenie zwrotne
// pogłębia efekt.
class Phaser : public Effect
{
public:
    float process(float sample) override;
    std::string getName() override;
    std::string getParamName(int idx) override;
    std::array<int, 3> getDefaultParams() override { return { 25, 60, 30 }; }
    bool isModulation() override { return true; }
    void TapToParam(float ms) override;

private:
    static constexpr int stages = 6;
    static constexpr int ctrlRate = 16;   // co ile próbek przeliczać LFO i współczynnik filtru
    static constexpr float fMin = 200.0f;

    float apX[stages] = { 0.0f };   // wejścia poprzednie każdego stopnia
    float apY[stages] = { 0.0f };   // wyjścia poprzednie każdego stopnia
    float feedbackSample = 0.0f;
    float phase = 0.0f;
    int ctrlCounter = 0;
    float a = 0.0f;           // współczynnik filtrów wszechprzepustowych

    float phaseInc = 0.0f;    // przyrost fazy LFO na krok sterowania (ctrlRate próbek)
    float logRatio = 0.0f;    // logf(fMax / fMin), zależy od depth
    float feedback = 0.3f;    // 0 - 0.7

    void scalePotValues(int pot0, int pot1, int pot2);
    void onParamChange() override { scalePotValues(pot[0], pot[1], pot[2]); }
};
