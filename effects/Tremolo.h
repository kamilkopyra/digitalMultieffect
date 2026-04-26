#pragma once

#include <cmath>
#include "Effect.h"

#define FramesPerBuffer  64
#define pi 3.14159f


class Tremolo : public Effect
{

public:
	float phase = 0.0f;
	float process(float sample = 32) override;
	float tremoloEffect(float x);
	std::string getName() override;

private:
	float lfoWave(float frequency, float depth);
};

