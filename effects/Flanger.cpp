#include "Flanger.h"
#include <algorithm>

static constexpr float kTwoPi = 6.28318531f;

float Flanger::process(float sample) {

    scalePotValues(pot[0], pot[1], pot[2]);

    // LFO w zakresie [0, 1]
    phase += kTwoPi * rateHz / sampleRate;
    if (phase > kTwoPi) phase -= kTwoPi;
    float lfo = 0.5f + 0.5f * sinf(phase);

    // opóźnienie: baza 1 ms + zamiatanie do ~8 ms
    float baseMs = 1.0f;
    float sweepMs = depth * 7.0f;
    float delayMs = baseMs + sweepMs * lfo;
    float delaySamples = delayMs * sampleRate / 1000.0f;

    float delayed = readInterpolated(delaySamples);

    // sprzężenie zwrotne do bufora
    buffer[writeIndex] = sample + delayed * feedback;

    writeIndex++;
    if (writeIndex >= bufferSize) writeIndex = 0;

    return 0.7f * sample + 0.7f * delayed;
}

float Flanger::readInterpolated(float delaySamples) {
    if (delaySamples < 1.0f) delaySamples = 1.0f;
    if (delaySamples > bufferSize - 2) delaySamples = bufferSize - 2;

    float readPos = static_cast<float>(writeIndex) - delaySamples;
    while (readPos < 0.0f) readPos += bufferSize;

    int i0 = static_cast<int>(readPos);
    int i1 = (i0 + 1) % bufferSize;
    float frac = readPos - i0;

    return buffer[i0] * (1.0f - frac) + buffer[i1] * frac;
}

std::string Flanger::getName() {
    return "Flanger";
}

void Flanger::scalePotValues(int pot0, int pot1, int pot2) {
    rateHz = 0.05f + (pot0 / 100.0f) * 1.95f;   // 0.05 - 2 Hz
    depth = pot1 / 100.0f;                       // 0 - 1
    feedback = (pot2 / 100.0f) * 0.7f;           // 0 - 0.9
}

std::string Flanger::getParamName(int idx) {
    switch (idx) {
    case 0: return "Rate";
    case 1: return "Depth";
    case 2: return "Feedback";
    default: return "?";
    }
}

void Flanger::TapToParam(float ms) {
    float hz = 1000.0f / ms;
    pot[0] = std::clamp(static_cast<int>((hz - 0.05f) / 1.95f * 100.0f), 0, 100);
}
