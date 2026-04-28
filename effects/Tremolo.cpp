#include "Tremolo.h"


float Tremolo::process(float sample) {

	float modified = tremoloEffect(sample);
    return modified;
}


float Tremolo::tremoloEffect(float x) {
	return x * scalePotValues(pot[0], pot[1]);
}

std::string Tremolo::getName() {
    return "Tremolo";
}

float Tremolo::lfoWave(float frequency, float depth) {
	phase += 2.0f * pi * frequency / sampleRate;
	if (phase > 2.0f * pi) phase -= 2.0f * pi;

	float lfo = (sinf(phase) + 1.0f) * 0.5f; // [0, 1]
	return 1.0f - depth * lfo;               // [1-depth, 1]

}

float Tremolo::scalePotValues(int pot0, int pot1) {

	float frequency = (pot0 / 1023.0f) * 10.0f; // Skaluje pot0 do zakresu 0-10 Hz
	float depth = (pot1 / 1023.0f); 
	return lfoWave(frequency, depth);

}

