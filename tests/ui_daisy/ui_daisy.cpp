// GUI multiefektu na Daisy Seed + Waveshare 4inch TFT Touch Shield (ILI9486 480x320, XPT2046).
// Kod GUI wspólny z symulatorem: ui/ (ten plik = platforma, patrz ui/platform.h).
//
// Audio: przelot wejście -> wyjście z master volume z GUI; z niego prawdziwy
// miernik wejścia i obciążenie CPU. Tuner na razie z ui/demo (efekt Tuner
// jeszcze nieprzeniesiony).
//
// Program w trybie BOOT_SRAM (bootloader Daisy) — grafiki i fonty nie mieszczą
// się w 128 KB wewnętrznego flasha, a QSPI musi być wolne do zapisu presetów.
#include "daisy_seed.h"
#include "util/scopedirqblocker.h"
#include "ui.h"
#include "gfx.h"
#include "platform.h"
#include "demo.h"
#include <math.h>
#include <string.h>

using namespace daisy;

// --- Piny Daisy (shield podpięty przewodami; w nawiasie fizyczny pin Seeda) --
static constexpr Pin kTpIrq  = seed::D0;    // (1)  -> shield D3  (TP_IRQ, niski = dotyk)
static constexpr Pin kTpCs   = seed::D1;    // (2)  -> shield D4  (TP_CS)
static constexpr Pin kSdCs   = seed::D2;    // (3)  -> shield D5  (SD_CS), trzymany wysoko
static constexpr Pin kLcdDc  = seed::D3;    // (4)  -> shield D7  (LCD_DC)
static constexpr Pin kLcdRst = seed::D4;    // (5)  -> shield D8  (LCD_RST)
static constexpr Pin kLcdBl  = seed::D5;    // (6)  -> shield D9  (LCD_BL)
static constexpr Pin kLcdCs  = seed::D7;    // (8)  -> shield D10 (LCD_CS)
static constexpr Pin kSclk   = seed::D8;    // (9)  -> shield D13 (SCLK)
static constexpr Pin kMiso   = seed::D9;    // (10) -> shield D12 (MISO, dotyk)
static constexpr Pin kMosi   = seed::D10;   // (11) -> shield D11 (MOSI)

// enkodery pod ekranem, od lewej; w nawiasie fizyczne piny Seeda (pull-upy wewnętrzne)
//                                  CLK (A)    DT (B)     SW
static const Pin kEncPins[3][3] = { { seed::D18, seed::D19, seed::D20 },    // E1: (25, 26, 27)
                                    { seed::D21, seed::D24, seed::D25 },    // E2: (28, 31, 32)
                                    { seed::D26, seed::D27, seed::D28 } };  // E3: (33, 34, 35)

static DaisySeed hw;
static SpiHandle spi;
static GPIO      lcdCs, lcdDc, lcdRst, lcdBl, tpCs, tpIrq, sdCs;
static CpuLoadMeter cpuMeter;

static DSY_SDRAM_BSS uint16_t framebuffer[gfx::W * gfx::H];

// --- SPI: ekran szybko, dotyk (XPT2046, max ~2 MHz) wolno -----------------

enum class SpiSpeed { None, Lcd, Touch };
static SpiSpeed spiSpeed = SpiSpeed::None;

static void spiUse(SpiSpeed speed)
{
	if (speed == spiSpeed) return;
	SpiHandle::Config cfg;
	cfg.periph         = SpiHandle::Config::Peripheral::SPI_1;
	cfg.mode           = SpiHandle::Config::Mode::MASTER;
	cfg.direction      = SpiHandle::Config::Direction::TWO_LINES;
	cfg.nss            = SpiHandle::Config::NSS::SOFT;
	cfg.datasize       = 8;
	cfg.clock_polarity = SpiHandle::Config::ClockPolarity::LOW;    // tryb 0, jak w demo Waveshare
	cfg.clock_phase    = SpiHandle::Config::ClockPhase::ONE_EDGE;
	cfg.baud_prescaler = speed == SpiSpeed::Lcd ? SpiHandle::Config::BaudPrescaler::PS_2     // 12.5 MHz
	                                            : SpiHandle::Config::BaudPrescaler::PS_16;   // 1.56 MHz
	cfg.pin_config.sclk = kSclk;
	cfg.pin_config.miso = kMiso;
	cfg.pin_config.mosi = kMosi;
	cfg.pin_config.nss  = Pin();
	spi.Init(cfg);
	spiSpeed = speed;
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

	// poziomo 480x320 — kierunek D2U_L2R z demo Arduino (do niego są
	// domyślne współczynniki kalibracji dotyku)
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

static constexpr int kBlSteps = 40;
static constexpr uint32_t kHoldMs = 500;
static TimerHandle ctrlTimer;
static volatile int blDuty = kBlSteps;

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
		if (enc[i].Pressed() && !holdSent[i] && enc[i].TimeHeldMs() > kHoldMs) {
			holdSent[i] = true;
			encHolds[i]++;
		}
		if (enc[i].FallingEdge() && !holdSent[i]) encPresses[i]++;
	}
}

