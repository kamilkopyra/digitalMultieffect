// symulator PC: EEPROM 1 KB w RAM (pusta = 0xFF jak nowy układ)
#pragma once
#include <stdint.h>
#include <string.h>
struct EEPROMClass {
  uint8_t mem[1024];
  EEPROMClass() { memset(mem, 0xFF, sizeof(mem)); }
  uint8_t read(int i) { return mem[i]; }
  void write(int i, uint8_t v) { mem[i] = v; }
  void update(int i, uint8_t v) { mem[i] = v; }
  template <class T> T &get(int i, T &t) { memcpy(&t, mem + i, sizeof(T)); return t; }
  template <class T> const T &put(int i, const T &t) { memcpy(mem + i, &t, sizeof(T)); return t; }
};
static EEPROMClass EEPROM;
