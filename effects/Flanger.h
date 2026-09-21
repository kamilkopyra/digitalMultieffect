#pragma once
#include "Effect.h"
#include <cmath>

// Flanger — krótka, modulowana linia opóźniająca (1-8 ms) ze sprzężeniem
// zwrotnym. Suma sygnału suchego i opóźnionego daje przesuwający się
// grzebieniowy filtr (efekt "odrzutowca").
class Flanger : public Effect
{
public:
    float process(float sample) override;
    std::string getName() override;
    std::string getParamName(int idx) override;
    std::array<int, 3> getDefaultParams() override { return { 20, 70, 40 }; }
    bool isModulation() override { return true; }
    void TapToParam(float ms) override;

private:
    static constexpr float maxDelayMs = 12.0f;
    static constexpr int bufferSize = static_cast<int>(sampleRate * maxDelayMs / 1000.0f) + 4;

    float buffer[bufferSize] = { 0.0f };
    int writeIndex = 0;
    float phase = 0.0f;

    float rateHz = 0.4f;
    float depth = 0.7f;      // 0 - 1 (część zakresu zamiatania)
    float feedback = 0.4f;   // 0 - 0.9

    void scalePotValues(int pot0, int pot1, int pot2);
    float readInterpolated(float delaySamples);
};
