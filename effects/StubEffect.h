#pragma once
#include "Effect.h"

// Wydmuszka — efekt bez przetwarzania, sygnał przechodzi bez zmian.
// Zastępuje na Daisy efekty jeszcze nieprzeniesione (Delay — bufor do SDRAM,
// Tuner — wątki i mutexy), żeby łańcuch miał pełną listę efektów.
// Oryginalne pliki Delay.* i Tuner.* zostają nietknięte.
class StubEffect : public Effect
{
public:
	StubEffect(const char* name, const char* p0, const char* p1, const char* p2)
		: name(name), paramNames{ p0, p1, p2 } {}

	float process(float sample) override { return sample; }
	std::string getName() override { return name; }
	std::string getParamName(int idx) override { return (idx >= 0 && idx < 3) ? paramNames[idx] : "?"; }
	std::array<int, 3> getDefaultParams() override { return { 0, 0, 0 }; }

private:
	const char* name;
	const char* paramNames[3];
};
