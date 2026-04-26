#pragma once
#include "AudioEngine.h"

class EventHandler
{
	

public:
	EventHandler(AudioEngine* e) {
		engine = e;
	}
	void handleKey(char key);
	void run();


private:
	AudioEngine* engine;

};

