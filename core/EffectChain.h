#pragma once
#include "Effect.h"
#include <vector>
#include <map>
#include <string>
#include <array>

// Łańcuch efektów.
//
// Sam jest efektem (wzorzec kompozyt), więc AudioEngine traktuje go tak
// jak pojedynczy efekt. Sygnał przechodzi kolejno przez wszystkie efekty
// w łańcuchu. Jeden efekt jest "wybrany" (focused) — to jego parametry
// edytują enkodery, i do niego trafia tap tempo.
class EffectChain : public Effect
{
public:
	~EffectChain() override;

	float process(float sample) override;
	std::string getName() override;
	std::string getParamName(int idx) override;
	std::array<int, 3> getDefaultParams() override { return { 0, 0, 0 }; }

	// modulacja / tap tempo — delegowane do wybranego efektu
	bool isModulation() override;
	void TapToParam(float ms) override;

	// --- zarządzanie łańcuchem (właścicielem wskaźników jest EffectChain) ---
	// Dodaje efekt, jeśli efektu o tej nazwie jeszcze nie ma; w przeciwnym
	// razie usuwa istniejący. Przejmuje własność przekazanego wskaźnika.
	void toggle(Effect* effect);
	void clear();

	// przełączenie wybranego (edytowanego) efektu na kolejny w łańcuchu
	void focusNext();
	Effect* focused();

	// zmiana parametru wybranego efektu (delta z enkodera)
	void adjustFocusedParam(int idx, int delta);

	int  size() const { return static_cast<int>(chain.size()); }
	std::string describe();   // np. "Fuzz > [Phaser] > Delay" — do konsoli

private:
	std::vector<Effect*> chain;
	int focusIndex = -1;
	std::map<std::string, std::array<int, 3>> savedParams;  // pamięć parametrów po nazwie

	int indexByName(const std::string& name) const;
};
