#include <conio.h>

#include "AudioEngine.h"
#include "EventHandler.h"

#include "Tremolo.h"
#include "Fuzz.h"
#include "Tuner.h"

int main() {

    AudioEngine engine;
    EventHandler eventHandler(&engine);

	// komunikacja po UART z Arduino
    std::cout << "Otwieram port...\n";
    if (eventHandler.openSerial("COM12"))
        std::cout << "Port otwarty\n";
    else
        std::cout << "Błąd otwierania portu\n";

    std::thread serialThread([&]() { eventHandler.runSerial(); });
    serialThread.detach();
    std::cout << "Po openSerial\n";
   

    engine.getChain().toggle(new Fuzz());
    engine.init_single_effect(16);

    while (engine.isActive()) {
        Pa_Sleep(100);
        if (_kbhit()) {
            char key = _getch();
            eventHandler.handleKey(key);
        }
    }
    engine.stop();
}