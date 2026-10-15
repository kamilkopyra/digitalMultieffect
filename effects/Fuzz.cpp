#include "Fuzz.h"

float Fuzz::process(float sample) {

    scalePotValues(pot[0], pot[1], pot[2]);
    float driven = sample * gain;
    float clipped = hardClipping(driven);
	
    return clipped * volume; 
}


float Fuzz::hardClipping(float x) {
    if (x > clipRange) return clipRange;
    if (x < -clipRange) return -clipRange;
    return x;
}

std:: string Fuzz::getName() {
    return "Fuzz";
}

void Fuzz::scalePotValues(int pot0, int pot1, int pot2) {
    gain = (pot0 / 100.0f) * 10.0f; 
    volume = (pot1 / 100.0f)*2; 
    clipRange = 0.32f - (0.01f + (pot2 / 100.0f) * 0.3f);
}

std::string Fuzz::getParamName(int idx) {
    switch (idx) {
    case 0: return "Gain";
    case 1: return "Volume";
    case 2: return "Clip";
    default: return "?";
    }
}

