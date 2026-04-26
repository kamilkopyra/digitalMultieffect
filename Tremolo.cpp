#include "Tremolo.h"


float Tremolo::process(float sample) {

	float modified = tremoloEffect(sample);
    return modified;
}


float Tremolo::tremoloEffect(float x) {
	return x * lfoWave(0.2f, 0.5f);
}

std::string Tremolo::getName() {
    return "Tremolo";
}

float Tremolo::lfoWave(float frequency, float depth) {
	phase += 2.0f * pi * frequency / sampleRate;
	return 0.5f + sinf(2.0f * pi * phase) * depth;
}

