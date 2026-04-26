#include "NoiseGate.h"


float NoiseGate::process(float sample) {

	static float buffer[64];
	static int idx = 0;
	static int printIdx = 0;
	buffer[idx++] = sample;

	if (idx >= FramesPerBuffer) {
		idx = 0;
		rms = calcRMS(buffer);
	}
	
	printIdx++;
	if (printIdx == 1000) 
	{
		printIdx = 0;
		std::cout << "RMS: " << rms << std::endl;
	}
	
	float modified = noiseGateEffect(sample);
	return modified;
}


float NoiseGate::noiseGateEffect(float x) {
	if (threshold > rms) {
		return 0.0f; 
	}
	return x;
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
