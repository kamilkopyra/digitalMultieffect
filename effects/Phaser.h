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

    float apX[stages] = { 0.0f };   // wejścia poprzednie każdego stopnia
    float apY[stages] = { 0.0f };   // wyjścia poprzednie każdego stopnia
    float feedbackSample = 0.0f;
    float phase = 0.0f;

    float rateHz = 0.3f;
    float depth = 0.6f;       // 0 - 1
    float feedback = 0.3f;    // 0 - 0.9

    void scalePotValues(int pot0, int pot1, int pot2);
};
