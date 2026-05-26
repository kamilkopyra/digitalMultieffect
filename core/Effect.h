#pragma once
#include <string>
#include <array>

class Effect
{
public:
#define sampleRate       48000


	virtual bool isModulation() { return false; }  //domyślnie fałsz
	virtual void TapToParam(float ms) {}  // domyślnie brak implementacji

	virtual float process(float sample) = 0;
	virtual std::string getName() = 0;
	void setPot(int* p) { pot = p; }

	virtual std::string getParamName(int idx) = 0;
	virtual std::array<int, 3> getDefaultParams() = 0;

protected:
	int* pot=nullptr;
};

