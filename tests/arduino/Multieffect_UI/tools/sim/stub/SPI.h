// symulator PC: SPI -> emulacja ILI9486 (patrz sim.cpp)
#pragma once
#include <stdint.h>
#define SPI_MODE0 0
#define MSBFIRST 1
#define SPI_CLOCK_DIV2 2
#define SPI_CLOCK_DIV8 8
struct SPIClass {
  uint8_t transfer(uint8_t b);
  void begin() {}
  void setDataMode(int) {}
  void setBitOrder(int) {}
  void setClockDivider(int) {}
};
extern SPIClass SPI;
