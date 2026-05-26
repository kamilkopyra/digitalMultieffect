#include "Delay.h"
#include <algorithm>

Delay::Delay() {
    for (int i = 0; i < bufferSize; ++i) {
        delayBuffer[i] = 0.0f;
    }
}

float Delay::process(float sample) {

    scalePotValues(pot[0], pot[1], pot[2]);

    int delaySamples = static_cast<int>((delayTimeMs / 1000.0f) * sampleRate);

    if (delaySamples < 1) {
        delaySamples = 1;
    }

    if (delaySamples >= bufferSize) {
        delaySamples = bufferSize - 1;
    }

    int readIndex = writeIndex - delaySamples;

    if (readIndex < 0) {
        readIndex += bufferSize;
    }

    float delayedSample = delayBuffer[readIndex];

    float output = sample * (1.0f - mix) + delayedSample * mix;

    delayBuffer[writeIndex] = sample + delayedSample * feedback;

    writeIndex++;

    if (writeIndex >= bufferSize) {
        writeIndex = 0;
    }

    return output;
}

std::string Delay::getName() {
    return "Delay";
}

void Delay::scalePotValues(int pot0, int pot1, int pot2) {
    float timeNorm = pot0 / 100.0f;
    float feedbackNorm = pot1 / 100.0f;
    float mixNorm = pot2 / 100.0f;

    delayTimeMs = 400.0f + timeNorm * 1200.0f;  // 400-1600 ms
    feedback = feedbackNorm * 0.4f;          // 0-0.4
    mix = mixNorm * 0.65f;                    // 0-0.65
}

std::string Delay::getParamName(int idx) {
    switch (idx) {
    case 0: return "Time";
    case 1: return "Feedback";
    case 2: return "Mix";
    default: return "?";
    }
}