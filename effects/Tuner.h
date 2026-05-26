#pragma once
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include "Effect.h"
#include <thread>
#include <mutex>



#define WINDOW_SIZE 2048


class Tuner : public Effect {
public:


    std::mutex printMutex;
    float process(float sample) override;
    std::string getName() override;
    struct GuitarString {
        std::string name;
        double frequency;
    };

    std::string getParamName(int idx) override;
    std::array<int, 3> getDefaultParams() override { return { 100, 80, 100 }; }
private:

    std::mutex historyMutex;
    struct Candidate {
        int tau;
        float freq;
        float value;
    };

    std::vector<float> fftBuffer;
    std::vector<float> freqHistory;

    float getClosestTarget(float maxFreq);
    std::string getClosestName(float maxFreq);
    void McLeod_Method(std::vector<float> buffer);
    void printTuner(float freq, float target, float cents, const std::string& stringName);
    inline float median3(float a, float b, float c);

};