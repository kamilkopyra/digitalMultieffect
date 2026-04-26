#include "Fuzz.h"

float Fuzz::process(float sample) {

    float clipped = hardClipping(sample) * (1.0f / clipRange);
    //float driven = softClipping(sample, 5.0f);

    return clipped;

}


float Fuzz::hardClipping(float x) {
    if (x > clipRange) return clipRange;
    if (x < -clipRange) return -clipRange;
    return x;
}

float Fuzz::softClipping(float x, float drive) {
    return tanhf(x * drive) / tanhf(drive);
}

std:: string Fuzz::getName() {
    return "Fuzz";
}

