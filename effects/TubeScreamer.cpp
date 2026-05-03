#include "TubeScreamer.h"

float TubeScreamer::process(float sample) {

    scalePotValues(pot[0], pot[1], pot[2]);
    bassFiltered = bassFiltered + 0.02f * (sample - bassFiltered);
    float highPassed = sample - bassFiltered;

    // soft clip
    float clipped = tanhf(highPassed * drive);

    // low-pass filter
    filtered = filtered + tone * (clipped - filtered);

    return filtered * volume;
}


float TubeScreamer::softClipping(float x, float drive) {
    return tanhf(x * drive);
}

std::string TubeScreamer::getName() {
    return "TubeScreamer";
}

void TubeScreamer::scalePotValues(int pot0, int pot1, int pot2) {
    drive = 2.0f + (pot[0] / 1023.0f) * 8.0f;   // 2-10
    volume = pot[1] / 1023.0f;
    tone = 0.05f + (pot[2] / 1023.0f) * 0.95f;   
}

