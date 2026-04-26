#include "EventHandler.h"
#include "Fuzz.h"
#include "Tremolo.h"
#include "NoiseGate.h"
#include "Tuner.h"


void EventHandler::handleKey(char key)
{
	switch (key) {
	case '1': {
		engine->setEffect(new Fuzz());
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		break;
	}
	case '2': {
		engine->setEffect(new Tremolo());
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		break;
	}
	case '3': {
		engine->setEffect(new Tuner());
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		break;
	}
	case '4': {
		engine->setEffect(new NoiseGate());
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		break;
	}
	case '0': {
		engine->setEffect(nullptr);
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		break;
	}
	case 'q': {
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		exit(0);
		break;
	}
	default:
		std::cout << "Nieznany klawisz: " << key << "\n";
	}
}


void EventHandler::run() {
	char key;
	while (true) {
		std::cin >> key;
		handleKey(key);
	}
}