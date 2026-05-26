#include "EventHandler.h"
#include "Fuzz.h"
#include "Tremolo.h"
#include "NoiseGate.h"
#include "Tuner.h"
#include "Rat.h"
#include "TubeScreamer.h"
#include "Delay.h"
#include "Compressor.h"
#include "WavWriter.h"
#include <filesystem>

void EventHandler::handleKey(char key)
{
	switch (key) {
	case '1': {
		engine->setEffect(new Fuzz());
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		std::cout << "Wczytano efekt: " << engine->getEffect()->getName() << "\n";
		break;
	}
	case '2': {
		engine->setEffect(new Tremolo());
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		std::cout << "Wczytano efekt: " << engine->getEffect()->getName() << "\n";
		break;
	}
	case '3': {
		engine->setEffect(new Tuner());
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		std::cout << "Wczytano efekt: " << engine->getEffect()->getName() << "\n";
		break;
	}
	case '4': {
		engine->setEffect(new NoiseGate());
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		std::cout << "Wczytano efekt: " << engine->getEffect()->getName() << "\n";
		break;
	}
	case '5': {
		engine->setEffect(new Rat());
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		std::cout << "Wczytano efekt: " << engine->getEffect()->getName() << "\n";
		break;
	}
	case '6': {
		engine->setEffect(new TubeScreamer());
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		std::cout << "Wczytano efekt: " << engine->getEffect()->getName() << "\n";
		break;
	}
	case '7': {
		engine->setEffect(new Delay());
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		std::cout << "Wczytano efekt: " << engine->getEffect()->getName() << "\n";
		break;
	}
	case '8': {
		engine->setEffect(new Compressor());
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		std::cout << "Wczytano efekt: " << engine->getEffect()->getName() << "\n";
		break;
	}

	case '0': {
		engine->setEffect(nullptr);
		std::cout << "Wcisnieto klawisz: " << key << "\n";
		break;
	}

	case 'r': {
		if (engine->wavWriter.isRecording()) {
			engine->wavWriter.stop();
			std::cout << "Nagrywanie zatrzymane.\n";
		}
		else {
			std::string nazwa = engine->getEffect() ? engine->getEffect()->getName() : "brak_efektu";
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

//void EventHandler::onLineReceived(const boost::system::error_code& ec, std::size_t bytes) {
//	if (ec) return;
//
//	std::istream stream(&serialBuffer);
//	std::string line;
//	std::getline(stream, line);
//
//	int a, b, c;
//	if (sscanf_s(line.c_str(), "POT1: %d  POT2: %d  POT3: %d", &a, &b, &c) == 3) {
//		pot[0] = a;
//		pot[1] = b;
//		pot[2] = c;
//		//std::cout << "pot0=" << pot[0] << " pot1=" << pot[1] << " pot2=" << pot[2] << "\n";
//	}
//
//	asyncRead();
//}

void EventHandler::onLineReceived(const boost::system::error_code& ec, std::size_t bytes) {
	if (ec) return;

	std::istream stream(&serialBuffer);
	std::string line;
	std::getline(stream, line);

	char type;
	int idx, dir;

	if (sscanf_s(line.c_str(), "E %d %d", &idx, &dir) == 2) {
		engine->onEncoderTurn(idx, dir);
	}
	else if (sscanf_s(line.c_str(), "B %d", &idx) == 1) {
		engine->onEncoderButton(idx);
	}

	asyncRead();
}


void EventHandler::runSerial() {
	io.run(); 
}

