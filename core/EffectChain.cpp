#include "EffectChain.h"
#include <algorithm>

EffectChain::~EffectChain() {
	for (Effect* e : chain)
		delete e;
	chain.clear();
}

int EffectChain::indexByName(const std::string& name) const {
	for (int i = 0; i < static_cast<int>(chain.size()); ++i)
		if (chain[i]->getName() == name)
			return i;
	return -1;
}

float EffectChain::process(float sample) {
	float out = sample;
	for (Effect* e : chain)
		out = e->process(out);
	return out;
}

void EffectChain::toggle(Effect* effect) {
	if (!effect) return;

	const std::string name = effect->getName();
	const int existing = indexByName(name);

	if (existing >= 0) {
		// efekt już jest w łańcuchu — usuwamy go
		savedParams[name] = { chain[existing]->getParam(0),
							  chain[existing]->getParam(1),
							  chain[existing]->getParam(2) };
		delete chain[existing];
		chain.erase(chain.begin() + existing);
		delete effect;  // przekazany wskaźnik nie jest potrzebny

		if (chain.empty())
			focusIndex = -1;
		else {
			if (existing < focusIndex) focusIndex--;                 // przesuń za usuniętym
			if (focusIndex >= static_cast<int>(chain.size()))
				focusIndex = static_cast<int>(chain.size()) - 1;
			if (focusIndex < 0) focusIndex = 0;
		}
		return;
	}

	// nowy efekt — na koniec łańcucha
	auto saved = savedParams.find(name);
	if (saved != savedParams.end()) {
		effect->setParam(0, saved->second[0]);
		effect->setParam(1, saved->second[1]);
		effect->setParam(2, saved->second[2]);
	}
	else {
		effect->loadDefaultParams();
	}

	chain.push_back(effect);
	focusIndex = static_cast<int>(chain.size()) - 1;  // świeżo dodany = wybrany
}

void EffectChain::clear() {
	for (Effect* e : chain) {
		savedParams[e->getName()] = { e->getParam(0), e->getParam(1), e->getParam(2) };
		delete e;
	}
	chain.clear();
	focusIndex = -1;
}

void EffectChain::focusNext() {
	if (chain.empty()) {
		focusIndex = -1;
		return;
	}
	focusIndex = (focusIndex + 1) % static_cast<int>(chain.size());
}

Effect* EffectChain::focused() {
	if (focusIndex < 0 || focusIndex >= static_cast<int>(chain.size()))
		return nullptr;
	return chain[focusIndex];
}

void EffectChain::adjustFocusedParam(int idx, int delta) {
	Effect* e = focused();
	if (!e) return;
	int v = std::clamp(e->getParam(idx) + delta, 0, 100);
	e->setParam(idx, v);
}

bool EffectChain::isModulation() {
	Effect* e = focused();
	return e && e->isModulation();
}

void EffectChain::TapToParam(float ms) {
	Effect* e = focused();
	if (e && e->isModulation())
		e->TapToParam(ms);
}

std::string EffectChain::getParamName(int idx) {
	Effect* e = focused();
	return e ? e->getParamName(idx) : "-";
}

std::string EffectChain::getName() {
	if (chain.empty())
		return "Chain";
	std::string s;
	for (size_t i = 0; i < chain.size(); ++i) {
		if (i) s += "_";
		s += chain[i]->getName();
	}
	return s;
}

std::string EffectChain::describe() {
	if (chain.empty())
		return "(łańcuch pusty)";
	std::string s;
	for (int i = 0; i < static_cast<int>(chain.size()); ++i) {
		if (i) s += " > ";
		if (i == focusIndex) s += "[" + chain[i]->getName() + "]";
		else                 s += chain[i]->getName();
	}
	return s;
}
