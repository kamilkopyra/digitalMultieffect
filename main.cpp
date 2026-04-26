#include <conio.h>

#include "AudioEngine.h"
#include "EventHandler.h"

#include "Tremolo.h"
#include "Fuzz.h"
#include "Tuner.h"

int main() {

    AudioEngine engine;
    EventHandler eventHandler(&engine);

    new Fuzz();

    engine.setEffect(new Fuzz());
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