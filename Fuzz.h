#pragma once
#include "Effect.h"
#include <cmath>

#define FramesPerBuffer  64
#define clipRange        0.05f

class Fuzz : public Effect
{
    float process(float sample=32) override;
    float hardClipping(float x);
    float softClipping(float x, float drive);
    std::string getName() override;

};

