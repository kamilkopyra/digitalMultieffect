#pragma once
#include "Effect.h"
#include <cmath>

#define FramesPerBuffer  64

class Rat : public Effect
{
public:
    float process(float sample) override;
    std::string getName() override;
    std::string getParamName(int idx) override;
    std::array<int, 3> getDefaultParams() override { return { 100, 80, 100 }; }
private:
    
    float softClipping(float x, float drive);
    void scalePotValues(int pot0, int pot1, int pot2);
	

    float bassFiltered = 0.0f;
    float filtered = 0.0f;

    float drive = 1.0f;
    float volume = 0.1f;
    float tone = 0.1f;

    float ratClipping(float x);

};

