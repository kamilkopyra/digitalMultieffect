#include "Equalizer.h"
#include <algorithm>

static constexpr float kPi = 3.14159265f;

float Equalizer::potToDb(int pot) {
    // 0 -> -12 dB, 50 -> 0 dB, 100 -> +12 dB
    return (pot - 50) / 50.0f * 12.0f;
}

void Equalizer::makeLowShelf(Biquad& bq, float f0, float dBgain) {
    float A = powf(10.0f, dBgain / 40.0f);
    float w0 = 2.0f * kPi * f0 / sampleRate;
    float cw = cosf(w0), sw = sinf(w0);
    float alpha = sw / 2.0f * 1.41421356f;          // S = 1
    float twoSqrtAalpha = 2.0f * sqrtf(A) * alpha;

    float b0 = A * ((A + 1) - (A - 1) * cw + twoSqrtAalpha);
    float b1 = 2 * A * ((A - 1) - (A + 1) * cw);
    float b2 = A * ((A + 1) - (A - 1) * cw - twoSqrtAalpha);
    float a0 = (A + 1) + (A - 1) * cw + twoSqrtAalpha;
    float a1 = -2 * ((A - 1) + (A + 1) * cw);
    float a2 = (A + 1) + (A - 1) * cw - twoSqrtAalpha;

    bq.b0 = b0 / a0; bq.b1 = b1 / a0; bq.b2 = b2 / a0;
    bq.a1 = a1 / a0; bq.a2 = a2 / a0;
}

void Equalizer::makeHighShelf(Biquad& bq, float f0, float dBgain) {
    float A = powf(10.0f, dBgain / 40.0f);
    float w0 = 2.0f * kPi * f0 / sampleRate;
    float cw = cosf(w0), sw = sinf(w0);
    float alpha = sw / 2.0f * 1.41421356f;
    float twoSqrtAalpha = 2.0f * sqrtf(A) * alpha;

    float b0 = A * ((A + 1) + (A - 1) * cw + twoSqrtAalpha);
    float b1 = -2 * A * ((A - 1) + (A + 1) * cw);
    float b2 = A * ((A + 1) + (A - 1) * cw - twoSqrtAalpha);
    float a0 = (A + 1) - (A - 1) * cw + twoSqrtAalpha;
    float a1 = 2 * ((A - 1) - (A + 1) * cw);
    float a2 = (A + 1) - (A - 1) * cw - twoSqrtAalpha;

    bq.b0 = b0 / a0; bq.b1 = b1 / a0; bq.b2 = b2 / a0;
    bq.a1 = a1 / a0; bq.a2 = a2 / a0;
}

void Equalizer::makePeak(Biquad& bq, float f0, float q, float dBgain) {
    float A = powf(10.0f, dBgain / 40.0f);
    float w0 = 2.0f * kPi * f0 / sampleRate;
    float cw = cosf(w0), sw = sinf(w0);
    float alpha = sw / (2.0f * q);

    float b0 = 1 + alpha * A;
    float b1 = -2 * cw;
    float b2 = 1 - alpha * A;
    float a0 = 1 + alpha / A;
    float a1 = -2 * cw;
    float a2 = 1 - alpha / A;

    bq.b0 = b0 / a0; bq.b1 = b1 / a0; bq.b2 = b2 / a0;
    bq.a1 = a1 / a0; bq.a2 = a2 / a0;
}

void Equalizer::recalcIfNeeded(int pot0, int pot1, int pot2) {
    if (pot0 == lastPot[0] && pot1 == lastPot[1] && pot2 == lastPot[2])
        return;
    lastPot[0] = pot0; lastPot[1] = pot1; lastPot[2] = pot2;

    makeLowShelf(low, 120.0f, potToDb(pot0));
    makePeak(mid, 1000.0f, 0.9f, potToDb(pot1));
    makeHighShelf(high, 4000.0f, potToDb(pot2));
}

float Equalizer::process(float sample) {
    recalcIfNeeded(pot[0], pot[1], pot[2]);

    float y = low.process(sample);
    y = mid.process(y);
    y = high.process(y);
    return y;
}

std::string Equalizer::getName() {
    return "Equalizer";
}

std::string Equalizer::getParamName(int idx) {
    switch (idx) {
    case 0: return "low";
    case 1: return "mid";
    case 2: return "high";
    default: return "?";
    }
}
