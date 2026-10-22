// symulator PC: minimum API Arduino używane przez szkic i sterownik Waveshare
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include "avr/pgmspace.h"
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define A0 14
#define F(s) (s)
using std::min;
using std::max;
void digitalWrite(int pin, int v);
int digitalRead(int pin);
inline void pinMode(int, int) {}
inline void analogWrite(int, int) {}
inline int analogRead(int) { return 0; }
unsigned long millis();
void delay(unsigned long ms);
inline long random(long a, long b) { return a + rand() % (b - a); }
inline void randomSeed(unsigned long s) { srand(s); }
struct SerialStub {
  void begin(long) {}
  template <class T> void print(T) {}
  template <class T> void println(T) {}
  void println() {}
};
extern SerialStub Serial;
void setup();
void loop();
