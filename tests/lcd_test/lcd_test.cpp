// Test wyświetlacza Waveshare 4inch TFT Touch Shield (13587): ILI9486, 480x320, SPI.
//
// Protokół przepisany z demo Waveshare (STM32/XNUCLEO-F103RB, LCD_Driver.c):
// shield ma rejestry przesuwne 74HC4094 (SPI -> 16 bit równolegle), dlatego
//  - komenda: DC=0, 1 bajt,
//  - każdy bajt danych: DC=1, wysyłany jako 16 bit (0x00, bajt),
//  - piksel: DC=1, 16 bit RGB565,
//  - CS w górę po każdej transakcji (zeruje licznik 74HC4040 na shieldzie).
#include "daisy_seed.h"
#include "util/oled_fonts.h"

using namespace daisy;

// --- Piny Daisy (shield podpięty przewodami; w nawiasie fizyczny pin Seeda) --
// SPI1 na domyślnych pinach Daisy; TP_IRQ (pin 1) i MISO (pin 10) tu nieużywane
static constexpr Pin kTpCs  = seed::D1;    // (2)  -> shield D4  (TP_CS), trzymany wysoko
static constexpr Pin kSdCs  = seed::D2;    // (3)  -> shield D5  (SD_CS), trzymany wysoko
static constexpr Pin kLcdDc = seed::D3;    // (4)  -> shield D7  (LCD_DC)
static constexpr Pin kLcdRst = seed::D4;   // (5)  -> shield D8  (LCD_RST)
static constexpr Pin kLcdBl = seed::D5;    // (6)  -> shield D9  (LCD_BL)
static constexpr Pin kLcdCs = seed::D7;    // (8)  -> shield D10 (LCD_CS)
static constexpr Pin kSclk  = seed::D8;    // (9)  -> shield D13 (SCLK)
static constexpr Pin kMosi  = seed::D10;   // (11) -> shield D11 (MOSI)

static constexpr int kWidth  = 480;
static constexpr int kHeight = 320;

// kolory RGB565
static constexpr uint16_t BLACK  = 0x0000;
static constexpr uint16_t WHITE  = 0xFFFF;
static constexpr uint16_t RED    = 0xF800;
static constexpr uint16_t GREEN  = 0x07E0;
static constexpr uint16_t BLUE   = 0x001F;
static constexpr uint16_t YELLOW = 0xFFE0;
static constexpr uint16_t CYAN   = 0x07FF;

static DaisySeed hw;
static SpiHandle spi;
static GPIO      lcdCs, lcdDc, lcdRst, lcdBl, tpCs, sdCs;

// --- Niski poziom ------------------------------------------------------

static void spiWrite(uint8_t* buf, size_t len)
{
	spi.BlockingTransmit(buf, len, 1000);
}

static void writeReg(uint8_t reg)
{
	lcdDc.Write(false);
	lcdCs.Write(false);
	spiWrite(&reg, 1);
	lcdCs.Write(true);
}

