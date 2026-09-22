#include "EffectChain.h"
#include <algorithm>
#include <chrono>

EffectChain::~EffectChain() {
	for (int i = 0; i < maxSlots; ++i)
		delete slotArray[i].load(std::memory_order_relaxed);
	for (Effect* e : graveyard)
		delete e;
}

void EffectChain::collectGarbage() {
	for (Effect* e : graveyard)
		delete e;
	graveyard.clear();
}

float EffectChain::process(float sample) {
	float out = sample;
	int n = count.load(std::memory_order_acquire);

	// mierz tylko co measureEvery próbek — sam std::chrono::now() ma swój
	// koszt, a przy 48kHz x do 6 slotów mierzenie każdej próbki dokładałoby
	// zauważalny narzut do samego pomiaru, który miał pokazywać narzut efektów
	bool measure = (sampleCounter.fetch_add(1, std::memory_order_relaxed) % measureEvery) == 0;
	float sumMicros = 0.0f;

	for (int i = 0; i < n; ++i) {
		Effect* e = slotArray[i].load(std::memory_order_acquire);
		if (!e) continue;

		if (measure) {
			auto t0 = std::chrono::high_resolution_clock::now();
			out = e->process(out);
			auto t1 = std::chrono::high_resolution_clock::now();

			float micros = std::chrono::duration<float, std::micro>(t1 - t0).count();
			sumMicros += micros;

			// wygładzenie (EMA) — surowy pomiar jednej próbki jest szumiący
			float prev = slotMicros[i].load(std::memory_order_relaxed);
			slotMicros[i].store(prev + 0.1f * (micros - prev), std::memory_order_relaxed);
		}
		else {
			out = e->process(out);
		}
	}

	if (measure) {
		float prevTotal = totalMicros.load(std::memory_order_relaxed);
		totalMicros.store(prevTotal + 0.1f * (sumMicros - prevTotal), std::memory_order_relaxed);
	}

	return out;
}

float EffectChain::slotCpuMicros(int index) const {
	if (index < 0 || index >= maxSlots) return 0.0f;
	return slotMicros[index].load(std::memory_order_relaxed);
}

float EffectChain::totalCpuMicros() const {
	return totalMicros.load(std::memory_order_relaxed);
}

float EffectChain::cpuLoadPercent() const {
	// budżet czasu na jedną próbkę przy danym sampleRate, w mikrosekundach
	const float budgetMicros = 1'000'000.0f / static_cast<float>(sampleRate);
	return (totalCpuMicros() / budgetMicros) * 100.0f;
}

void EffectChain::setSlot(int index, Effect* effect) {
	if (index < 0 || index >= slotCount()) {
		delete effect;
		return;
	}

	// dopiero teraz bezpiecznie skasuj to, co zostało wyjęte przy POPRZEDNIM
	// wywołaniu — wątek audio na pewno zdążył już przejść dalej niż wtedy
	collectGarbage();

	Effect* old = slotArray[index].load(std::memory_order_relaxed);
	if (old) {
		savedParams[{index, old->getName()}] = { old->getParam(0), old->getParam(1), old->getParam(2) };
	}

	if (effect) {
		auto saved = savedParams.find({ index, effect->getName() });
		if (saved != savedParams.end()) {
			effect->setParam(0, saved->second[0]);
			effect->setParam(1, saved->second[1]);
			effect->setParam(2, saved->second[2]);
		}
		else {
			effect->loadDefaultParams();
		}
	}

	// zeruj statystyki CPU tego slotu — inaczej przez chwilę pokazywałby
	// koszt poprzedniego efektu, dopóki EMA się nie przestawi
	slotMicros[index].store(0.0f, std::memory_order_relaxed);

	// publikacja nowego wskaźnika — release, żeby wątek audio (acquire w
	// process()) widział w pełni skonfigurowany efekt, nigdy połowiczny
	slotArray[index].store(effect, std::memory_order_release);

	if (old)
		graveyard.push_back(old);   // NIE delete teraz — dopiero przy następnej zmianie
}

void EffectChain::toggleInSlot(int index, Effect* effect) {
	if (index < 0 || index >= slotCount()) {
		delete effect;
		return;
	}

	Effect* current = slot(index);
	if (current && effect && current->getName() == effect->getName()) {
		// ten sam efekt już tu jest — usuń go (klawisz działa jak przełącznik)
		delete effect;
		clearSlot(index);
		return;
	}

	setSlot(index, effect);
}

void EffectChain::clearSlot(int index) {
	setSlot(index, nullptr);
}

void EffectChain::clear() {
	for (int i = 0; i < slotCount(); ++i)
		clearSlot(i);
}

Effect* EffectChain::slot(int index) const {
	if (index < 0 || index >= maxSlots) return nullptr;
	return slotArray[index].load(std::memory_order_acquire);
}

bool EffectChain::addSlot() {
	int c = count.load(std::memory_order_relaxed);
	if (c >= maxSlots) return false;
	count.store(c + 1, std::memory_order_release);
	return true;
}

bool EffectChain::removeSlot() {
	int c = count.load(std::memory_order_relaxed);
	if (c <= minSlots) return false;

	// najpierw skróć zakres, którego dotyka wątek audio (process()) —
	// dopiero potem bezpiecznie zwolnij to, co wypada z łańcucha
	count.store(c - 1, std::memory_order_release);
	clearSlot(c - 1);

	if (focusIndex >= c - 1)
		focusIndex = c - 2 >= 0 ? c - 2 : 0;

	return true;
}

void EffectChain::focusNext() {
	focusIndex = (focusIndex + 1) % slotCount();
}

void EffectChain::setFocus(int index) {
	if (index < 0 || index >= slotCount()) return;
	focusIndex = index;
}

Effect* EffectChain::focused() {
	return slot(focusIndex);
}

void EffectChain::adjustFocusedParam(int idx, int delta) {
	Effect* e = focused();
	if (!e) return;
	int v = std::clamp(e->getParam(idx) + delta, 0, 100);
	e->setParam(idx, v);
}

void EffectChain::setSlotParam(int slotIndex, int paramIdx, int value) {
	Effect* e = slot(slotIndex);
	if (!e) return;
	e->setParam(paramIdx, std::clamp(value, 0, 100));
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
	std::string s;
	for (int i = 0; i < slotCount(); ++i) {
		Effect* e = slot(i);
		if (!e) continue;
		if (!s.empty()) s += "_";
		s += e->getName();
	}
	return s.empty() ? "Chain" : s;
}

std::string EffectChain::describe() {
	std::string s;
	for (int i = 0; i < slotCount(); ++i) {
		if (i) s += " | ";
		Effect* e = slot(i);
		std::string label = e ? e->getName() : "-";
		s += (i == focusIndex) ? ("[" + label + "]") : label;
	}
	return s;
}

std::vector<EffectChain::EffectInfo> EffectChain::listEffects() const {
	std::vector<EffectInfo> result;
	int n = slotCount();
	result.reserve(n);
	for (int i = 0; i < n; ++i) {
		Effect* e = slot(i);
		EffectInfo info;
		info.empty = (e == nullptr);
		info.name = e ? e->getName() : "";
		info.focused = (i == focusIndex);
		for (int p = 0; p < 3; ++p) {
			info.params[p] = e ? e->getParam(p) : 0;
			info.paramNames[p] = e ? e->getParamName(p) : "";
		}
		result.push_back(info);
	}
	return result;
}
