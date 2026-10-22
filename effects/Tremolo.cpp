#include "Tremolo.h"
#include <algorithm>


float Tremolo::process(float sample) {

	float modified = tremoloEffect(sample);
    return modified;
}

float Tremolo::tremoloEffect(float x) {
	return x * lfoWave(depth);
}

std::string Tremolo::getName() {
    return "Tremolo";
}

float Tremolo::lfoWave(float depth) {
	phase += phaseInc;
	if (phase > 2.0f * pi) phase -= 2.0f * pi;

	float lfo = (sinf(phase) + 1.0f) * 0.5f; // [0, 1]
	return 1.0f - depth * lfo;               // [1-depth, 1]

}

void Tremolo::scalePotValues(int pot0, int pot1) {

	float frequency = (pot0 / 100.0f) * 10.0f; // Skaluje pot0 do zakresu 0-10 Hz
	phaseInc = 2.0f * pi * frequency / sampleRate;
	depth = (pot1 / 100.0f); 

}

std::string Tremolo::getParamName(int idx) {
	switch (idx) {
	case 0: return "frequency";
	case 1: return "depth";
	case 2: return "None";
	default: return "?";
	}
}


void Tremolo::TapToParam(float ms) {
	float hz = 1000.0f / ms;  // ms na Hz
	pot[0] = std::clamp((int)(hz / 10.0f * 100.0f), 0, 100);
	onParamChange();
}
