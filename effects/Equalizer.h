#pragma once
#include "Effect.h"
#include <cmath>

// Equalizer — 3-pasmowy korektor barwy zbudowany z filtrów biquad
// (wzory RBJ Audio EQ Cookbook):
//   - dolna półka  ~120 Hz
//   - środek (peak) ~1 kHz, Q ~ 0.9
//   - górna półka  ~4 kHz
// Każdy potencjometr: 0 = -12 dB, 50 = płasko, 100 = +12 dB.
class Equalizer : public Effect
{
public:
    float process(float sample) override;
    std::string getName() override;
    std::string getParamName(int idx) override;
    std::array<int, 3> getDefaultParams() override { return { 50, 50, 50 }; }

private:
    struct Biquad {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        float process(float x) {
            float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x;
            y2 = y1; y1 = y;
            return y;
        }
    };

    Biquad low, mid, high;
    int lastPot[3] = { -1, -1, -1 };

    void recalcIfNeeded(int pot0, int pot1, int pot2);
    static void makeLowShelf(Biquad& bq, float f0, float dBgain);
    static void makeHighShelf(Biquad& bq, float f0, float dBgain);
    static void makePeak(Biquad& bq, float f0, float q, float dBgain);
    static float potToDb(int pot);
};
