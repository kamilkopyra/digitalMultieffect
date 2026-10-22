// Multiefekt na Daisy Seed + Waveshare 4inch TFT Touch Shield (ILI9486 480x320).
// GUI (ui/) steruje łańcuchem efektów (core/EffectChain): 3 sloty, efekty z
// effects/. Sterowanie tylko enkoderami — dotyk shieldu nieużywany.
//
// Program w trybie BOOT_SRAM (bootloader Daisy) — grafiki i fonty nie mieszczą
// się w 128 KB wewnętrznego flasha, a QSPI musi być wolne do zapisu presetów.
#include "daisy_seed.h"
#include "util/scopedirqblocker.h"

#include "ui.h"
#include "gfx.h"
#include "platform.h"
#include "demo.h"

#include "EffectChain.h"
#include "NoiseGate.h"
#include "Compressor.h"
#include "TubeScreamer.h"
#include "Rat.h"
#include "Fuzz.h"
#include "Boost.h"
#include "Equalizer.h"
#include "AutoWah.h"
#include "Phaser.h"
#include "Flanger.h"
#include "Vibrato.h"
#include "Tremolo.h"
#include "StubEffect.h"

#include <math.h>
#include <string.h>

using namespace daisy;

// --- Piny Daisy (shield podpięty przewodami; w nawiasie fizyczny pin Seeda) --
static constexpr Pin TpCs   = seed::D1;    // (2)  -> shield D4  (TP_CS), dotyk nieużywany — trzymany wysoko
static constexpr Pin SdCs   = seed::D2;    // (3)  -> shield D5  (SD_CS), trzymany wysoko
static constexpr Pin LcdDc  = seed::D3;    // (4)  -> shield D7  (LCD_DC)
static constexpr Pin LcdRst = seed::D4;    // (5)  -> shield D8  (LCD_RST)
static constexpr Pin LcdBl  = seed::D5;    // (6)  -> shield D9  (LCD_BL)
static constexpr Pin LcdCs  = seed::D7;    // (8)  -> shield D10 (LCD_CS)
static constexpr Pin Sclk   = seed::D8;    // (9)  -> shield D13 (SCLK)
static constexpr Pin Miso   = seed::D9;    // (10) -> shield D12 (MISO)
static constexpr Pin Mosi   = seed::D10;   // (11) -> shield D11 (MOSI)

// enkodery pod ekranem, od lewej; w nawiasie fizyczne piny Seeda (pull-upy wewnętrzne)
//                                 CLK (A)    DT (B)     SW
static const Pin EncPins[3][3] = { { seed::D18, seed::D19, seed::D20 },    // E1: (25, 26, 27)
                                   { seed::D21, seed::D24, seed::D25 },    // E2: (28, 31, 32)
                                   { seed::D26, seed::D27, seed::D28 } };  // E3: (33, 34, 35)

static DaisySeed hw;
static SpiHandle spi;
static GPIO      lcdCs, lcdDc, lcdRst, lcdBl, tpCs, sdCs;
static CpuLoadMeter cpuMeter;

static DSY_SDRAM_BSS uint16_t framebuffer[gfx::W * gfx::H];

// --- efekty ---------------------------------------------------------------

static EffectChain  chain;

// pula efektów — kolejność jak w GUI (ui.cpp kEffects), indeksy = Preset.effect[]
static Fuzz         fuzz;
static Tremolo      tremolo;
static NoiseGate    noiseGate;
static Rat          rat;
static TubeScreamer tubeScreamer;
static StubEffect   delayFx("Delay", "Time", "Feedback", "Mix");   // wydmuszka, bufor do SDRAM
static Compressor   compressor;
static Vibrato      vibrato;
static Flanger      flanger;
static Phaser       phaser;
static AutoWah      autoWah;
static Equalizer    equalizer;
static Boost        boost;

// stan GUI (efekty w slotach, włączniki, parametry) -> łańcuch; tanie, woła się co pętlę
static void syncChain()
{
	const Preset& p = ui_current();
	constexpr int n = EffectChain::numSlots;

	// najpierw zwolnij sloty, w których zmienia się efekt — inaczej zamiana
	// efektów między slotami (np. przy wczytaniu presetu) by się zablokowała
	for (int s = 0; s < n; s++)
		if (chain.slotEffect(s) != p.effect[s]) chain.setSlot(s, -1);
	for (int s = 0; s < n; s++)
		if (chain.slotEffect(s) != p.effect[s]) chain.setSlot(s, p.effect[s]);   // ten sam efekt 2x: slot zostaje pusty

	for (int s = 0; s < n; s++) {
		chain.setEnabled(s, p.enabled & (1 << s));
		Effect* e = chain.slot(s);
		if (!e || chain.slotEffect(s) != p.effect[s]) continue;
		for (int k = 0; k < 3; k++)
			if (e->getParam(k) != p.params[s][k]) e->setParam(k, p.params[s][k]);
	}
}

