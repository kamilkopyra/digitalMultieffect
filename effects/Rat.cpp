#include "Rat.h"

float Rat::process(float sample) {

    scalePotValues(pot[0], pot[1], pot[2]);
	float clipped = softClipping(sample, drive);
    filtered = filtered + tone * (clipped - filtered); // filtr dolnoprzepustowy
    return filtered * volume;
}


float Rat::softClipping(float x, float drive) {
    return tanhf(x * drive);
}

std::string Rat::getName() {
    return "Rat";
}

void Rat::scalePotValues(int pot0, int pot1, int pot2) {
    drive =  (1 + (pot0 / 1023.0f) * 60.0f);
    volume = (pot1 / 1023.0f) /10;
    //tone = 0.05f + (pot2 / 1023.0f) * 0.4f;  // 0.05-0.45
    tone = 0.01f + (pot2 / 1023.0f) * 0.89f;
}

