#include "AudioEngine.h"
#include <algorithm>

std::vector<AudioDeviceInfo> AudioEngine::listDevices()
{
    std::vector<AudioDeviceInfo> result;

    if (Pa_Initialize() != paNoError)
        return result;

    int numDev = Pa_GetDeviceCount();
    int defaultIn = Pa_GetDefaultInputDevice();
    int defaultOut = Pa_GetDefaultOutputDevice();

    for (int i = 0; i < numDev; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (!info) continue;
        if (info->maxInputChannels <= 0 && info->maxOutputChannels <= 0) continue;

        const PaHostApiInfo* host = Pa_GetHostApiInfo(info->hostApi);
        std::string label = std::string(info->name) + " (" + (host ? host->name : "?") + ")";

        bool isUmc = label.find("(USB Audio CODEC)") != std::string::npos;
        bool isDefault = (i == defaultIn) || (i == defaultOut);

        result.push_back({ i, label, info->maxInputChannels, info->maxOutputChannels, isUmc || isDefault });
    }

    Pa_Terminate();   // symetryczne do Pa_Initialize() powyżej — PortAudio liczy wywołania
    return result;
}

int AudioEngine::init_single_effect(int FramesPerBuffer)
{
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "Init error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    int numDev = Pa_GetDeviceCount();
    std::cout << "=== PortAudio devices ===\n";
    for (int i = 0; i < numDev; ++i)
        std::cout << i << ": " << Pa_GetDeviceInfo(i)->name
        << " (in=" << Pa_GetDeviceInfo(i)->maxInputChannels
        << " out=" << Pa_GetDeviceInfo(i)->maxOutputChannels << ")\n";
    std::cout << "=========================\n";

    int inDev = -1;
    int outDev = -1;

    // Szukamy UMC22 — osobno wejście, osobno wyjście (to dwa różne wpisy)
    for (int i = 0; i < numDev; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        std::string name(info->name);
        if (name.find("(USB Audio CODEC)") != std::string::npos && info->maxInputChannels > 0) {
            inDev = i;
            break;
        }
    }
    for (int i = 0; i < numDev; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        std::string name(info->name);
        if (name.find("(USB Audio CODEC)") != std::string::npos && info->maxOutputChannels > 0) {
            outDev = i;
            break;
        }
    }

    // Fallback — na innym komputerze (bez UMC22) bierzemy domyślne
    // urządzenie systemowe zamiast się poddawać.
    bool usedFallback = false;
    if (inDev == -1) {
        inDev = Pa_GetDefaultInputDevice();
        usedFallback = true;
    }
    if (outDev == -1) {
        outDev = Pa_GetDefaultOutputDevice();
        usedFallback = true;
    }

    if (inDev == paNoDevice || outDev == paNoDevice) {
        std::cerr << "Nie znaleziono zadnego urzadzenia audio (ani UMC22, ani domyslnego)\n";
        return 1;
    }

    if (usedFallback)
        std::cout << "UMC22 nie znaleziony - uzywam domyslnego urzadzenia systemowego\n";

    return openStream(FramesPerBuffer, inDev, outDev);
}

int AudioEngine::init_single_effect(int FramesPerBuffer, int inputDeviceIndex, int outputDeviceIndex)
{
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "Init error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    return openStream(FramesPerBuffer, inputDeviceIndex, outputDeviceIndex);
}

