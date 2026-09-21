#pragma once
#include "Effect.h"
#include <cmath>

// AutoWah — filtr pasmowo-przepustowy (SVF, wg Chamberlina), którego
// częstotliwość środkowa podąża za obwiednią sygnału wejściowego.
// Mocniejsze szarpnięcie struny = wyższe otwarcie filtru ("kwak").
class AutoWah : public Effect
{
public:
    float process(float sample) override;
    std::string getName() override;
    std::string getParamName(int idx) override;
    std::array<int, 3> getDefaultParams() override { return { 60, 40, 100 }; }

private:
    float env = 0.0f;
    float lp = 0.0f;
    float bp = 0.0f;

    float sensitivity = 3.0f;   // ile Hz na jednostkę obwiedni
    float qCoeff = 0.3f;        // 1/Q
    float mix = 1.0f;

    static constexpr float baseFreq = 200.0f;
    static constexpr float maxFreq = 3000.0f;
    static constexpr float attackCoeff = 0.01f;
    static constexpr float releaseCoeff = 0.0008f;

    void scalePotValues(int pot0, int pot1, int pot2);
};
