#include "AutoWah.h"
#include <algorithm>

static constexpr float kPi = 3.14159265f;

float AutoWah::process(float sample) {

    scalePotValues(pot[0], pot[1], pot[2]);

    // --- śledzenie obwiedni (szybki atak, wolne opadanie) ---
    float level = fabsf(sample);
    if (level > env) env += attackCoeff * (level - env);
    else             env += releaseCoeff * (level - env);

    // --- częstotliwość środkowa filtru z obwiedni ---
    float fc = baseFreq + sensitivity * env * (maxFreq - baseFreq);
    fc = std::clamp(fc, baseFreq, maxFreq);

    // --- filtr stanu zmiennego (Chamberlin) ---
    float f = 2.0f * sinf(kPi * fc / sampleRate);
    lp += f * bp;
    float hp = sample - lp - qCoeff * bp;
    bp += f * hp;

    return sample * (1.0f - mix) + bp * mix;
}

std::string AutoWah::getName() {
    return "AutoWah";
}

void AutoWah::scalePotValues(int pot0, int pot1, int pot2) {
    sensitivity = 0.5f + (pot0 / 100.0f) * 8.0f;   // czułość na obwiednię
    qCoeff = 0.5f - (pot1 / 100.0f) * 0.45f;        // większy pot1 = węższe pasmo (ostrzejszy kwak)
    mix = pot2 / 100.0f;
}

std::string AutoWah::getParamName(int idx) {
    switch (idx) {
    case 0: return "sensitivity";
    case 1: return "resonance";
    case 2: return "mix";
    default: return "?";
    }
}
