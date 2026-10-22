#include "demo.h"
#include <math.h>

static uint32_t seed = 12345;

static int rnd(int lo, int hi)   // [lo, hi)
{
	seed = seed * 1103515245u + 12345u;
	return lo + (int)((seed >> 16) % (uint32_t)(hi - lo));
}

static uint32_t pluckStart = 0;
static float    pluckPeak = 0;

float demo_input_peak(uint32_t now)
{
	if (now - pluckStart > 2200) {
		pluckStart = now;
		pluckPeak = rnd(0, 4) == 0 ? 1.05f : rnd(25, 70) / 100.0f;   // czasem przester
	}
	float t = (now - pluckStart) / 1000.0f;
	return pluckPeak * expf(-t * 2.2f);
}

static const float kStrings[6] = { 82.41f, 110.0f, 146.83f, 196.0f, 246.94f, 329.63f };
static bool  wasValid = false;
static int   string = 0;
static float cents = 0;
static uint32_t lastStep = 0;

bool demo_tuner(uint32_t now, float peak, float* hz)
{
	bool valid = peak > 0.03f;
	if (valid && !wasValid) {          // nowe szarpnięcie
		string = rnd(0, 6);
		cents = (float)rnd(-40, 41);
		lastStep = now;
	} else if (valid && now - lastStep >= 100) {
		cents = cents * 0.72f + rnd(-30, 31) / 10.0f;   // "dokręcanie" do zera
		lastStep = now;
	}
	wasValid = valid;
	if (valid) *hz = kStrings[string] * powf(2.0f, cents / 1200.0f);
	return valid;
}
