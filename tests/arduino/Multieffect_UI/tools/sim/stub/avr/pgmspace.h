// symulator PC: PROGMEM = zwykła pamięć
#pragma once
#include <stdint.h>
#include <string.h>
#define PROGMEM
#define pgm_read_byte(p) (*(const uint8_t *)(p))
#define pgm_read_word(p) (*(const uint16_t *)(p))
#define strcpy_P strcpy
#define memcpy_P memcpy
