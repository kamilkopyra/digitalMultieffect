#pragma once
#include "AudioEngine.h"
#include <iostream>
#include <boost/asio.hpp>
#include <string.h>

class EventHandler
{
	

public:
	EventHandler(AudioEngine* e) {
		engine = e;
		pot = e->pot;
	}
	void handleKey(char key);
	void run();
	bool openSerial(const std::string& portName, int baudRate = 9600);
	void runSerial(); // wywołaj w osobnym wątku

private:
	int* pot;
	AudioEngine* engine;
	boost::asio::io_context io;
	boost::asio::serial_port port{ io };
	boost::asio::streambuf serialBuffer;

	void asyncRead();
	void onLineReceived(const boost::system::error_code& ec, std::size_t bytes);

};

