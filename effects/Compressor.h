#pragma once
#include "Effect.h"
#include <string>

class Compressor : public Effect {
public:
    float process(float sample) override;
    std::string getName() override;
    std::string getParamName(int idx) override;
    std::array<int, 3> getDefaultParams() override { return { 100, 80, 100 }; }
private:
    void scalePotValues(int pot0, int pot1, int pot2);

    float compressSample(float x);

    float threshold = 0.2f;
    float ratio = 4.0f;
    float makeupGain = 1.0f;

    float envelope = 0.0f;
    float currentGain = 1.0f;

    float attackCoeff = 0.01f;
    float releaseCoeff = 0.001f;
};