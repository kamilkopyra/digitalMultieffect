#pragma once

#include <cmath>
#include "Effect.h"

#define FramesPerBuffer  64
#define pi 3.14159f


class Tremolo : public Effect
{

public:
	float process(float x) override;
	std::string getName() override;

private:
	float phase = 0.0f;
	float lfoWave(float frequency, float depth);
	float scalePotValues(int pot0, int pot1);
	float tremoloEffect(float x);
};

