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
	std::string getParamName(int idx) override;
	std::array<int, 3> getDefaultParams() override { return { 100, 80, 0 }; }
private:
	float phase = 0.0f;
	float lfoWave(float frequency, float depth);
	float scalePotValues(int pot0, int pot1);
	float tremoloEffect(float x);
};

