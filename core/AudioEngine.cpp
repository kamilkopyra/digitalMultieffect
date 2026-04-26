#include "AudioEngine.h"


int AudioEngine::init_single_effect(int FramesPerBuffer)
{
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "Init error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    // Wypisz listę urządzeń
    int numDev = Pa_GetDeviceCount();
    std::cout << "=== PortAudio devices ===\n";
    for (int i = 0; i < numDev; ++i)
        std::cout << i << ": " << Pa_GetDeviceInfo(i)->name << "\n";
    std::cout << "=========================\n";

    // Automatyczne wybieranie
    PaStreamParameters inParams{}, outParams{};
    int inDev = Pa_GetDefaultInputDevice();
    int outDev = Pa_GetDefaultOutputDevice();

    for (int i = 0; i < numDev; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        std::string name(info->name);
        if (name.find("USB Audio") != std::string::npos && info->maxInputChannels > 0) {
            inDev = i;
            break;
        }
    }
    for (int i = 0; i < numDev; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        std::string name(info->name);
        if (name.find("USB Audio") != std::string::npos && info->maxOutputChannels > 0) {
            outDev = i;
            break;
        }
    }
    std::cout << "Wejście = " << inDev << ", Wyjście = " << outDev << "\n";

    // Parametry wejścia
    inParams.device = inDev;
    inParams.channelCount = 1;
    inParams.sampleFormat = paFloat32;
    inParams.suggestedLatency = Pa_GetDeviceInfo(inDev)->defaultLowInputLatency;
    inParams.hostApiSpecificStreamInfo = nullptr;

    // Parametry wyjścia
    outParams.device = outDev;
    outParams.channelCount = 2;
    outParams.sampleFormat = paFloat32;
    outParams.suggestedLatency = Pa_GetDeviceInfo(outDev)->defaultLowOutputLatency;
    outParams.hostApiSpecificStreamInfo = nullptr;

    
    err = Pa_OpenStream(&stream, &inParams, &outParams,
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

    std::cout << "Strumień uruchomiony, Ctrl+C aby zakończyć\n";
    return 0;
}

void AudioEngine::setEffect(Effect* e)
{
    delete effect;   //usuwam stary efekt
	effect = e;     //zapisuję wskaźnik na nowy efekt
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
        if (engine->effect)
            std::cout << "Program uruchomiony, efekt: " << engine->effect->getName() << "\n";
        else
            std::cout << "Program uruchomiony, brak efektu\n";
        printed = true;
    }

    if (!in) return paContinue;

    for (unsigned i = 0; i < framesPerBuffer; ++i) {
        float sample = *in++;
        float modified = engine->effect ? engine->effect->process(sample) : sample;
        *out++ = modified;
        *out++ = modified;
    }
    return paContinue;
}

void AudioEngine::stop() {
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}

bool AudioEngine::isActive() {
    return Pa_IsStreamActive(stream) == 1;
}

   
