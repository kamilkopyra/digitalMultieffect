#pragma once
#include "Effect.h"
#include <vector>
#include <map>
#include <string>
#include <array>
#include <atomic>
#include <utility>
#include <cstdint>

// Łańcuch efektów — od 3 (domyślnie) do 6 slotów (odpowiadają sekcjom
// ekranu w GUI; 3 fizyczne enkodery obsługują dowolną liczbę slotów przez
// przełączanie fokusu). Sygnał przechodzi przez aktywne sloty po kolei
// 0 -> 1 -> ... (pusty slot jest pomijany). Sam jest efektem (wzorzec
// kompozyt), więc AudioEngine traktuje go tak jak pojedynczy efekt.
//
// Jeden slot jest "wybrany" (focused) — to jego parametry edytują enkodery
// i klawiatura, i do niego trafia tap tempo. GUI może edytować dowolny slot
// bezpośrednio (setSlotParam), bo ma miejsce żeby pokazać wszystkie naraz.
//
// Wątkowość: `slotArray` to std::atomic<Effect*>, żeby wątek audio (process())
// zawsze widział albo stary, albo nowy, w pełni skonstruowany wskaźnik —
// nigdy połowicznie zainicjalizowany obiekt. Usuwanie starego efektu jest
// odroczone (graveyard) do następnej zmiany slotu, żeby nie skasować
// obiektu, gdy wątek audio może być akurat w środku process() na nim.
// `count` (ile slotów jest aktywnych) jest z tego samego powodu atomowe.
class EffectChain : public Effect
{
public:
	static constexpr int minSlots = 3;   // domyślna / minimalna liczba slotów
	static constexpr int maxSlots = 6;   // maksymalna liczba slotów

	EffectChain() = default;
	~EffectChain() override;

	float process(float sample) override;
	std::string getName() override;
	std::string getParamName(int idx) override;
	std::array<int, 3> getDefaultParams() override { return { 0, 0, 0 }; }

	// modulacja / tap tempo — delegowane do wybranego slotu
	bool isModulation() override;
	void TapToParam(float ms) override;

	// --- zarządzanie slotami (właścicielem wskaźników jest EffectChain) ---
	// Podmienia zawartość slotu (odracza usunięcie poprzedniego efektu).
	// effect == nullptr czyści slot.
	void setSlot(int index, Effect* effect);
	// Jak setSlot, ale jeśli w slocie jest już efekt tego samego typu —
	// usuwa go zamiast podmieniać (wygodne pod klawiaturę: ten sam klawisz
	// dodaje/usuwa). GUI (dropdown) używa zwykłego setSlot.
	void toggleInSlot(int index, Effect* effect);
	void clearSlot(int index);
	void clear();   // czyści wszystkie aktywne sloty
	Effect* slot(int index) const;   // surowy wskaźnik, nullptr = pusty slot

	// liczba aktywnych slotów (3-6) i jej zmiana
	int slotCount() const { return count.load(std::memory_order_acquire); }
	bool addSlot();      // +1 slot, do maxSlots; false jeśli już na maksimum
	bool removeSlot();   // usuwa (czyści) ostatni slot, do minSlots; false jeśli już na minimum

	// który slot edytują enkodery/klawiatura
	void focusNext();
	void setFocus(int index);
	Effect* focused();
	int focusedIndex() const { return focusIndex; }

	// zmiana parametru wybranego slotu (delta z enkodera)
	void adjustFocusedParam(int idx, int delta);
	// bezpośrednia zmiana parametru dowolnego slotu (GUI — suwak w danej sekcji)
	void setSlotParam(int slotIndex, int paramIdx, int value);

	std::string describe();   // np. "[Fuzz] | Delay | -" — do konsoli

	// --- statystyki obciążenia CPU (mierzone na wątku audio, odczyt bezpieczny
	// z dowolnego wątku) — wygładzony (EMA) czas ostatnich pomiarów w mikrosekundach.
	// Mierzone co measureEvery próbek, żeby sam pomiar nie dokładał znaczącego narzutu.
	float slotCpuMicros(int index) const;
	float totalCpuMicros() const;
	float cpuLoadPercent() const;   // totalCpuMicros() / budżet-czasowy-na-próbkę * 100

	// strukturalny opis łańcucha — dokładnie slotCount() elementów,
	// puste sloty oznaczone `empty`. Do zbudowania UI (np. modelu QML).
	struct EffectInfo {
		std::string name;   // "" jeśli slot pusty
		int params[3];
		std::string paramNames[3];
		bool focused;
		bool empty;
	};
	std::vector<EffectInfo> listEffects() const;

private:
	std::atomic<Effect*> slotArray[maxSlots] {};
	std::atomic<int> count{ minSlots };
	std::vector<Effect*> graveyard;   // efekty wyjęte ze slotu, czekające na bezpieczne usunięcie
	int focusIndex = 0;

	static constexpr int measureEvery = 32;   // mierz co 32. próbkę (ograniczenie narzutu pomiaru)
	std::atomic<uint32_t> sampleCounter{ 0 };
	std::atomic<float> slotMicros[maxSlots] {};
	std::atomic<float> totalMicros{ 0.0f };

	// klucz: (slot, nazwa efektu) — nie samo (nazwa), bo dwa różne sloty
	// mogą trzymać ten sam typ efektu niezależnie od siebie
	std::map<std::pair<int, std::string>, std::array<int, 3>> savedParams;

	void collectGarbage();   // usuwa efekty odłożone przy POPRZEDNIEJ zmianie slotu
};