int AudioEngine::openStream(int FramesPerBuffer, int inDev, int outDev)
{
    int numDev = Pa_GetDeviceCount();
    if (inDev < 0 || outDev < 0 || inDev >= numDev || outDev >= numDev) {
        std::cerr << "Nieprawidlowy indeks urzadzenia audio (in=" << inDev << " out=" << outDev << ")\n";
        return 1;
    }
    if (Pa_GetDeviceInfo(inDev)->maxInputChannels <= 0) {
        std::cerr << "Urzadzenie " << inDev << " (" << Pa_GetDeviceInfo(inDev)->name << ") nie ma wejscia\n";
        return 1;
    }
    if (Pa_GetDeviceInfo(outDev)->maxOutputChannels <= 0) {
        std::cerr << "Urzadzenie " << outDev << " (" << Pa_GetDeviceInfo(outDev)->name << ") nie ma wyjscia\n";
        return 1;
    }

    std::cout << "Wybrano wejście = " << inDev << ": " << Pa_GetDeviceInfo(inDev)->name << "\n";
    std::cout << "Wybrano wyjście = " << outDev << ": " << Pa_GetDeviceInfo(outDev)->name << "\n";

    // Nie zakładamy sztywno 2 kanałów — urządzenie może mieć tylko 1
    // (np. wbudowany mikrofon laptopa). Woleliśmy 2 (prawy kanał = INST na
    // UMC22), ale bierzemy tyle ile urządzenie faktycznie ma.
    inputChannels = std::min(2, Pa_GetDeviceInfo(inDev)->maxInputChannels);
    if (inputChannels < 1) inputChannels = 1;
    outputChannels = std::min(2, Pa_GetDeviceInfo(outDev)->maxOutputChannels);
    if (outputChannels < 1) outputChannels = 1;

    PaStreamParameters inParams{}, outParams{};

    inParams.device = inDev;
    inParams.channelCount = inputChannels;
    inParams.sampleFormat = paFloat32;
    inParams.suggestedLatency = Pa_GetDeviceInfo(inDev)->defaultLowInputLatency;
    inParams.hostApiSpecificStreamInfo = nullptr;

    outParams.device = outDev;
    outParams.channelCount = outputChannels;
    outParams.sampleFormat = paFloat32;
    outParams.suggestedLatency = Pa_GetDeviceInfo(outDev)->defaultLowOutputLatency;
    outParams.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(&stream, &inParams, &outParams,
        sampleRate, FramesPerBuffer,
        paClipOff, audioCallback, this);
    if (err != paNoError) {
        std::cerr << "OpenStream error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "StartStream error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    curInDev = inDev;
    curOutDev = outDev;

    std::cout << "Strumień uruchomiony, Ctrl+C aby zakończyć\n";
    return 0;
}

int AudioEngine::audioCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    AudioEngine* engine = static_cast<AudioEngine*>(userData);
    const float* in = static_cast<const float*>(inputBuffer);
    float* out = static_cast<float*>(outputBuffer);

    static bool printed = false;
    if (!printed) {
        std::cout << "Program uruchomiony\n";
        printed = true;
    }



    if (!in) return paContinue;

    // Nie zakładamy sztywno 2 kanałów wejścia/wyjścia — urządzenie awaryjne
    // (np. wbudowany mikrofon) może mieć tylko 1. Bierzemy ostatni kanał
    // wejścia jako sygnał (na UMC22 to prawy = INST; przy mono to jedyny
    // kanał jaki jest), i duplikujemy wyjście na tyle kanałów ile otwarto.
    const int inCh = engine->inputChannels;
    const int outCh = engine->outputChannels;

    for (unsigned i = 0; i < framesPerBuffer; ++i) {
        float sample = 0.0f;
        for (int c = 0; c < inCh; ++c)
            sample = *in++;   // ostatni odczytany kanał zostaje jako sygnał

        float modified = engine->chain.process(sample);


        if (engine->wavWriter.isRecording())
            engine->wavWriter.writeSample(modified);

        {
            std::lock_guard<std::mutex> lock(engine->waveformMutex);
            engine->waveformBuffer[engine->waveformWriteIndex] = modified;
            engine->waveformWriteIndex = (engine->waveformWriteIndex + 1) % waveformSize;
        }

        for (int c = 0; c < outCh; ++c)
            *out++ = modified;
    }
    return paContinue;
}

std::vector<float> AudioEngine::snapshotWaveform() {
    std::lock_guard<std::mutex> lock(waveformMutex);
    std::vector<float> result(waveformSize);
    for (int i = 0; i < waveformSize; ++i)
        result[i] = waveformBuffer[(waveformWriteIndex + i) % waveformSize];
    return result;
}

void AudioEngine::stop() {
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}

bool AudioEngine::isActive() {
    return Pa_IsStreamActive(stream) == 1;
}
