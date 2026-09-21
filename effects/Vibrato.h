#pragma once
#include "Effect.h"
#include <cmath>

// Vibrato — modulacja wysokości dźwięku przez płynnie zmienianą linię
// opóźniającą (LFO steruje pozycją odczytu, interpolacja liniowa).
class Vibrato : public Effect
{
public:
    float process(float sample) override;
    std::string getName() override;
    std::string getParamName(int idx) override;
    std::array<int, 3> getDefaultParams() override { return { 30, 45, 100 }; }
    bool isModulation() override { return true; }
    void TapToParam(float ms) override;

private:
    static constexpr float maxDelayMs = 30.0f;
    static constexpr int bufferSize = static_cast<int>(sampleRate * maxDelayMs / 1000.0f) + 4;

    float buffer[bufferSize] = { 0.0f };
    int writeIndex = 0;
    float phase = 0.0f;

    float rateHz = 5.0f;
    float depthMs = 3.0f;
    float mix = 1.0f;

    void scalePotValues(int pot0, int pot1, int pot2);
    float readInterpolated(float delaySamples);
};
