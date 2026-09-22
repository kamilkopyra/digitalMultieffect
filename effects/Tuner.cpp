#include "Tuner.h"

// Błędy:
// problem z wykrywaniem - czasami wykrywana jest harmoniczna zamiast częstotliwości podstawowek
// problem z czułością - czasami wykrywana jest częstotliwość, ale jest ona bardzo niestabilna
//
// problem z wielowątkowością ("czasami się crashuje") naprawiony: wątek
// analizy dostawał surowe `this` i mógł dostać się do usuniętego już Tunera
// (np. po zmianie slotu w EffectChain). Teraz dostaje tylko shared_ptr do
// współdzielonego stanu (SharedState) — ten stan przeżywa Tunera, więc
// usunięcie efektu w trakcie trwania analizy jest bezpieczne.


float Tuner::process(float sample) {

    fftBuffer.push_back(sample);
    if (fftBuffer.size() >= WINDOW_SIZE) {
        std::vector<float> bufferCopy = fftBuffer;
        std::thread t(&Tuner::analyze, state, std::move(bufferCopy)); // nowy wątek żeby nie zaśmiecać toru audio
        t.detach();
        if (fftBuffer.size() > WINDOW_SIZE * 0.8) {
            fftBuffer.erase(fftBuffer.begin(), fftBuffer.end() - (size_t)(WINDOW_SIZE * 0.8));
        }
    }
    return sample;
}

std::string Tuner::getName() {
	return "Tuner";
}

inline float Tuner::median3(float a, float b, float c)
{
    return std::max(std::min(a, b), std::min(std::max(a, b), c));
}

static std::vector<Tuner::GuitarString> guitarStrings = {
        {"E4", 329.63},
        {"B3", 246.94},
        {"G3", 196.00},
        {"D3", 146.83},
        {"A2", 110.00},
        {"E2", 82.41}
};

// Funkcja do znajdowania najbliższej zdefiniwanej częstotliwości
float Tuner::getClosestTarget(float maxFreq)
{
    float closestFreq = guitarStrings[0].frequency;
    float minDiff = abs(maxFreq - closestFreq);

    for (const auto& gs : guitarStrings) {
        float diff = abs(maxFreq - gs.frequency);
        if (diff < minDiff) {
            minDiff = diff;
            closestFreq = gs.frequency;
        }
    }
    return closestFreq;
}

std::string Tuner::getClosestName(float maxFreq)
{
    std::string closestName = guitarStrings[0].name;
    float minDiff = abs(maxFreq - (float)guitarStrings[0].frequency);

    for (const auto& gs : guitarStrings) {
        float diff = abs(maxFreq - (float)gs.frequency);
        if (diff < minDiff) {
            minDiff = diff;
            closestName = gs.name;
        }
    }
    return closestName;
}

