// Symulator GUI na PC: ten sam kod z ui/, platforma = framebuffer w RAM.
// Skrypt dotyku w scenario.h, zrzuty jako PPM (build.sh robi z nich PNG i PDF).
// Dodatkowo liczy, ile pikseli poszłoby po SPI do ekranu na Daisy.
#include "ui.h"
#include "ui_layout.h"
#include "gfx.h"
#include "platform.h"
#include <stdio.h>
#include <string>
#include <stdlib.h>

static uint16_t framebuffer[gfx::W * gfx::H];
static uint32_t simTime = 0;
static Stored   storage;            // pusta pamięć = pierwszy start (presety fabryczne)
static const char* outDir = ".";

// dane "z audio" ustawiane przez scenariusz
static float simPeak = 0.0f;
static float simTunerHz = 0.0f;     // 0 = brak sygnału
static float simCpu = 0.23f;

static unsigned long spiPixels = 0;

uint32_t plat_millis() { return simTime; }
void plat_lcd_write(int x0, int y0, int x1, int y1, const uint16_t*) { spiPixels += (unsigned long)(x1 - x0) * (y1 - y0); }
void plat_backlight(uint8_t) {}
void plat_load(void* dst, size_t size) { memcpy(dst, &storage, size); }
void plat_save(const void* src, size_t size) { memcpy(&storage, src, size); }
float plat_input_peak() { return simPeak; }
float plat_cpu_load() { return simCpu; }
void plat_set_master(float) {}
int  plat_tap(int, float) { return -1; }
bool plat_tuner(float* hz, bool* demo)
{
	*demo = false;
	*hz = simTunerHz;
	return simTunerHz > 0;
}

// --- skrypt ------------------------------------------------------------------
static void step(bool pressed, int x, int y)
{
	ui_touch(pressed, x, y);
	ui_tick();
	simTime += 10;
}

[[maybe_unused]] static void tap(int x, int y) { step(true, x, y); step(false, 0, 0); }
static void wait(int ms) { for (int t = 0; t < ms; t += 10) step(false, 0, 0); }

// enkodery: wolne kroki (bez przyspieszenia), wciśnięcia
static void enc(int i, int d)
{
	for (int k = 0; k < abs(d); k++) {
		ui_encoder(i, d > 0 ? 1 : -1);
		wait(150);
	}
}
static void push(int i) { ui_button(i, UI_PRESS); step(false, 0, 0); }
[[maybe_unused]] static void hold(int i) { ui_button(i, UI_HOLD); step(false, 0, 0); }

[[maybe_unused]] static void drag(int x0, int x1, int y)
{
	for (int i = 0; i <= 10; i++) step(true, x0 + (x1 - x0) * i / 10, y);
	step(false, 0, 0);
}

static void shot(const char* name)
{
	std::string path = std::string(outDir) + "/" + name + ".ppm";
	FILE* f = fopen(path.c_str(), "wb");
	fprintf(f, "P6 %d %d 255\n", gfx::W, gfx::H);
	for (int i = 0; i < gfx::W * gfx::H; i++) {
		uint16_t c = framebuffer[i];
		uint8_t rgb[3] = { (uint8_t)((c >> 11) << 3), (uint8_t)(((c >> 5) & 0x3F) << 2), (uint8_t)((c & 0x1F) << 3) };
		fwrite(rgb, 1, 3, f);
	}
	fclose(f);
	// SPI 12.5 MHz, 2 bajty na piksel
	printf("%-20s SPI od poprzedniego zrzutu: %6lu px = %5.1f ms\n", name, spiPixels, spiPixels * 16.0 / 12.5e3);
	spiPixels = 0;
}

#include "scenario.h"

int main(int argc, char** argv)
{
	if (argc > 1) outDir = argv[1];
	ui_init(framebuffer);
	scenario();
	return 0;
}
