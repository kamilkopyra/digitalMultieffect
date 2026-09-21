#pragma once
#include <iostream>
#include <portaudio.h>

#include "Effect.h"
#include "EffectChain.h"
#include "WavWriter.h"
#include <string>

class AudioEngine
{


    private:

		PaStream* stream = nullptr;   // Wskaźnik na otwarty strumień audio
        EffectChain chain;            // cały tor sygnału to łańcuch efektów

        static int audioCallback(const void* inputBuffer, void* outputBuffer,
            unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void* userData);

    public:
            WavWriter wavWriter;
		    int init_single_effect(int buffer);
            void stop();
			bool isActive();

            EffectChain& getChain() { return chain; }
            Effect* getEffect() { return &chain; }   // zgodność wstecz (nagrywanie, tap tempo)

};
