#include "Boost.h"

float Boost::process(float sample) {

    scalePotValues(pot[0], pot[1], pot[2]);

    float boosted = sample * gain;

    // prosty filtr barwy — jak w Rat/TubeScreamer: mniejszy "tone" = ciemniej
    filtered += tone * (boosted - filtered);

    // delikatny soft-clip — nie jest to "efekt" zniekształcający, tylko
    // zabezpieczenie przed twardym cyfrowym obcięciem przy dużym gainie
    float safe = tanhf(filtered);

    return safe * volume;
}

std::string Boost::getName() {
    return "Boost";
}

void Boost::scalePotValues(int pot0, int pot1, int pot2) {
    gain = 1.0f + (pot0 / 100.0f) * 5.0f;     // 1x - 6x (0 - ok. +15.5 dB)
    tone = 0.15f + (pot1 / 100.0f) * 0.85f;    // 0.15 - 1.0 (ciemno - jasno)
    volume = pot2 / 100.0f;                     // 0 - 1
}

std::string Boost::getParamName(int idx) {
    switch (idx) {
    case 0: return "gain";
    case 1: return "tone";
    case 2: return "volume";
    default: return "?";
    }
}
