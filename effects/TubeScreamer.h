//ready for Daisy
#pragma once
#include "Effect.h"
#include <cmath>


class TubeScreamer : public Effect
{
   
public:
    float process(float sample) override;
    std::string getName() override;
    std::string getParamName(int idx) override;
    std::array<int, 3> getDefaultParams() override { return { 100, 80, 100 }; }
private:

    float volume = 0.0f;
    float drive = 0.0f;
    void scalePotValues(int pot0, int pot1, int pot2);
    void onParamChange() override { scalePotValues(pot[0], pot[1], pot[2]); }
    float tone = 0.0f;
    float bassFiltered = 0.0f;
    float filtered = 0.0f;


};