// --- SPI ekranu ---------------------------------------------------------

static void spiInit()
{
	SpiHandle::Config cfg;
	cfg.periph         = SpiHandle::Config::Peripheral::SPI_1;
	cfg.mode           = SpiHandle::Config::Mode::MASTER;
	cfg.direction      = SpiHandle::Config::Direction::TWO_LINES;
	cfg.nss            = SpiHandle::Config::NSS::SOFT;
	cfg.datasize       = 8;
	cfg.clock_polarity = SpiHandle::Config::ClockPolarity::LOW;    // tryb 0, jak w demo Waveshare
	cfg.clock_phase    = SpiHandle::Config::ClockPhase::ONE_EDGE;
	cfg.baud_prescaler = SpiHandle::Config::BaudPrescaler::PS_2;   // 12.5 MHz
	cfg.pin_config.sclk = Sclk;
	cfg.pin_config.miso = Miso;
	cfg.pin_config.mosi = Mosi;
	cfg.pin_config.nss  = Pin();
	spi.Init(cfg);
}

// --- ILI9486 przez rejestry przesuwne shieldu (jak tests/lcd_test) --------
// komenda: DC=0, 1 bajt; dane: DC=1, po 2 bajty (0x00, bajt); piksel: 16 bit

static void writeReg(uint8_t reg)
{
	lcdDc.Write(false);
	lcdCs.Write(false);
	spi.BlockingTransmit(&reg, 1, 100);
	lcdCs.Write(true);
}

static void writeData(uint8_t data)
{
	uint8_t buf[2] = { 0x00, data };
	lcdDc.Write(true);
	lcdCs.Write(false);
	spi.BlockingTransmit(buf, 2, 100);
	lcdCs.Write(true);
}

static void setWindow(int x0, int y0, int x1, int y1)   // x1, y1 wyłącznie
{
	writeReg(0x2A);
	writeData(x0 >> 8);
	writeData(x0 & 0xFF);
	writeData((x1 - 1) >> 8);
	writeData((x1 - 1) & 0xFF);

	writeReg(0x2B);
	writeData(y0 >> 8);
	writeData(y0 & 0xFF);
	writeData((y1 - 1) >> 8);
	writeData((y1 - 1) & 0xFF);

	writeReg(0x2C);
}

