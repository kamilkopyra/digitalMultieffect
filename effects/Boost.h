#pragma once
#include "Effect.h"
#include <cmath>

// Boost — zwykły, czysty wzmacniacz sygnału (bez zniekształceń). Prosta
// barwa (tone) tym samym mechanizmem co w Rat/TubeScreamer, plus delikatny
// soft-clip na wyjściu jako zabezpieczenie przed cyfrowym obcięciem przy
// dużym poziomie boostu (analogowe boostery też miękko nasycają się na
// krańcach zakresu).
class Boost : public Effect
{
public:
    float process(float sample) override;
    std::string getName() override;
    std::string getParamName(int idx) override;
    std::array<int, 3> getDefaultParams() override { return { 40, 70, 80 }; }

private:
    float gain = 1.0f;
    float tone = 0.6f;
    float volume = 1.0f;
    float filtered = 0.0f;

    void scalePotValues(int pot0, int pot1, int pot2);
};