static void writeData(uint8_t data)
{
	uint8_t buf[2] = { 0x00, data };
	lcdDc.Write(true);
	lcdCs.Write(false);
	spiWrite(buf, 2);
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

// wysyła ten sam kolor count razy, porcjami przez mały bufor
static void writeColor(uint16_t color, uint32_t count)
{
	static uint8_t buf[512];
	for (size_t i = 0; i < sizeof(buf); i += 2) {
		buf[i]     = color >> 8;
		buf[i + 1] = color & 0xFF;
	}

	lcdDc.Write(true);
	lcdCs.Write(false);
	while (count > 0) {
		uint32_t n = count > sizeof(buf) / 2 ? sizeof(buf) / 2 : count;
		spiWrite(buf, n * 2);
		count -= n;
	}
	lcdCs.Write(true);
}

// --- Inicjalizacja (sekwencja 1:1 z demo Waveshare) ----------------------

static void lcdInit()
{
	lcdRst.Write(true);
	System::Delay(500);
	lcdRst.Write(false);
	System::Delay(500);
	lcdRst.Write(true);
	System::Delay(500);

	lcdBl.Write(true);

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

	// orientacja pozioma 480x320 — D2U_L2R z demo Arduino (tak samo jak tests/ui_daisy)
	writeReg(0xB6); writeData(0x00); writeData(0x62);
	writeReg(0x36); writeData(0x28);
	System::Delay(200);

	writeReg(0x11);   // sleep out
	System::Delay(120);
	writeReg(0x29);   // display on
}

// --- Rysowanie ---------------------------------------------------------

static void fillRect(int x, int y, int w, int h, uint16_t color)
{
	if (w <= 0 || h <= 0) return;
	setWindow(x, y, x + w, y + h);
	writeColor(color, (uint32_t)w * h);
}

static void fillScreen(uint16_t color)
{
	fillRect(0, 0, kWidth, kHeight, color);
}

// znak fontem z libDaisy (te same fonty co na OLED), powiększony scale razy
static void drawChar(int x, int y, char ch, const FontDef& font, uint16_t fg, uint16_t bg, int scale)
{
	if (ch < 32 || ch > 126) ch = '?';
	for (int row = 0; row < font.FontHeight; row++) {
		uint16_t bits = font.data[(ch - 32) * font.FontHeight + row];
		for (int col = 0; col < font.FontWidth; col++) {
			uint16_t c = ((bits << col) & 0x8000) ? fg : bg;
			fillRect(x + col * scale, y + row * scale, scale, scale, c);
		}
	}
}

static void drawText(int x, int y, const char* s, const FontDef& font, uint16_t fg, uint16_t bg, int scale = 1)
{
	for (; *s; s++) {
		drawChar(x, y, *s, font, fg, bg, scale);
		x += font.FontWidth * scale;
	}
}

// --- Program -----------------------------------------------------------

int main(void)
{
	hw.Init();
	hw.StartLog(false);

	// CS dotyku i karty SD wysoko — inaczej pływają na wspólnej magistrali
	tpCs.Init(kTpCs, GPIO::Mode::OUTPUT);
	tpCs.Write(true);
	sdCs.Init(kSdCs, GPIO::Mode::OUTPUT);
	sdCs.Write(true);

	lcdCs.Init(kLcdCs, GPIO::Mode::OUTPUT);
	lcdCs.Write(true);
	lcdDc.Init(kLcdDc, GPIO::Mode::OUTPUT);
	lcdRst.Init(kLcdRst, GPIO::Mode::OUTPUT);
	lcdBl.Init(kLcdBl, GPIO::Mode::OUTPUT);

	SpiHandle::Config cfg;
	cfg.periph         = SpiHandle::Config::Peripheral::SPI_1;
	cfg.mode           = SpiHandle::Config::Mode::MASTER;
	cfg.direction      = SpiHandle::Config::Direction::TWO_LINES_TX_ONLY;   // MISO niepotrzebne
	cfg.nss            = SpiHandle::Config::NSS::SOFT;
	cfg.datasize       = 8;
	cfg.clock_polarity = SpiHandle::Config::ClockPolarity::LOW;    // tryb 0, jak w demo
	cfg.clock_phase    = SpiHandle::Config::ClockPhase::ONE_EDGE;
	cfg.baud_prescaler = SpiHandle::Config::BaudPrescaler::PS_4;   // 25 MHz / 4 = 6.25 MHz
	cfg.pin_config.sclk = kSclk;
	cfg.pin_config.mosi = kMosi;
	cfg.pin_config.miso = Pin();
	cfg.pin_config.nss  = Pin();
	spi.Init(cfg);

	hw.PrintLine("LCD test - init");
	lcdInit();
	hw.PrintLine("LCD test - init gotowy");

	while (1)
	{
		fillScreen(RED);
		System::Delay(500);
		fillScreen(GREEN);
		System::Delay(500);
		fillScreen(BLUE);
		System::Delay(500);

		fillScreen(BLACK);
		fillRect(0, 0, kWidth, 40, CYAN);
		drawText(10, 7, "Multieffect Daisy", Font_16x26, BLACK, CYAN);
		fillRect(20, 70, 140, 100, RED);
		fillRect(170, 70, 140, 100, GREEN);
		fillRect(320, 70, 140, 100, BLUE);
		drawText(20, 200, "ILI9486 480x320", Font_11x18, WHITE, BLACK, 2);
		drawText(20, 260, "lewy gorny rog = (0,0)", Font_7x10, YELLOW, BLACK, 2);
		hw.PrintLine("LCD test - klatka narysowana");
		System::Delay(5000);
	}
}
