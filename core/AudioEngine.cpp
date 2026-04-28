#include "AudioEngine.h"


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

    PaStreamParameters inParams{}, outParams{};
    int inDev = -1;
    int outDev = -1;

    // Szukamy UMC22 — to urządzenie z 2 kanałami wejściowymi i nazwą "USB Audio"
    for (int i = 0; i < numDev; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        std::string name(info->name);
        // szukamy nazwy zaczynającej się od "Mikrofon (USB" — wykluczamy "Mikrofon (2 — USB"
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

    if (inDev == -1 || outDev == -1) {
        std::cerr << "Nie znaleziono UMC22\n";
        return 1;
    }

    std::cout << "Wybrano wejście = " << inDev << ": " << Pa_GetDeviceInfo(inDev)->name << "\n";
    std::cout << "Wybrano wyjście = " << outDev << ": " << Pa_GetDeviceInfo(outDev)->name << "\n";

    inParams.device = inDev;
    inParams.channelCount = 2;  // ważne — żeby dostać INST 2 jako prawy kanał
    inParams.sampleFormat = paFloat32;
    inParams.suggestedLatency = Pa_GetDeviceInfo(inDev)->defaultLowInputLatency;
    inParams.hostApiSpecificStreamInfo = nullptr;

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
    
    if (effect) 
    {
        effect->setPot(pot);
    }
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
        float left = *in++;   
        float right = *in++;  

        float sample = right; 

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

   
