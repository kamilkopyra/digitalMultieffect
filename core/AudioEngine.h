#pragma once
#include <iostream>
#include <portaudio.h>

#include "Effect.h"
#include "EffectChain.h"
#include "WavWriter.h"
#include <string>
#include <vector>
#include <mutex>

// Opis jednego urządzenia audio widzianego przez PortAudio. Windows (i przez
// to PortAudio) traktuje wejście i wyjście jako OSOBNE obiekty — jedno
// fizyczne urządzenie (np. UMC22) pojawia się jako dwa oddzielne wpisy,
// jeden tylko-wejście, jeden tylko-wyjście, prawie nigdy jako jeden wpis
// z obydwoma. Stąd dwie osobne listy/wybory poniżej, nie jeden.
struct AudioDeviceInfo {
	int index;
	std::string name;   // z nazwą backendu (MME/DirectSound/WASAPI/WDM-KS)
	int maxInputChannels;
	int maxOutputChannels;
	// true, jeśli to ten sam wpis co znalazłaby autodetekcja (nazwa
	// zawiera "(USB Audio CODEC)") albo jest domyślnym urządzeniem
	// systemowym — podpowiedź w GUI, żeby nie zgadywać z gołej listy Windowsa
	bool recommended;
};

class AudioEngine
{


    private:

		PaStream* stream = nullptr;   // Wskaźnik na otwarty strumień audio
        EffectChain chain;            // cały tor sygnału to łańcuch efektów

        // Rzeczywista liczba kanałów otwartego strumienia — nie zawsze 2.
        // Urządzenie awaryjne (domyślne systemowe, np. wbudowany mikrofon
        // laptopa) może mieć tylko 1 kanał wejściowy; audioCallback musi
        // wiedzieć ile próbek na ramkę faktycznie czytać/pisać.
        int inputChannels = 2;
        int outputChannels = 2;

        // Bufor kołowy ostatnich próbek wyjściowych — do prostej wizualizacji
        // (oscyloskop) w GUI. Pisany z wątku audio, czytany (kopiowany) z
        // dowolnego wątku przez snapshotWaveform(). Zwykły mutex wystarczy —
        // GUI odpytuje to kilkanaście razy na sekundę, nie w takt próbek.
        static constexpr int waveformSize = 512;
        std::mutex waveformMutex;
        float waveformBuffer[waveformSize] = { 0.0f };
        int waveformWriteIndex = 0;

        static int audioCallback(const void* inputBuffer, void* outputBuffer,
            unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void* userData);

        // wspólna reszta otwierania strumienia (parametry kanałów, OpenStream,
        // StartStream)
        int openStream(int framesPerBuffer, int inDev, int outDev);

        int curInDev = -1;
        int curOutDev = -1;

    public:
            WavWriter wavWriter;

            // Wszystkie urządzenia widziane przez PortAudio (bezpieczna do
            // wywołania w dowolnym momencie, sama inicjalizuje/zamyka
            // PortAudio). Filtrowanie na "wejściowe"/"wyjściowe" (po
            // maxInputChannels/maxOutputChannels) zostawiamy wywołującemu —
            // to samo urządzenie może być w jednej liście, w drugiej, albo
            // (rzadko) w obu.
            static std::vector<AudioDeviceInfo> listDevices();

            // Autodetekcja: szuka "(USB Audio CODEC)", a jak nie znajdzie —
            // spada na domyślne urządzenie systemowe.
		    int init_single_effect(int buffer);
            // Jawny wybór — OSOBNY indeks dla wejścia i wyjścia (indeksy
            // z listDevices()), bo to zwykle dwa różne wpisy.
            int init_single_effect(int buffer, int inputDeviceIndex, int outputDeviceIndex);

            void stop();
			bool isActive();

            EffectChain& getChain() { return chain; }
            Effect* getEffect() { return &chain; }   // zgodność wstecz (nagrywanie, tap tempo)

            // kopia bufora, od najstarszej do najnowszej próbki — bezpieczna
            // do wywołania z dowolnego wątku
            std::vector<float> snapshotWaveform();

            // aktualnie otwarte urządzenia (-1 = brak/nieudane)
            int currentInputDevice() const { return curInDev; }
            int currentOutputDevice() const { return curOutDev; }

};
