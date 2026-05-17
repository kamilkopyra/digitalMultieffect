#pragma once
#include <string>
#include <vector>
#include "Effect.h"

class Delay : public Effect
{
public:
    Delay();

    float process(float sample) override;
    std::string getName() override;
private:
    static constexpr float maxDelaySeconds = 1.0f;
    static constexpr int bufferSize = static_cast<int>(sampleRate * maxDelaySeconds);

    float delayBuffer[bufferSize] = { 0.0f };

    int writeIndex = 0;

    float delayTimeMs = 300.0f;
    float feedback = 0.3f;
    float mix = 0.3f;
    void scalePotValues(int pot0, int pot1, int pot2);
};