#include "NoiseGate.h"


float NoiseGate::process(float sample) {

	static float buffer[64];
	static int idx = 0;
	//static int printIdx = 0;
	buffer[idx++] = sample;

	if (idx >= FramesPerBuffer) {
		idx = 0;
		rms = calcRMS(buffer);
	}
	
	//printIdx++;
	//if (printIdx == 1000) 
	//{
	//	printIdx = 0;
	//	std::cout << "RMS: " << rms << std::endl;
	//}
	
	float modified = noiseGateEffect(sample);
	return modified;
}


////float NoiseGate::noiseGateEffect(float x) {
//	if (threshold > rms) {
//		return 0.0f; 
//	}
//	return x;
//}

float NoiseGate::noiseGateEffect(float x) {
	// 1. Wyznacz docelowy gain na podstawie RMS i threshold
	float target_gain = (rms > threshold) ? 1.0f : 0.0f;

	// 2. Płynnie dąż do target_gain (attack jak otwieramy, release jak zamykamy)
	if (target_gain > current_gain) {
		current_gain += attack_coeff;
	}
	else {
		current_gain -= release_coeff;
	}

	// 3. Klamp żeby nie wyjechało poza [0, 1]
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
	attack_coeff = 0.0005f + (pot1 / 1023.0f) * 0.05f;  
	release_coeff = 0.00005f + (pot2 / 1023.0f) * 0.005f; 
}
