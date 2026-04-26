#pragma once
#include <string>

class Effect
{
public:
#define sampleRate       44100

	virtual float process(float sample) = 0;
	virtual std::string getName() = 0;
};