static void lcdInit()
{
	lcdRst.Write(true);
	System::Delay(100);
	lcdRst.Write(false);
	System::Delay(100);
	lcdRst.Write(true);
	System::Delay(100);

	// sekwencja 1:1 z demo Waveshare
	writeReg(0xF9); writeData(0x00); writeData(0x08);
	writeReg(0xC0); writeData(0x19); writeData(0x1A);
	writeReg(0xC1); writeData(0x45); writeData(0x00);
	writeReg(0xC2); writeData(0x33);
	writeReg(0xC5); writeData(0x00); writeData(0x28);
	writeReg(0xB1); writeData(0xA0); writeData(0x11);
	writeReg(0xB4); writeData(0x02);
	writeReg(0xB6); writeData(0x00); writeData(0x42); writeData(0x3B);
	writeReg(0xB7); writeData(0x07);

	static const uint8_t gammaP[15] = { 0x1F, 0x25, 0x22, 0x0B, 0x06, 0x0A, 0x4E, 0xC6,
	                                    0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static const uint8_t gammaN[15] = { 0x1F, 0x3F, 0x3F, 0x0F, 0x1F, 0x0F, 0x46, 0x49,
	                                    0x31, 0x05, 0x09, 0x03, 0x1C, 0x1A, 0x00 };
	writeReg(0xE0); for (uint8_t v : gammaP) writeData(v);
	writeReg(0xE1); for (uint8_t v : gammaN) writeData(v);

	static const uint8_t f1[8] = { 0x36, 0x04, 0x00, 0x3C, 0x0F, 0x0F, 0xA4, 0x02 };
	static const uint8_t f2[9] = { 0x18, 0xA3, 0x12, 0x02, 0x32, 0x12, 0xFF, 0x32, 0x00 };
	static const uint8_t f4[5] = { 0x40, 0x00, 0x08, 0x91, 0x04 };
	writeReg(0xF1); for (uint8_t v : f1) writeData(v);
	writeReg(0xF2); for (uint8_t v : f2) writeData(v);
	writeReg(0xF4); for (uint8_t v : f4) writeData(v);
	writeReg(0xF8); writeData(0x21); writeData(0x04);

	writeReg(0x3A); writeData(0x55);   // 16 bit / piksel (RGB565)

	// poziomo 480x320 — kierunek D2U_L2R z demo Arduino
	writeReg(0xB6); writeData(0x00); writeData(0x62);
	writeReg(0x36); writeData(0x28);
	System::Delay(50);

	writeReg(0x11);   // sleep out
	System::Delay(120);
	writeReg(0x29);   // display on
}

// --- przerwanie TIM5 16 kHz: podświetlenie + enkodery ------------------------
// Podświetlenie: programowy PWM (D5 = PD2 nie ma kanału timera), 40 poziomów -> 400 Hz.
// Enkodery czytane tu (co 16. wywołanie = 1 kHz), a nie w pętli głównej, bo
// przerysowanie ekranu blokuje pętlę nawet na ~200 ms i gubiłyby się kroki.

static constexpr int BlSteps = 40;
static constexpr uint32_t HoldMs = 500;
static TimerHandle ctrlTimer;
static volatile int blDuty = BlSteps;

static Encoder enc[3];
static volatile int encDelta[3];
static volatile int encPresses[3], encHolds[3];
static bool holdSent[3];

static void readEncoders()
{
	for (int i = 0; i < 3; i++) {
		enc[i].Debounce();
		encDelta[i] += enc[i].Increment();
		if (enc[i].RisingEdge()) holdSent[i] = false;
		if (enc[i].Pressed() && !holdSent[i] && enc[i].TimeHeldMs() > HoldMs) {
			holdSent[i] = true;
			encHolds[i]++;
		}
		if (enc[i].FallingEdge() && !holdSent[i]) encPresses[i]++;
	}
}

static void controlTick(void*)
{
	static int phase = 0, encDiv = 0;
	if (++phase >= BlSteps) phase = 0;
	lcdBl.Write(phase < blDuty);
	if (++encDiv >= 16) {
		encDiv = 0;
		readEncoders();
	}
}

static void controlInit()
{
	lcdBl.Init(LcdBl, GPIO::Mode::OUTPUT);
	lcdBl.Write(true);
	for (int i = 0; i < 3; i++) enc[i].Init(EncPins[i][0], EncPins[i][1], EncPins[i][2]);

	TimerHandle::Config cfg;
	cfg.periph     = TimerHandle::Config::Peripheral::TIM_5;   // TIM2 zajęty przez System (czas)
	cfg.dir        = TimerHandle::Config::CounterDir::UP;
	cfg.enable_irq = true;
	ctrlTimer.Init(cfg);
	ctrlTimer.SetPeriod(ctrlTimer.GetFreq() / 16000 - 1);
	// libDaisy włącza buforowanie okresu (auto-reload preload), a startowy okres to
	// 0xFFFFFFFF — bez wymuszonego przeładowania nowy okres wszedłby dopiero po
	// pierwszym przepełnieniu, czyli po ~21 s (enkodery i podświetlenie martwe)
	TIM5->EGR = TIM_EGR_UG;
	ctrlTimer.SetCallback(controlTick);
	ctrlTimer.Start();
}

// zebrane w przerwaniu kroki i wciśnięcia -> GUI
static void pollEncoders()
{
	int delta[3], presses[3], holds[3];
	{
		ScopedIrqBlocker block;
		for (int i = 0; i < 3; i++) {
			delta[i] = encDelta[i];
			presses[i] = encPresses[i];
			holds[i] = encHolds[i];
			encDelta[i] = encPresses[i] = encHolds[i] = 0;
		}
	}
	for (int i = 0; i < 3; i++) {
		if (delta[i]) ui_encoder(i, delta[i]);
		for (int k = 0; k < presses[i]; k++) ui_button(i, UI_PRESS);
		for (int k = 0; k < holds[i]; k++) ui_button(i, UI_HOLD);
	}
}

// --- audio: łańcuch efektów + master volume, pomiar szczytu i CPU ----------

static volatile float masterGain = 0.8f;
static volatile float peakUi = 0, peakTuner = 0;

static void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	cpuMeter.OnBlockStart();
	float g = masterGain, peak = 0;
	for (size_t i = 0; i < size; i++) {
		// gitara na lewym kanale, wyjście na obu kanałach
		// jedno wyjście gitarowe, drugie wyjście słuchawkowe
		float x = in[0][i];
		float a = fabsf(x);
		if (a > peak) peak = a;
		float s = chain.process(x) * g;
		out[0][i] = s;
		out[1][i] = s;
	}
	if (peak > peakUi) peakUi = peak;
	if (peak > peakTuner) peakTuner = peak;
	cpuMeter.OnBlockEnd();
}

