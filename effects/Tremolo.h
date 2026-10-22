//ready for Daisy
#pragma once

#include <cmath>
#include "Effect.h"



class Tremolo : public Effect
{

public:
	float process(float x) override;
	std::string getName() override;
	std::string getParamName(int idx) override;
	std::array<int, 3> getDefaultParams() override { return { 100, 80, 0 }; }
	virtual bool isModulation() override { return true; }
private:
	float phase = 0.0f;
	float phaseInc = 0.0f;   // przyrost fazy LFO na próbkę
	float depth = 0.0f;
	float lfoWave(float depth);
	void scalePotValues(int pot0, int pot1);
	void onParamChange() override { scalePotValues(pot[0], pot[1]); }
	float tremoloEffect(float x);

	virtual void TapToParam(float ms) override;
};

