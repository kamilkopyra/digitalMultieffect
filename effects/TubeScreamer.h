#pragma once
#include "Effect.h"
#include <cmath>

#define FramesPerBuffer  64


class TubeScreamer : public Effect
{
   
public:
    float process(float sample) override;
    std::string getName() override;

private:
    float volume = 0.0f;
    float drive = 0.0f;
    float softClipping(float x, float drive);
    void scalePotValues(int pot0, int pot1, int pot2);
    float tone = 0.0f;
    float bassFiltered = 0.0f;
    float filtered = 0.0f;


};

