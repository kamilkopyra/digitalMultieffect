#include "EffectChain.h"

void EffectChain::add(Effect* effect) {
	if (!effect || count >= maxEffects) return;

	effect->loadDefaultParams();
	pool[count++] = effect;
}

float EffectChain::process(float sample) {
	float out = sample;
	for (int s = 0; s < numSlots; ++s) {
		int idx = slotIdx[s];   // jeden odczyt — slot może się zmienić z pętli głównej
		if (idx >= 0 && enabled[s])
			out = pool[idx]->process(out);
	}
	return out;
}

bool EffectChain::setSlot(int slot, int effectIndex) {
	if (slot < 0 || slot >= numSlots) return false;
	if (effectIndex < -1 || effectIndex >= count) return false;

	// ten sam obiekt nie może grać w dwóch slotach naraz
	if (effectIndex >= 0)
		for (int s = 0; s < numSlots; ++s)
			if (s != slot && slotIdx[s] == effectIndex)
				return false;

	slotIdx[slot] = effectIndex;
	return true;
}

int EffectChain::slotEffect(int slot) const {
	return (slot >= 0 && slot < numSlots) ? slotIdx[slot] : -1;
}

Effect* EffectChain::slot(int slot) {
	int idx = slotEffect(slot);
	return (idx >= 0 && idx < count) ? pool[idx] : nullptr;
}

void EffectChain::setEnabled(int slot, bool on) {
	if (slot >= 0 && slot < numSlots)
		enabled[slot] = on;
}

bool EffectChain::isEnabled(int slot) const {
	return slot >= 0 && slot < numSlots && enabled[slot];
}
