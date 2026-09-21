#include "Vibrato.h"
#include <algorithm>

static constexpr float kTwoPi = 6.28318531f;

float Vibrato::process(float sample) {

    scalePotValues(pot[0], pot[1], pot[2]);

    // zapis próbki do bufora kołowego
    buffer[writeIndex] = sample;

    // LFO — sinus w zakresie [-1, 1]
    phase += kTwoPi * rateHz / sampleRate;
    if (phase > kTwoPi) phase -= kTwoPi;
    float lfo = sinf(phase);

    // opóźnienie oscyluje wokół wartości środkowej
    float centerMs = maxDelayMs * 0.5f;
    float delayMs = centerMs + depthMs * lfo;
    float delaySamples = delayMs * sampleRate / 1000.0f;

    float wet = readInterpolated(delaySamples);

    writeIndex++;
    if (writeIndex >= bufferSize) writeIndex = 0;

    return sample * (1.0f - mix) + wet * mix;
}

float Vibrato::readInterpolated(float delaySamples) {
    if (delaySamples < 1.0f) delaySamples = 1.0f;
    if (delaySamples > bufferSize - 2) delaySamples = bufferSize - 2;

    float readPos = static_cast<float>(writeIndex) - delaySamples;
    while (readPos < 0.0f) readPos += bufferSize;

    int i0 = static_cast<int>(readPos);
    int i1 = (i0 + 1) % bufferSize;
    float frac = readPos - i0;

    return buffer[i0] * (1.0f - frac) + buffer[i1] * frac;
}

std::string Vibrato::getName() {
    return "Vibrato";
}

void Vibrato::scalePotValues(int pot0, int pot1, int pot2) {
    rateHz = 0.5f + (pot0 / 100.0f) * 8.0f;   // 0.5 - 8 Hz
    depthMs = (pot1 / 100.0f) * 2.0f;         // 0 - 12 ms
    mix = pot2 / 100.0f;                        // 0 - 1
}

std::string Vibrato::getParamName(int idx) {
    switch (idx) {
    case 0: return "rate";
    case 1: return "depth";
    case 2: return "mix";
    default: return "?";
    }
}

void Vibrato::TapToParam(float ms) {
    float hz = 1000.0f / ms;
    pot[0] = std::clamp(static_cast<int>((hz - 0.5f) / 7.5f * 100.0f), 0, 100);
}
