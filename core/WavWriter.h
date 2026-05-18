#pragma once
#include <string>
#include <fstream>
#include <cstdint>


class WavWriter {
public:
    WavWriter(int channels = 1);
    ~WavWriter();

    bool start(const std::string& filename);
    void writeSample(float sample);   // -1.0 do 1.0
    void stop();
    bool isRecording() const { return recording; }

private:
    void writeHeader();
    void patchHeader();  // poprawia rozmiary na końcu

    std::ofstream file;
    int channels;
    bool recording = false;
    uint32_t sampleCount = 0;
};