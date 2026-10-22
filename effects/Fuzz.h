// Ready for Daisy
#pragma once
#include "Effect.h"
#include <cmath>


class Fuzz : public Effect
{
public:
    float process(float sample) override;
    std::string getName() override;
    std::string getParamName(int idx) override;
    std::array<int, 3> getDefaultParams() override { return { 100, 80, 100 }; } 
private:
    float gain = 0.0f;
    float volume = 0.0f;
	float clipRange = 0.2f;
    float hardClipping(float x);
	void scalePotValues(int pot0, int pot1, int pot2);
	void onParamChange() override { scalePotValues(pot[0], pot[1], pot[2]); }

};

