#pragma once
#include <string>

class Effect
{
public:
#define sampleRate       44100


	virtual float process(float sample) = 0;
	virtual std::string getName() = 0;
	void setPot(int* p) { pot = p; }

protected:
	int* pot=nullptr;
};

