#include "EventHandler.h"
#include "Fuzz.h"
#include "Tremolo.h"
#include "NoiseGate.h"
#include "Tuner.h"
#include "Rat.h"
#include "TubeScreamer.h"
#include "Delay.h"
#include "Compressor.h"
#include "Vibrato.h"
#include "Flanger.h"
#include "Phaser.h"
#include "AutoWah.h"
#include "Equalizer.h"
#include "Boost.h"
#include "WavWriter.h"
#include <filesystem>
#include <algorithm>
#include <ctime>

// Wrzuca / usuwa efekt w aktualnie wybranym (edytowanym) slocie i wypisuje
// stan toru. Ten sam klawisz drugi raz usuwa efekt (patrz toggleInSlot).
static void toggleAndReport(AudioEngine* engine, Effect* effect, char key) {
	EffectChain& chain = engine->getChain();
	chain.toggleInSlot(chain.focusedIndex(), effect);
	std::cout << "Wcisnieto klawisz: " << key << "\n";
	std::cout << "Lancuch: " << chain.describe() << "\n";
}

void EventHandler::handleKey(char key)
{
	switch (key) {
	case '1': toggleAndReport(engine, new Fuzz(), key); break;
	case '2': toggleAndReport(engine, new Tremolo(), key); break;
	case '3': toggleAndReport(engine, new Tuner(), key); break;
	case '4': toggleAndReport(engine, new NoiseGate(), key); break;
	case '5': toggleAndReport(engine, new Rat(), key); break;
	case '6': toggleAndReport(engine, new TubeScreamer(), key); break;
	case '7': toggleAndReport(engine, new Delay(), key); break;
	case '8': toggleAndReport(engine, new Compressor(), key); break;
	case 'z': toggleAndReport(engine, new Vibrato(), key); break;
	case 'x': toggleAndReport(engine, new Flanger(), key); break;
	case 'c': toggleAndReport(engine, new Phaser(), key); break;
	case 'v': toggleAndReport(engine, new AutoWah(), key); break;
	case 'b': toggleAndReport(engine, new Equalizer(), key); break;
	case 'n': toggleAndReport(engine, new Boost(), key); break;

	case '0': {
		engine->getChain().clear();
		std::cout << "Wyczyszczono lancuch efektow.\n";
		break;
	}

	case '=': {
		bool ok = engine->getChain().addSlot();
		std::cout << (ok ? "Dodano slot. " : ("Juz maksimum (" + std::to_string(EffectChain::maxSlots) + "). "))
			<< "Lancuch: " << engine->getChain().describe() << "\n";
		break;
	}
	case '-': {
		bool ok = engine->getChain().removeSlot();
		std::cout << (ok ? "Usunieto ostatni slot. " : ("Juz minimum (" + std::to_string(EffectChain::minSlots) + "). "))
			<< "Lancuch: " << engine->getChain().describe() << "\n";
		break;
	}

	// edycja parametrów edytowanego efektu bez enkoderów (np. do testów na samym PC)
	case 'a': onEncoderTurn(0, -1); break;
	case 's': onEncoderTurn(0, +1); break;
	case 'd': onEncoderTurn(1, -1); break;
	case 'f': onEncoderTurn(1, +1); break;
	case 'g': onEncoderTurn(2, -1); break;
	case 'h': onEncoderTurn(2, +1); break;
	case 't': onEncoderButton(0); break;   // symulacja przycisku enkodera 0 (tap tempo)

	case '\t': {
		engine->getChain().focusNext();
		Effect* f = engine->getChain().focused();
		std::cout << "Edytowany efekt: " << (f ? f->getName() : "brak") << "\n";
		std::cout << "Lancuch: " << engine->getChain().describe() << "\n";
		break;
	}

	case 'r': {
		if (engine->wavWriter.isRecording()) {
			engine->wavWriter.stop();
			std::cout << "Nagrywanie zatrzymane.\n";
		}
		else {
			std::string nazwa = engine->getChain().getName();
			std::filesystem::create_directories("./recordings");
			std::filesystem::path p = std::filesystem::absolute("./recordings/" + nazwa + ".wav");
			std::cout << "Zapisuje do: " << p << "\n";
			bool ok = engine->wavWriter.start("./recordings/" + nazwa + ".wav");
			std::cout << (ok ? "Nagrywanie rozpoczete.\n" : "Blad otwarcia pliku!\n");
		}
		break;
	}
	case 'q': {
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		engine->stop();
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

bool EventHandler::openSerial(const std::string& portName, int baudRate) {
	try {
		port.open(portName);
		port.set_option(boost::asio::serial_port::baud_rate(baudRate));
		port.set_option(boost::asio::serial_port::character_size(8));
		port.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one));
		port.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::none));
		asyncRead();
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "Błąd otwierania portu: " << e.what() << "\n";
		return false;
	}
}

void EventHandler::asyncRead() {
	boost::asio::async_read_until(port, serialBuffer, '\n',
		[this](const boost::system::error_code& ec, std::size_t bytes) {
			onLineReceived(ec, bytes);
		});
}

void EventHandler::onLineReceived(const boost::system::error_code& ec, std::size_t bytes) {
	if (ec) return;

	std::istream stream(&serialBuffer);
	std::string line;
	std::getline(stream, line);

	int idx, dir;

	if (sscanf_s(line.c_str(), "E %d %d", &idx, &dir) == 2) {
		onEncoderTurn(idx, dir);
	}
	else if (sscanf_s(line.c_str(), "B %d", &idx) == 1) {
		onEncoderButton(idx);
	}

	asyncRead();
}


void EventHandler::runSerial() {
	io.run();
}

void EventHandler::onEncoderTurn(int idx, int dir) {
	if (idx < 0 || idx > 2) return;

	EffectChain& chain = engine->getChain();
	chain.adjustFocusedParam(idx, dir * 2);

	Effect* f = chain.focused();
	if (f)
		std::cout << f->getName() << " - "
		<< f->getParamName(idx) << ": "
		<< f->getParam(idx) << "\n";
	else
		std::cout << "Lancuch pusty - brak parametrow do edycji\n";
}


void EventHandler::onEncoderButton(int idx) {
	EffectChain& chain = engine->getChain();

	if (idx == 0 && chain.isModulation()) {
		// tap tempo dla wybranego efektu modulacyjnego
		static clock_t taps[4] = { 0, 0, 0, 0 };
		static int tapCount = 0;

		taps[tapCount % 4] = clock();
		tapCount++;

		if (tapCount >= 2) {
			int count = std::min(tapCount, 4);
			float avgMs = (float)(taps[(tapCount - 1) % 4] - taps[(tapCount - count) % 4])
				/ (count - 1) / CLOCKS_PER_SEC * 1000.0f;
			float bpm = 60000.0f / avgMs;

			chain.TapToParam(avgMs);
			std::cout << "Tap tempo: " << bpm << " BPM\n";
		}
	}
	else {
		// pozostałe przyciski enkoderów przełączają edytowany efekt
		chain.focusNext();
		Effect* f = chain.focused();
		std::cout << "Edytowany efekt: " << (f ? f->getName() : "brak") << "\n";
	}
}
