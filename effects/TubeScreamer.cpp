#include "TubeScreamer.h"

float TubeScreamer::process(float sample) {

    bassFiltered = bassFiltered + 0.15f * (sample - bassFiltered);
    float highPassed = sample - bassFiltered;

    // soft clip
    float clipped = tanhf(highPassed * drive);

    // low-pass filter
    filtered = filtered + tone * (clipped - filtered);

    return filtered * volume;
}


std::string TubeScreamer::getName() {
    return "TubeScreamer";
}

void TubeScreamer::scalePotValues(int pot0, int pot1, int pot2) {
    drive = 2.0f + (pot0 / 100.0f) * 12.0f;   // 2-10
    volume = pot1 / 100.0f;
    tone = 0.05f + (pot2 / 100.0f) * 0.95f;   
}

std::string TubeScreamer::getParamName(int idx) {
    switch (idx) {
    case 0: return "drive";
    case 1: return "volume";
    case 2: return "tone";
    default: return "?";
    }
}

