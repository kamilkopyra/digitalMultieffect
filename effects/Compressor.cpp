#include "Compressor.h"
#include <cmath>

// Błędy:
// Logika wydaje się być ok ale efekt jest dość subtelny
// trzeba pobawić się w ustawienie parametrów
// czasami klika, trzeba dodać histerezę jak w NoiseGate?
// Jest ok, ale trzeba się pobawić w ustawienie parametrów, żeby było bardziej słyszalne

float Compressor::process(float sample) {

    scalePotValues(pot[0], pot[1], pot[2]);

    float compressed = compressSample(sample);

    return compressed * makeupGain;
}

float Compressor::compressSample(float x) {  

    // Envelope follower
    float level = fabsf(x);

    if (level > envelope) {
        envelope += attackCoeff * (level - envelope);
    }
    else {
        envelope += releaseCoeff * (level - envelope);
    }

    // Jeśli sygnał poniżej progu, nie ściskamy
    float targetGain = 1.0f;

    if (envelope > threshold) {
        float exceeded = envelope - threshold;

        // poziom po kompresji
        float compressedLevel = threshold + exceeded / ratio;

        // gain reduction
        targetGain = compressedLevel / envelope;
    }

    // Wygładzanie gainu, żeby nie klikało
    if (targetGain < currentGain) {
        currentGain += attackCoeff * (targetGain - currentGain);
    }
    else {
        currentGain += releaseCoeff * (targetGain - currentGain);
    }

    return x * currentGain;
}

std::string Compressor::getName() {
    return "Compressor";
}

void Compressor::scalePotValues(int pot0, int pot1, int pot2) {

    float thresholdNorm = pot0 / 100.0;
    float ratioNorm = pot1 / 100.0f;
    float makeupNorm = pot2 / 100.0f;

    // pot0 większy = mocniej łapie sygnał
    threshold = 0.35f - thresholdNorm * 0.29f;
    // zakres: 0.35 -> 0.06

    
    ratio = 2.0f + ratioNorm * 8.0f;
    // zakres: 2.0 -> 10.0

    
    makeupGain = 0.9f + makeupNorm * 2.6f;
  
    attackCoeff = 0.02f;

    
    releaseCoeff = 0.0015f;
}

std::string Compressor::getParamName(int idx) {
    switch (idx) {
    case 0: return "Threshold";
    case 1: return "Ratio";
    case 2: return "Makeup Gain";
    default: return "?";
    }
}