#include "Compressor.h"
#include <cmath>

// Błędy:
// Logika wydaje się być ok ale efekt jest dość subtelny
// trzeba pobawić się w ustawienie parametrów
// czasami klika, trzeba dodać histerezę jak w NoiseGate?
// Jest ok, ale trzeba się pobawić w ustawienie parametrów, żeby było bardziej słyszalne

float Compressor::process(float sample) {

    float compressed = compressSample(sample);

    return compressed * makeupGain;
}

float Compressor::compressSample(float x) {  

    // Envelope follower — jedyne wygładzanie (attack/release), gain liczony wprost z obwiedni
    float level = fabsf(x);

    if (level > envelope) {
        envelope += attackCoeff * (level - envelope);
    }
    else {
        envelope += releaseCoeff * (level - envelope);
    }

    // Jeśli sygnał poniżej progu, nie ściskamy
    float gain = 1.0f;

    if (envelope > threshold) {
        // kompresja w dB: każdy dB ponad progiem -> 1/ratio dB na wyjściu,
        // czyli redukcja (1 - 1/ratio) dB na każdy dB nadmiaru.
        // W skali liniowej to samo: gain = (threshold / envelope)^(1 - 1/ratio)
        gain = powf(threshold / envelope, slope);
    }

    return x * gain;
}

std::string Compressor::getName() {
    return "Compressor";
}

void Compressor::scalePotValues(int pot0, int pot1, int pot2) {

    float thresholdNorm = pot0 / 100.0f;
    float ratioNorm = pot1 / 100.0f;
    float makeupNorm = pot2 / 100.0f;

    // pot0 większy = mocniej łapie sygnał
    threshold = 0.35f - thresholdNorm * 0.29f;
    // zakres: 0.35 -> 0.06

    
    ratio = 2.0f + ratioNorm * 8.0f;
    // zakres: 2.0 -> 10.0
    slope = 1.0f - 1.0f / ratio;

    
    makeupGain = 0.9f + makeupNorm * 2.6f;
  
    attackCoeff = 0.02f;      // ~1 ms przy 48 kHz

    
    releaseCoeff = 0.0002f;   // ~100 ms
}

std::string Compressor::getParamName(int idx) {
    switch (idx) {
    case 0: return "treshold";
    case 1: return "ratio";
    case 2: return "makeupGain";
    default: return "?";
    }
}