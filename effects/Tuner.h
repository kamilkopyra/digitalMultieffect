#pragma once
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include "Effect.h"
#include <thread>
#include <mutex>
#include <memory>



#define WINDOW_SIZE 2048

// Wynik ostatniego strojenia — bezpieczny do odczytu z dowolnego wątku
// (np. GUI odpytujące Tunera co ~50ms przez Tuner::getReading()).
struct TunerReading {
    float freq = 0.0f;
    float target = 0.0f;
    float cents = 0.0f;
    std::string stringName;
    bool inTune = false;
    bool valid = false;   // false dopóki nic sensownego nie wykryto
};

class Tuner : public Effect {
public:

    float process(float sample) override;
    std::string getName() override;
    struct GuitarString {
        std::string name;
        double frequency;
    };

    std::string getParamName(int idx) override;
    std::array<int, 3> getDefaultParams() override { return { 100, 80, 100 }; }

    // bezpieczne wątkowo — wołaj z GUI/dowolnego wątku, żeby odczytać
    // ostatni wynik strojenia bez ryzyka wyścigu z wątkiem analizy
    TunerReading getReading() const;

private:
    // Analiza (McLeod) leci w oddzielnym, odłączonym wątku, żeby nie
    // zaśmiecać toru audio. Ten wątek NIE trzyma wskaźnika na Tuner (`this`)
    // — dostaje tylko współdzieloną kopię tego stanu (shared_ptr). Dzięki
    // temu usunięcie efektu (np. zmiana slotu w EffectChain) w trakcie
    // trwania analizy jest bezpieczne: stan po prostu przeżyje Tunera,
    // zamiast być use-after-free (to był realny powód "czasami się crashuje").
    struct SharedState {
        std::mutex mutex;
        std::vector<float> freqHistory;
        TunerReading lastReading;
    };
    std::shared_ptr<SharedState> state = std::make_shared<SharedState>();

    struct Candidate {
        int tau;
        float freq;
        float value;
    };

    std::vector<float> fftBuffer;

    static float getClosestTarget(float maxFreq);
    static std::string getClosestName(float maxFreq);
    static void analyze(std::shared_ptr<SharedState> state, std::vector<float> buffer);
    inline float median3(float a, float b, float c);

};
