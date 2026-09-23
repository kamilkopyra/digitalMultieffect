#include "Phaser.h"
#include <algorithm>

static constexpr float kPi = 3.14159265f;
static constexpr float kTwoPi = 6.28318531f;

float Phaser::process(float sample) {

    scalePotValues(pot[0], pot[1], pot[2]);

    // LFO -> częstotliwość załamania filtrów wszechprzepustowych
    phase += kTwoPi * rateHz / sampleRate;
    if (phase > kTwoPi) phase -= kTwoPi;
    float lfo = 0.5f + 0.5f * sinf(phase);

    // zamiatanie logarytmiczne między ~200 Hz a ~2 kHz (skalowane głębią)
    float fMin = 200.0f;
    float fMax = 200.0f + depth * 1800.0f;
    float fc = fMin * powf(fMax / fMin, lfo);

    // współczynnik filtru wszechprzepustowego 1. rzędu
    float t = tanf(kPi * fc / sampleRate);
    float a = (t - 1.0f) / (t + 1.0f);

    float x = sample + feedbackSample * feedback;

    // kaskada stopni wszechprzepustowych
    for (int i = 0; i < stages; ++i) {
        float y = a * x + apX[i] - a * apY[i];
        apX[i] = x;
        apY[i] = y;
        x = y;
    }

    feedbackSample = x;

    // 50% sucho / 50% przefiltrowane
    return 0.5f * sample + 0.5f * x;
}

std::string Phaser::getName() {
    return "Phaser";
}

void Phaser::scalePotValues(int pot0, int pot1, int pot2) {
    rateHz = 0.05f + (pot0 / 100.0f) * 1.95f;   // 0.05 - 2 Hz
    depth = pot1 / 100.0f;                       // 0 - 1
    feedback = (pot2 / 100.0f) * 0.7f;           // 0 - 0.9
}

std::string Phaser::getParamName(int idx) {
    switch (idx) {
    case 0: return "Rate";
    case 1: return "Depth";
    case 2: return "Feedback";
    default: return "?";
    }
}

void Phaser::TapToParam(float ms) {
    float hz = 1000.0f / ms;
    pot[0] = std::clamp(static_cast<int>((hz - 0.05f) / 1.95f * 100.0f), 0, 100);
}
