#include "Tuner.h"

// Błędy:
// problem z wielowątkowością - program czasami się crashuje
// problem z wykrywaniem - czasami wykrywana jest harmoniczna zamiast częstotliwości podstawowek
// problem z czułością - czasami wykrywana jest częstotliwość, ale jest ona bardzo niestabilna


float Tuner::process(float sample) {

    fftBuffer.push_back(sample);
    if (fftBuffer.size() >= WINDOW_SIZE) {
        std::vector<float> bufferCopy = fftBuffer;
        std::thread t(&Tuner::McLeod_Method, this, bufferCopy); // nowy wątek żeby nie zaśmiecać toru audio
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

std::vector<Tuner::GuitarString> guitarStrings = {
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
    std::string closestString = guitarStrings[0].name;
    float minDiff = abs(maxFreq - closestFreq);

    for (const auto& gs : guitarStrings) {
        float diff = abs(maxFreq - gs.frequency);
        if (diff < minDiff) {
            minDiff = diff;
            closestFreq = gs.frequency;
            closestString = gs.name;
        }
    }

    //printf("Closest string: %s with frequency %f Hz\n", closestString.c_str(), closestFreq);
    //printf("Difference: %f\n\n", minDiff);
    return closestFreq;
}

void Tuner::McLeod_Method(std::vector<float> fftBuffer)
{
    int N = fftBuffer.size();
    std::vector<float> nsdf(N, 0.0f);
    float meanFreq;
    float detectedString;
    float centsDifference;

    // 0. Sprawdzenie czy sygnał jest wystarczająco silny
    double energy = 0.0;
    for (int i = 0; i < N; i++) {
        energy += fftBuffer[i] * fftBuffer[i];
    }
    energy = sqrt(energy / N);  // RMS
    //printf("energy: %.8f\n", energy);
    if (energy < 0.0009) {        // próg zależny od mikrofonu
        meanFreq = 0.0;
        return;
        //printf("McLeod Method : %.1f\n", meanFreq);

    }

    else {

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
        //int peakIndex = 1;
        float peakValue = -1.0f;
        float maxScore = 0.0f;

        std::vector<Candidate> candidates;
        float maxNsdf = *max_element(nsdf.begin() + minTau, nsdf.begin() + maxTau);
        //printf("max NSDF: %.4f, energy: %.6f\n", maxNsdf, energy);

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
        int size = candidates.size();

        float LN = nsdf[peakIndex - 1];
        float RN = nsdf[peakIndex + 1];

        float pv = nsdf[peakIndex];
        float offset = 0.5f * (LN - RN) / (LN - 2 * pv + RN);
        float trueIndex = peakIndex + offset;

        // 4. Wyliczenie częstotliwości
        float interpolatedFreq = sampleRate / trueIndex;

        float rawFreq = interpolatedFreq;

        {
            std::lock_guard<std::mutex> lock(historyMutex);
            if (freqHistory.size() >= 3) {
                std::vector<float> sorted = freqHistory;
                std::sort(sorted.begin(), sorted.end());
                float currentMedian = sorted[sorted.size() / 2];
                if (abs(rawFreq - currentMedian) > 300) return;
            }
            freqHistory.push_back(rawFreq);
            if (freqHistory.size() > 10)
                freqHistory.erase(freqHistory.begin());

            if (freqHistory.size() < 3) {
                return;
            }
        }

        std::vector<float> sorted = freqHistory;
        std::sort(sorted.begin(), sorted.end());
        meanFreq = sorted[sorted.size() / 2];
        //nFreq: %.2f\n", meanFreq);
        if (meanFreq > 1000)
        {
            meanFreq = 0;
        }
        detectedString = getClosestTarget(meanFreq);
        std::string detectedName = getClosestName(meanFreq);
        centsDifference = (meanFreq > 0.0f)
            ? 1200.0f * log2f(meanFreq / detectedString)
            : 0.0f;
        printTuner(meanFreq, detectedString, centsDifference, detectedName);

    }

}



void Tuner::printTuner(float freq, float target, float cents, const std::string& stringName)
{
    std::lock_guard<std::mutex> lock(printMutex);
    if (freq < 10.0f) return;  // nie rysuj dla 0

    printf("\033[2J\033[H");

    const int barWidth = 41;
    const float maxCents = 50.0f;
    int pos = (int)((cents / maxCents) * (barWidth / 2)) + barWidth / 2;
    pos = std::max(0, std::min(barWidth - 1, pos));

    std::string bar(barWidth, '-');
    bar[barWidth / 2] = '|';
    bar[pos] = '*';

    printf("+------------------------------------------+\n");
    printf("|           GUITAR TUNER                   |\n");
    printf("+------------------------------------------+\n");
    printf("|  Struna: %-6s     Freq: %6.1f Hz       |\n", stringName.c_str(), freq);
    printf("|  Target: %-6s     Freq: %6.2f Hz       |\n", stringName.c_str(), target);
    printf("|                                          |\n");
    printf("|  [%s]  |\n", bar.c_str());
    printf("|   -50        -10    0    +10        +50  |\n");
    printf("|                                          |\n");

    if (abs(cents) < 5.0f)
        printf("|          *** NASTROJONO! ***             |\n");
    else if (cents > 0)
        printf("|     %+6.1f centow  ->  poluzuj strune      |\n", cents);
    else
        printf("|     %+6.1f centow  ->  naciagnij strune  |\n", cents);

    printf("+------------------------------------------+\n");
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
