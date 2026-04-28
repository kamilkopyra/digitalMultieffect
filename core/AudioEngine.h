#pragma once
#include <iostream>
#include <portaudio.h>

#include "Effect.h"

class AudioEngine
{


    private:

        #define sampleRate       48000
		PaStream* stream = nullptr;   // Wskaźnik na otwarty strumień audio
        Effect* effect = nullptr;

        static int audioCallback(const void* inputBuffer, void* outputBuffer,
            unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void* userData);
        



    public:
		    int init_single_effect(int buffer);
            void setEffect(Effect* e);
            void stop();
			bool isActive();
            int pot[3] = { 0, 0, 0 };
};