// --- platforma dla ui/ --------------------------------------------------------

static PersistentStorage<Stored> storage(hw.qspi);
static constexpr uint32_t StorageOffset = 0x700000;   // 7 MB w QSPI, daleko za programem

uint32_t plat_millis() { return System::GetNow(); }

void plat_lcd_write(int x0, int y0, int x1, int y1, const uint16_t* fb)
{
	static uint8_t buf[4096];
	setWindow(x0, y0, x1, y1);
	lcdDc.Write(true);
	lcdCs.Write(false);
	size_t n = 0;
	for (int y = y0; y < y1; y++) {
		const uint16_t* row = fb + y * gfx::W;
		for (int x = x0; x < x1; x++) {
			buf[n++] = row[x] >> 8;   // ekran chce starszy bajt pierwszy
			buf[n++] = row[x] & 0xFF;
			if (n == sizeof(buf)) {
				spi.BlockingTransmit(buf, n, 100);
				n = 0;
			}
		}
	}
	if (n) spi.BlockingTransmit(buf, n, 100);
	lcdCs.Write(true);
}

void plat_backlight(uint8_t percent)
{
	blDuty = 2 + percent * (BlSteps - 2) / 100;   // minimum 5 %, żeby ekran nie zgasł całkiem
}

void plat_load(void* dst, size_t size) { memcpy(dst, &storage.GetSettings(), size); }

void plat_save(const void* src, size_t size)
{
	memcpy(&storage.GetSettings(), src, size);
	storage.Save();
}

float plat_input_peak()
{
	float p = peakUi;
	peakUi = 0;
	return p;
}

float plat_cpu_load() { return cpuMeter.GetAvgCpuLoad(); }

void plat_set_master(float gain) { masterGain = gain; }

// efekt Tuner jeszcze nieprzeniesiony — ekran tunera pokazuje dane z ui/demo
bool plat_tuner(float* hz, bool* demo)
{
	float p = peakTuner;
	peakTuner = 0;
	*demo = true;
	return demo_tuner(System::GetNow(), p, hz);
}

int plat_tap(int slot, float ms)
{
	syncChain();   // na wypadek, gdyby efekt w slocie zmienił się w tej samej pętli
	Effect* e = chain.slot(slot);
	if (!e || !e->isModulation()) return -1;
	e->TapToParam(ms);
	return e->getParam(0);
}

// --- start ---------------------------------------------------------------

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(48);   // 1 ms przy 48 kHz
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

	// CS dotyku i karty SD wysoko — inaczej pływają na wspólnej magistrali
	tpCs.Init(TpCs, GPIO::Mode::OUTPUT);
	tpCs.Write(true);
	sdCs.Init(SdCs, GPIO::Mode::OUTPUT);
	sdCs.Write(true);
	lcdCs.Init(LcdCs, GPIO::Mode::OUTPUT);
	lcdCs.Write(true);
	lcdDc.Init(LcdDc, GPIO::Mode::OUTPUT);
	lcdRst.Init(LcdRst, GPIO::Mode::OUTPUT);

	controlInit();
	spiInit();
	lcdInit();

	// pula efektów — kolejność rejestracji = indeksy jak w GUI
	chain.add(&fuzz);           // 0
	chain.add(&tremolo);        // 1
	chain.add(&noiseGate);      // 2
	chain.add(&rat);            // 3
	chain.add(&tubeScreamer);   // 4
	chain.add(&delayFx);        // 5
	chain.add(&compressor);     // 6
	chain.add(&vibrato);        // 7
	chain.add(&flanger);        // 8
	chain.add(&phaser);         // 9
	chain.add(&autoWah);        // 10
	chain.add(&equalizer);      // 11
	chain.add(&boost);          // 12

	Stored empty;
	memset(&empty, 0, sizeof(empty));   // magic = 0 -> ui_init wgra presety fabryczne
	storage.Init(empty, StorageOffset);

	cpuMeter.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
	ui_init(framebuffer);
	syncChain();   // aktualny preset do łańcucha, zanim ruszy audio
	hw.StartAudio(AudioCallback);

	while (1) {
		pollEncoders();
		syncChain();
		ui_tick();
	}
}
