#include "Rat.h"
#include <cmath>
#include <algorithm>

float Rat::process(float sample) {

    scalePotValues(pot[0], pot[1], pot[2]);

    // 1. Lekki bass cut przed clippingiem, ale dużo mniejszy niż w TubeScreamerze
    bassFiltered += 0.045f * (sample - bassFiltered);
    float highPassed = sample - bassFiltered;

    // 2. Mocniejsze wzmocnienie
    float driven = highPassed * drive;

    // 3. Twardszy clipping niż TubeScreamer
    float clipped = ratClipping(driven);

    // 4. Ciemniejszy filtr po clippingu
    filtered += tone * (clipped - filtered);

    return filtered * volume;
}

float Rat::ratClipping(float x) {
    // twardy, ale nie całkiem ordynarny clip
    const float clip = 0.45f;

    if (x > clip) {
        return clip + (1.0f - clip) * tanhf((x - clip) * 0.6f);
    }

    if (x < -clip) {
        return -clip + (-1.0f + clip) * tanhf((x + clip) * 0.6f);
    }

    return x;
}

std::string Rat::getName() {
    return "Rat";
}

void Rat::scalePotValues(int pot0, int pot1, int pot2) {
    float driveNorm = pot0 / 1023.0f;
    float volumeNorm = pot1 / 1023.0f;
    float toneNorm = pot2 / 1023.0f;

    drive = 4.0f + driveNorm * 55.0f;
    volume = volumeNorm * 0.15f;
    tone = 0.09f + toneNorm * 0.20f;
}