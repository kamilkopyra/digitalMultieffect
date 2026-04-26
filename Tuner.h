#pragma once
#pragma once

#include <cmath>
#include "Effect.h"

#define FramesPerBuffer  64

class Tuner : public Effect
{

	float process(float sample = 32) override;
	float tuner(float x);
	std::string getName() override;

};

