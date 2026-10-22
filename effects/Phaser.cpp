#include "Phaser.h"
#include <algorithm>


float Phaser::process(float sample) {

    // LFO ma najwyżej 2 Hz, więc współczynnik filtrów wystarczy przeliczać
    // co ctrlRate próbek (3 kHz) zamiast co próbkę — różnicy nie słychać
    if (ctrlCounter == 0) {
        ctrlCounter = ctrlRate;

        // LFO -> częstotliwość załamania filtrów wszechprzepustowych
        phase += phaseInc;
        if (phase > twoPi) phase -= twoPi;
        float lfo = 0.5f + 0.5f * sinf(phase);

        // zamiatanie logarytmiczne między ~200 Hz a ~2 kHz (skalowane głębią):
        // fMin * (fMax / fMin)^lfo
        float fc = fMin * expf(lfo * logRatio);

        // współczynnik filtru wszechprzepustowego 1. rzędu
        float t = tanf(pi * fc / sampleRate);
        a = (t - 1.0f) / (t + 1.0f);
    }
    ctrlCounter--;

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
    float rateHz = 0.05f + (pot0 / 100.0f) * 1.95f;   // 0.05 - 2 Hz
    phaseInc = twoPi * rateHz / sampleRate * ctrlRate;

    float depth = pot1 / 100.0f;                       // 0 - 1
    float fMax = fMin + depth * 1800.0f;
    logRatio = logf(fMax / fMin);

    feedback = (pot2 / 100.0f) * 0.7f;           // 0 - 0.7
}

std::string Phaser::getParamName(int idx) {
    switch (idx) {
    case 0: return "rate";
    case 1: return "depth";
    case 2: return "feedback";
    default: return "?";
    }
}

void Phaser::TapToParam(float ms) {
    float hz = 1000.0f / ms;
    pot[0] = std::clamp(static_cast<int>((hz - 0.05f) / 1.95f * 100.0f), 0, 100);
    onParamChange();
}