static void controlTick(void*)
{
	static int phase = 0, encDiv = 0;
	if (++phase >= kBlSteps) phase = 0;
	lcdBl.Write(phase < blDuty);
	if (++encDiv >= 16) {
		encDiv = 0;
		readEncoders();
	}
}

static void controlInit()
{
	lcdBl.Init(kLcdBl, GPIO::Mode::OUTPUT);
	lcdBl.Write(true);
	for (int i = 0; i < 3; i++) enc[i].Init(kEncPins[i][0], kEncPins[i][1], kEncPins[i][2]);

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

// --- dotyk XPT2046 (odczyt jak TP_Read_ADC w demo Waveshare) ---------------

static uint16_t touchAdc(uint8_t cmd)
{
	uint8_t tx[3] = { cmd, 0, 0 }, rx[3] = { 0, 0, 0 };
	tpCs.Write(false);
	spi.BlockingTransmitAndReceive(tx, rx, 3, 100);
	tpCs.Write(true);
	return ((rx[1] << 8) | rx[2]) >> 3;   // 12 bit
}

// 5 odczytów, bez min i max, średnia
static uint16_t touchAdcAvg(uint8_t cmd)
{
	uint16_t v[5];
	for (auto& s : v) s = touchAdc(cmd);
	for (int i = 0; i < 4; i++)
		for (int j = i + 1; j < 5; j++)
			if (v[j] < v[i]) { uint16_t t = v[i]; v[i] = v[j]; v[j] = t; }
	return (v[1] + v[2] + v[3]) / 3;
}

static bool touchRead(int* x, int* y)
{
	if (tpIrq.Read()) return false;   // IRQ wysoko = brak dotyku

	spiUse(SpiSpeed::Touch);
	uint16_t xad = touchAdcAvg(0xD0);
	uint16_t yad = touchAdcAvg(0x90);
	if (!tpIrq.Read() && xad > 50 && yad > 50) {
		// domyślne współczynniki Waveshare dla D2U_L2R (TP_GetAdFac w LCD_Touch.cpp)
		*x = 480 - (int)(-0.132443f * yad) - 516;
		*y = 320 - (int)(0.089997f * xad) + 22;
		return *x >= 0 && *x < gfx::W && *y >= 0 && *y < gfx::H;
	}
	return false;
}

// --- audio: przelot z master volume, pomiar szczytu i CPU ------------------

static volatile float masterGain = 0.8f;
static volatile float peakUi = 0, peakTuner = 0;

static void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	cpuMeter.OnBlockStart();
	float g = masterGain, peak = 0;
	for (size_t i = 0; i < size; i++) {
		float l = in[0][i], r = in[1][i];
		float a = fabsf(l);
		if (a > peak) peak = a;
		out[0][i] = l * g;
		out[1][i] = r * g;
	}
	if (peak > peakUi) peakUi = peak;
	if (peak > peakTuner) peakTuner = peak;
	cpuMeter.OnBlockEnd();
}

// --- platforma dla ui/ --------------------------------------------------------

static PersistentStorage<Stored> storage(hw.qspi);
static constexpr uint32_t kStorageOffset = 0x700000;   // 7 MB w QSPI, daleko za programem

uint32_t plat_millis() { return System::GetNow(); }

void plat_lcd_write(int x0, int y0, int x1, int y1, const uint16_t* fb)
{
	static uint8_t buf[4096];
	spiUse(SpiSpeed::Lcd);
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
	blDuty = 2 + percent * (kBlSteps - 2) / 100;   // minimum 5 %, żeby ekran nie zgasł całkiem
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

int plat_tap(int, float) { return -1; }   // bez efektów

bool plat_tuner(float* hz, bool* demo)
{
	float p = peakTuner;
	peakTuner = 0;
	*demo = true;
	return demo_tuner(System::GetNow(), p, hz);
}

// --- start ---------------------------------------------------------------

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(48);
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

	// CS dotyku i karty SD wysoko — inaczej pływają na wspólnej magistrali
	tpCs.Init(kTpCs, GPIO::Mode::OUTPUT);
	tpCs.Write(true);
	sdCs.Init(kSdCs, GPIO::Mode::OUTPUT);
	sdCs.Write(true);
	tpIrq.Init(kTpIrq, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
	lcdCs.Init(kLcdCs, GPIO::Mode::OUTPUT);
	lcdCs.Write(true);
	lcdDc.Init(kLcdDc, GPIO::Mode::OUTPUT);
	lcdRst.Init(kLcdRst, GPIO::Mode::OUTPUT);

	controlInit();
	spiUse(SpiSpeed::Lcd);
	lcdInit();

	Stored empty;
	memset(&empty, 0, sizeof(empty));   // magic = 0 -> ui_init wgra presety fabryczne
	storage.Init(empty, kStorageOffset);

	cpuMeter.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
	ui_init(framebuffer);
	hw.StartAudio(AudioCallback);

	bool pressed = false;
	int tx = 0, ty = 0;
	uint32_t lastTouch = 0;
	while (1) {
		uint32_t now = System::GetNow();
		if (now - lastTouch >= 10) {
			lastTouch = now;
			int x, y;
			pressed = touchRead(&x, &y);
			if (pressed) {
				tx = x;
				ty = y;
			}
		}
		ui_touch(pressed, tx, ty);
		pollEncoders();
		ui_tick();
	}
}
