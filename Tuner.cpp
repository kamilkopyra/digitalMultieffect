#include "Tuner.h"


float Tuner::process(float sample) {

	float modified = tuner(sample);
	return modified;
}


float Tuner::tuner(float x) {
	return x * (0.5f * (1.0f + sinf(2.0f * 3.14159f * 5.0f * x / sampleRate)));
}

std::string Tuner::getName() {
	return "Tuner";
}
