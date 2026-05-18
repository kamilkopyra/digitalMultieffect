#include "WavWriter.h"
#include <cstring>
#include "AudioEngine.h"


WavWriter::WavWriter(int channels)
    : channels(channels) {
}

WavWriter::~WavWriter() {
    if (recording) stop();
}

bool WavWriter::start(const std::string& filename) {
    file.open(filename, std::ios::binary);
    if (!file.is_open()) return false;

    sampleCount = 0;
    recording = true;
    writeHeader();  // placeholder, naprawimy na końcu
    return true;
}

void WavWriter::writeSample(float sample) {
    if (!recording) return;

    // Konwersja float → int16 (PCM 16-bit)
    int16_t pcm = static_cast<int16_t>(
        std::max(-1.0f, std::min(1.0f, sample)) * 32767.0f
        );
    file.write(reinterpret_cast<const char*>(&pcm), sizeof(pcm));
    sampleCount++;
}

void WavWriter::stop() {
    if (!recording) return;
    recording = false;
    patchHeader();  // uzupełniamy rozmiary
    file.close();
}

void WavWriter::writeHeader() {
    // Zapisujemy placeholder — prawdziwe rozmiary wstawiamy w patchHeader()
    uint32_t zero = 0;
    uint16_t audioFmt = 1;         // PCM
    uint16_t numCh = channels;
    uint32_t sr = sampleRate;
    uint32_t byteRate = sampleRate * channels * 2; // 16-bit = 2 bajty
    uint16_t blockAlign = channels * 2;
    uint16_t bitsPerSample = 16;

    file.write("RIFF", 4);
    file.write(reinterpret_cast<char*>(&zero), 4);       
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    uint32_t subchunk1 = 16;
    file.write(reinterpret_cast<char*>(&subchunk1), 4);
    file.write(reinterpret_cast<char*>(&audioFmt), 2);
    file.write(reinterpret_cast<char*>(&numCh), 2);
    file.write(reinterpret_cast<char*>(&sr), 4);
    file.write(reinterpret_cast<char*>(&byteRate), 4);
    file.write(reinterpret_cast<char*>(&blockAlign), 2);
    file.write(reinterpret_cast<char*>(&bitsPerSample), 2);
    file.write("data", 4);
    file.write(reinterpret_cast<char*>(&zero), 4);       
}

void WavWriter::patchHeader() {
    uint32_t dataSize = sampleCount * channels * 2;
    uint32_t chunkSize = 36 + dataSize;

    file.seekp(4);
    file.write(reinterpret_cast<char*>(&chunkSize), 4);
    file.seekp(40);
    file.write(reinterpret_cast<char*>(&dataSize), 4);
}
