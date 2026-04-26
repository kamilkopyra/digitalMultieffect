#pragma once
#include <cmath>
#include "Effect.h"
#include <iostream>

#define FramesPerBuffer  64


class NoiseGate : public Effect
{

public:
	float rms = 0.0f; 
	float threshold = 0.01; // próg poniżej którego dźwięk będzie tłumiony
	float process(float sample = 32) override;
	float noiseGateEffect(float x);
	std::string getName() override;

private:
	float calcRMS(float buffer[]);

};