// Statyczna — nie dotyka `this`. Jedyny współdzielony stan to `state`,
// przekazany jako shared_ptr (kopia trzymana przez wątek, niezależna od
// tego czy oryginalny Tuner nadal istnieje).
void Tuner::analyze(std::shared_ptr<SharedState> state, std::vector<float> fftBuffer)
{
    int N = fftBuffer.size();
    std::vector<float> nsdf(N, 0.0f);

    // 0. Sprawdzenie czy sygnał jest wystarczająco silny
    double energy = 0.0;
    for (int i = 0; i < N; i++) {
        energy += fftBuffer[i] * fftBuffer[i];
    }
    energy = sqrt(energy / N);  // RMS
    if (energy < 0.0009) {        // próg zależny od mikrofonu
        return;
    }

    int minTau = sampleRate / 1200;
    int maxTau = std::min(sampleRate / 25, N / 2);

    // NSDF tylko w potrzebnym zakresie
    for (int tau = minTau; tau <= maxTau; tau++) {
        double numerator = 0.0;
        double denominator = 0.0;
        for (int i = 0; i < N - tau; i++) {
            numerator += fftBuffer[i] * fftBuffer[i + tau];
            denominator += fftBuffer[i] * fftBuffer[i] + fftBuffer[i + tau] * fftBuffer[i + tau];
        }
        nsdf[tau] = (denominator > 0) ? (2.0 * numerator / denominator) : 0;
    }

    // 2. Znalezienie głównego maksimum (po tau > 0)
    std::vector<Candidate> candidates;

    for (int tau = minTau; tau < maxTau - 1; tau++) {
        if (nsdf[tau] > 0.5 &&
            nsdf[tau] > nsdf[tau - 1] &&
            nsdf[tau] > nsdf[tau + 1]) {
            candidates.push_back({ tau, (float)sampleRate / tau, nsdf[tau] });
        }
    }

    if (candidates.empty()) return;

    sort(candidates.begin(), candidates.end(),
        [](const Candidate& a, const Candidate& b) { return a.value > b.value; });

    int peakIndex = -1;
    for (auto& c : candidates) {
        if (c.freq < 2000 && c.freq > 50) {
            peakIndex = c.tau;
            break;
        }
    }

    if (peakIndex < 2) return;

    // czy to sub-oktawa? sprawdź czy jest peak przy połowie tau
    for (auto& c : candidates) {
        if (c.tau > peakIndex * 0.45 && c.tau < peakIndex * 0.55
            && c.value > 0.5f * candidates[0].value) {
            peakIndex = c.tau;
            break;
        }
    }

    if (peakIndex < 2) return;

    float LN = nsdf[peakIndex - 1];
    float RN = nsdf[peakIndex + 1];
    float pv = nsdf[peakIndex];
    float offset = 0.5f * (LN - RN) / (LN - 2 * pv + RN);
    float trueIndex = peakIndex + offset;

    // 4. Wyliczenie częstotliwości
    float rawFreq = sampleRate / trueIndex;
    float meanFreq;

    {
        std::lock_guard<std::mutex> lock(state->mutex);
        if (state->freqHistory.size() >= 3) {
            std::vector<float> sorted = state->freqHistory;
            std::sort(sorted.begin(), sorted.end());
            float currentMedian = sorted[sorted.size() / 2];
            if (abs(rawFreq - currentMedian) > 300) return;
        }
        state->freqHistory.push_back(rawFreq);
        if (state->freqHistory.size() > 10)
            state->freqHistory.erase(state->freqHistory.begin());

        if (state->freqHistory.size() < 3) {
            return;
        }

        std::vector<float> sorted = state->freqHistory;
        std::sort(sorted.begin(), sorted.end());
        meanFreq = sorted[sorted.size() / 2];
    }

    if (meanFreq > 1000) meanFreq = 0;
    if (meanFreq < 10.0f) return;  // nie publikuj "zera"

    float detectedString = getClosestTarget(meanFreq);
    std::string detectedName = getClosestName(meanFreq);
    float centsDifference = 1200.0f * log2f(meanFreq / detectedString);

    TunerReading reading;
    reading.freq = meanFreq;
    reading.target = detectedString;
    reading.cents = centsDifference;
    reading.stringName = detectedName;
    reading.inTune = std::abs(centsDifference) < 5.0f;
    reading.valid = true;

    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->lastReading = reading;
    }

    // ASCII podgląd w konsoli (wersja terminalowa) — ten sam widok co dawniej
    printf("\033[2J\033[H");

    const int barWidth = 41;
    const float maxCents = 50.0f;
    int pos = (int)((centsDifference / maxCents) * (barWidth / 2)) + barWidth / 2;
    pos = std::max(0, std::min(barWidth - 1, pos));

    std::string bar(barWidth, '-');
    bar[barWidth / 2] = '|';
    bar[pos] = '*';

    printf("+------------------------------------------+\n");
    printf("|           GUITAR TUNER                   |\n");
    printf("+------------------------------------------+\n");
    printf("|  Struna: %-6s     Freq: %6.1f Hz       |\n", detectedName.c_str(), meanFreq);
    printf("|  Target: %-6s     Freq: %6.2f Hz       |\n", detectedName.c_str(), detectedString);
    printf("|                                          |\n");
    printf("|  [%s]  |\n", bar.c_str());
    printf("|   -50        -10    0    +10        +50  |\n");
    printf("|                                          |\n");

    if (std::abs(centsDifference) < 5.0f)
        printf("|          *** NASTROJONO! ***             |\n");
    else if (centsDifference > 0)
        printf("|     %+6.1f centow  ->  poluzuj strune      |\n", centsDifference);
    else
        printf("|     %+6.1f centow  ->  naciagnij strune  |\n", centsDifference);

    printf("+------------------------------------------+\n");
}

std::string Tuner::getParamName(int idx) {

    return "?";
}

TunerReading Tuner::getReading() const {
    std::lock_guard<std::mutex> lock(state->mutex);
    return state->lastReading;
}
