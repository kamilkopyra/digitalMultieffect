#pragma once
#include <cmath>
#include "Effect.h"
#include <iostream>

#define FramesPerBuffer  64


class NoiseGate : public Effect
{

public:
	float process(float) override;

private:
	float calcRMS(float buffer[]);
	void scalePotValues(int pot0, int pot1, int pot2);
	float rms = 0.0f;
	float threshold = 0.01; // próg poniżej którego dźwięk będzie tłumiony
	float attack_coeff = 0.01f;
	float release_coeff = 0.0005f;
	float current_gain = 0.0f;

	float noiseGateEffect(float x);
	std::string getName() override;

};