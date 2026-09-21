#pragma once
#include <string>
#include <array>

// Stała częstotliwość próbkowania całego projektu. `inline constexpr` (C++17)
// daje jeden, prawdziwie typowany egzemplarz widoczny we wszystkich plikach
// dołączających ten nagłówek — bez ryzyka niezgodnych #define w różnych
// miejscach (tak jak było wcześniej: Effect.h i AudioEngine.h definiowały
// identyczne makro osobno).
inline constexpr int sampleRate = 48000;

class Effect
{
public:
	virtual ~Effect() {}

	virtual bool isModulation() { return false; }  //domyślnie fałsz
	virtual void TapToParam(float ms) {}  // domyślnie brak implementacji

	virtual float process(float sample) = 0;
	virtual std::string getName() = 0;

	virtual std::string getParamName(int idx) = 0;
	virtual std::array<int, 3> getDefaultParams() = 0;

	// Sterowanie parametrami (enkodery / klawiatura).
	// Każdy efekt trzyma własne 3 parametry, dzięki czemu w łańcuchu
	// efekty nie współdzielą jednego zestawu potencjometrów.
	void setParam(int idx, int value) { if (idx >= 0 && idx < 3) ownParams[idx] = value; }
	int  getParam(int idx) const { return (idx >= 0 && idx < 3) ? ownParams[idx] : 0; }
	void loadDefaultParams() {
		auto d = getDefaultParams();
		ownParams[0] = d[0]; ownParams[1] = d[1]; ownParams[2] = d[2];
	}

	// zgodność wstecz — można wpiąć zewnętrzną tablicę potów
	void setPot(int* p) { pot = p; }

protected:
	int ownParams[3] = { 0, 0, 0 };
	int* pot = ownParams;   // efekty czytają pot[0..2] — domyślnie własne parametry
};
