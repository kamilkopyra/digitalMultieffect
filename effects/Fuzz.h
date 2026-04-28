#pragma once
#include "Effect.h"
#include <cmath>

#define FramesPerBuffer  64

class Fuzz : public Effect
{
public:
    float process(float sample) override;
    std::string getName() override;
    
private:
    float gain = 0.0f;
    float volume = 0.0f;
	float clipRange = 0.2f;
    float hardClipping(float x);
    float softClipping(float x, float drive);
	void scalePotValues(int pot0, int pot1, int pot2);

};

