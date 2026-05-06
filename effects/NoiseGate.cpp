#include "NoiseGate.h"


float NoiseGate::process(float sample) {
	scalePotValues(pot[0], pot[1], pot[2]);

	// płynny envelope follower zamiast RMS na blokach
	float level = fabsf(sample);
	rms = rms + 0.001f * (level - rms);

	float modified = noiseGateEffect(sample);
	return modified;
}


float NoiseGate::noiseGateEffect(float x) {
	float target_gain;
	if (current_gain < 0.5f) {
		// gate zamknięty — otwórz tylko jak sygnał wyraźnie powyżej progu
		target_gain = (rms > threshold) ? 1.0f : 0.0f;
	}
	else {
		// gate otwarty — zamknij dopiero jak sygnał spadnie znacznie poniżej
		target_gain = (rms > threshold * 0.5f) ? 1.0f : 0.0f;
	}

	if (target_gain > current_gain) {
		current_gain += attack_coeff;
	}
	else {
		current_gain -= release_coeff;
	}

	if (current_gain < 0.0f) current_gain = 0.0f;
	if (current_gain > 1.0f) current_gain = 1.0f;

	return x * current_gain;
}
std::string NoiseGate::getName() {
	return "NoiseGate";
}

float NoiseGate::calcRMS(float buffer[]) {
	float sum = 0.0f;
	for (int i = 0; i < FramesPerBuffer; ++i) {
		sum += buffer[i] * buffer[i];
	}
	return sqrtf(sum / FramesPerBuffer);
}

void NoiseGate::scalePotValues(int pot0, int pot1, int pot2) {
	threshold = (pot0 / 1023.0f) /5 ; 
	attack_coeff = 0.0005f + (pot1 / 1023.0f) * 0.005f;  
	release_coeff = 0.00005f + (pot1 / 1023.0f) * 0.005f; 
}
