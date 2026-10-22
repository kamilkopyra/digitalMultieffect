// Multieffect UI — ekran pedału (Waveshare 4inch TFT Touch Shield 480x320 + Arduino UNO).
// Styl i kolory z wersji Qt (motyw ciemny, zielony akcent), grafiki pedałów z Qt (icons.h).
//
// Ekrany:
//   główny   — pasek: preset | miernik wejścia | VOL | tuner | ustawienia,
//              3 kafle efektów, panel efektu w fokusie (3 paski = 3 enkodery),
//              [Change effect] [TAP]
//   presety  — 8 presetów w EEPROM, Load / Save
//   tuner    — cały ekran
//   settings — jasność, tryb footswitchy, CPU, info, reset ustawień
//   wybór efektu
//
// Dotyk: kafel = fokus, ponowne dotknięcie kafla w fokusie = ON/OFF,
// VOL = fokus na master, pasek = ustaw wartość.
// Na UNO nie ma audio: miernik wejścia, tuner i CPU są symulowane.
// Rysuje tylko to, co się zmieniło.
//
// Sterownik ekranu/dotyku = pliki z demo Waveshare LCD_Touch bez zmian,
// poza dodanym TP_GetPoint() w LCD_Touch.cpp/.h.
// Podgląd bez Arduino: tools/sim (symulator na PC, robi PNG/PDF).
#include "DEV_Config.h"
#include "LCD_Driver.h"
#include "LCD_GUI.h"
#include "LCD_Touch.h"
#include <avr/pgmspace.h>
#include <EEPROM.h>
#include "icons.h"

#define FW_VERSION    "v0.1"

// --- kolory: odcienie szarości z Main.qml (darkMode) + akcenty ----------
// szarość n (0..31) w RGB565 bez zielonego zafarbu: r = n, g = 2n, b = n
#define SHADE(n)       (((n) << 11) | ((2 * (n)) << 5) | (n))
#define C_BG          SHADE(3)     // ~#1e1e1e bgColor
#define C_PANEL       SHADE(5)     // #2a2a2a panelColor
#define C_PANEL_HI    SHADE(7)     // górna krawędź gradientu
#define C_BORDER      SHADE(8)     // #444444 borderColor
#define C_TRACK       SHADE(2)     // #151515 trackColor
#define C_TEXT        0xFFFF      // textColor
#define C_MUTED       SHADE(21)    // #aaaaaa mutedTextColor
#define C_DIM         SHADE(13)    // #666666 dimTextColor
#define C_KNOB        SHADE(26)
#define C_GREEN       0x4D6A      // #4caf50 highlight
#define C_GREEN_DARK  0x1A44      // #1f4a22
#define C_GREEN_GLOW  0x8F11      // #8be08f
#define C_ORANGE      0xE506      // #e0a030 (Qt: CPU > 50 %, tuner nie w stroju)
#define C_RED         0xE208      // #e04040
#define C_RED_DARK    0x4904      // #482020

// tło fontu == FONT_BACKGROUND -> GUI_DisChar rysuje tylko piksele znaku
#define TRANSPARENT   FONT_BACKGROUND

// --- efekty: nazwy parametrów jak w wersji PC ---------------------------
struct EffectInfo {
  char name[13];
  char params[3][14];
  char category[11];
  bool modulation;     // tap tempo
};

const EffectInfo kEffects[] PROGMEM = {
  { "Fuzz",         { "Gain", "Volume", "Clip" },                         "distortion", false },
  { "Tremolo",      { "frequency", "depth", "None" },                     "modulation", true },
  { "NoiseGate",    { "threshold", "attack_coeff", "release_coeff" },     "dynamics",   false },
  { "Rat",          { "drive", "volume", "tone" },                        "distortion", false },
  { "TubeScreamer", { "drive", "volume", "tone" },                        "distortion", false },
  { "Delay",        { "Time", "Feedback", "Mix" },                        "time",       false },
  { "Compressor",   { "treshold", "ratio", "makeupGain" },                "dynamics",   false },
  { "Vibrato",      { "rate", "depth", "mix" },                           "modulation", true },
  { "Flanger",      { "rate", "depth", "feedback" },                      "modulation", true },
  { "Phaser",       { "rate", "depth", "feedback" },                      "modulation", true },
  { "AutoWah",      { "Sensitivity", "Resonance", "Mix" },                "filter",     false },
  { "Equalizer",    { "Low", "Mid", "High" },                             "filter",     false },
  { "Boost",        { "gain", "tone", "volume" },                         "gain",       false },
};
#define NUM_EFFECTS   (int)(sizeof(kEffects) / sizeof(kEffects[0]))

// --- stan ----------------------------------------------------------------
#define NUM_SLOTS     3
#define FOCUS_MASTER  NUM_SLOTS     // fokus na master volume zamiast efektu

// preset = to, co jest w EEPROM i w presetach fabrycznych
struct Preset {
  char    name[9];
  int8_t  effect[NUM_SLOTS];        // indeks w kEffects
  uint8_t enabled;                  // bit i = slot i ON
  uint8_t params[NUM_SLOTS][3];     // 0..100
};

#define NUM_PRESETS   8

const Preset kFactoryPresets[NUM_PRESETS] PROGMEM = {
  { "Clean",   { 6, 11, 5 },  0b011, { { 40, 30, 55 }, { 50, 45, 60 }, { 35, 30, 25 } } },
  { "Crunch",  { 4, 12, 5 },  0b001, { { 55, 60, 50 }, { 40, 50, 60 }, { 30, 25, 20 } } },
  { "Lead",    { 3, 12, 5 },  0b111, { { 70, 55, 60 }, { 60, 55, 65 }, { 45, 40, 30 } } },
  { "Ambient", { 6, 7, 5 },   0b110, { { 35, 30, 50 }, { 25, 40, 50 }, { 70, 65, 55 } } },
  { "Funk",    { 6, 10, 9 },  0b011, { { 50, 45, 55 }, { 65, 60, 70 }, { 40, 50, 30 } } },
  { "Fuzz",    { 2, 0, 1 },   0b011, { { 30, 40, 50 }, { 80, 55, 70 }, { 45, 60, 50 } } },
  { "Surf",    { 1, 11, 5 },  0b111, { { 60, 70, 50 }, { 45, 55, 70 }, { 30, 35, 40 } } },
  { "Default", { 4, 1, 5 },   0b000, { { 50, 50, 50 }, { 50, 50, 50 }, { 50, 50, 50 } } },
};

static Preset cur;                  // aktualny stan łańcucha
static int8_t  curPreset = 0;
static bool    modified = false;    // zmiany od ostatniego Load/Save
static int8_t  focused = 0;
static uint8_t master = 80;
static uint8_t brightness = 80;     // %
static uint8_t fsMode = 0;          // 0 = footswitche przełączają efekty, 1 = presety
static uint16_t bpm = 120;
static unsigned long lastTap = 0;

// --- EEPROM ----------------------------------------------------------------
#define EE_MAGIC      0xA6
#define EE_ADDR_MAGIC 0
#define EE_ADDR_BRIGHT 1
#define EE_ADDR_FS    2
#define EE_ADDR_MASTER 3
#define EE_ADDR_CUR   4
#define EE_ADDR_PRESET(i) (16 + (i) * sizeof(Preset))

static void readPreset(int i, Preset &p)  { EEPROM.get(EE_ADDR_PRESET(i), p); }
static void writePreset(int i, const Preset &p) { EEPROM.put(EE_ADDR_PRESET(i), p); }

static void saveSettings()
{
  EEPROM.update(EE_ADDR_BRIGHT, brightness);
  EEPROM.update(EE_ADDR_FS, fsMode);
  EEPROM.update(EE_ADDR_MASTER, master);
  EEPROM.update(EE_ADDR_CUR, curPreset);
}

static void loadPreset(int i)
{
  readPreset(i, cur);
  curPreset = i;
  modified = false;
  EEPROM.update(EE_ADDR_CUR, curPreset);
}

static void factoryReset()
{
  Preset p;
  for (int i = 0; i < NUM_PRESETS; i++) {
    memcpy_P(&p, &kFactoryPresets[i], sizeof(Preset));
    writePreset(i, p);
  }
  brightness = 80;
  fsMode = 0;
  master = 80;
  curPreset = 0;
  saveSettings();
  EEPROM.update(EE_ADDR_MAGIC, EE_MAGIC);
  loadPreset(0);
}

static void applyBrightness()
{
  PWM_SetValue(10 + (uint16_t)brightness * 245 / 100);
}

// --- układ ekranu ------------------------------------------------------
#define SCR_W         480
#define SCR_H         320

// pasek górny (ekran główny)
#define TOP_Y0        4
#define TOP_Y1        30
#define PRE_X0        6
#define PRE_X1        186
#define IN_X0         192
#define IN_X1         316
#define VOL_X0        322
#define VOL_X1        398
#define TUN_X0        404
#define TUN_X1        436
#define SET_X0        442
#define SET_X1        474

#define TILE_Y0       36
#define TILE_Y1       102
#define TILE_W        152
#define TILE_GAP      8

#define FX_X0         6
#define FX_X1         474
#define FX_Y0         112      // panel efektu w fokusie
#define FX_Y1         266
#define BAR_X0        124
#define BAR_X1        318
#define BAR_H         10
#define BAR_Y(p)      (FX_Y0 + 76 + (p) * 28)
#define VAL_X1        364      // prawa krawędź wartości
#define ICON_SCALE    2
#define ICON_X0       (FX_X1 - 12 - ICON_W * ICON_SCALE)
#define ICON_Y0       (FX_Y0 + (FX_Y1 - FX_Y0 - ICON_H * ICON_SCALE) / 2)

#define BTN_Y0        272
#define BTN_Y1        314
#define BTN_L_X0      6
#define BTN_L_X1      236
#define BTN_R_X0      244
#define BTN_R_X1      474

// nagłówek podekranów
#define BACK_X0       384
#define BACK_X1       474

// --- pomocnicze rysowanie ------------------------------------------------

static void fill(int x0, int y0, int x1, int y1, COLOR c)   // x1, y1 wyłącznie
{
  LCD_SetArealColor(x0, y0, x1, y1, c);
}

static COLOR lerp(COLOR a, COLOR b, int t, int n)   // t z 0..n
{
  int r = ((a >> 11) * (n - t) + (b >> 11) * t) / n;
  int g = (((a >> 5) & 0x3F) * (n - t) + ((b >> 5) & 0x3F) * t) / n;
  int bl = ((a & 0x1F) * (n - t) + (b & 0x1F) * t) / n;
  return (r << 11) | (g << 5) | bl;
}

// prostokąt z zaokrąglonymi rogami (r = 4) i pionowym gradientem top -> bottom
static const uint8_t kCorner[4] = { 4, 2, 1, 1 };

static void roundRect(int x0, int y0, int x1, int y1, COLOR top, COLOR bottom)
{
  int h = y1 - y0;
  for (int y = 0; y < h; y++) {
    int in = y < 4 ? kCorner[y] : (h - 1 - y < 4 ? kCorner[h - 1 - y] : 0);
    fill(x0 + in, y0 + y, x1 - in, y0 + y + 1, top == bottom ? top : lerp(top, bottom, y, h - 1));
  }
}

// ramka (1 px, albo 2 px gdy podświetlona) + wnętrze z gradientem;
// panele z treścią odświeżaną kawałkami: top = C_PANEL (płaskie, żeby łaty tła nie odstawały)
static void card(int x0, int y0, int x1, int y1, COLOR border, bool thick = false, COLOR top = C_PANEL_HI)
{
  int t = thick ? 2 : 1;
  roundRect(x0, y0, x1, y1, border, border);
  roundRect(x0 + t, y0 + t, x1 - t, y1 - t, top, C_PANEL);
}

static void text(int x, int y, const char *s, sFONT *font, COLOR fg)
{
  GUI_DisString_EN(x, y, s, font, TRANSPARENT, fg);
}

static int textW(const char *s, sFONT *font)
{
  return strlen(s) * font->Width;
}

static void textCentered(int x0, int x1, int y, const char *s, sFONT *font, COLOR fg)
{
  text(x0 + (x1 - x0 - textW(s, font)) / 2, y, s, font, fg);
}

static void textRight(int x1, int y, const char *s, sFONT *font, COLOR fg)
{
  text(x1 - textW(s, font), y, s, font, fg);
}

// powiększony tekst z fontu Waveshare (każdy piksel = blok scale x scale)
static void textBig(int x, int y, const char *s, sFONT *font, int scale, COLOR fg)
{
  int bpr = (font->Width + 7) / 8;
  for (; *s; s++, x += font->Width * scale) {
    const uint8_t *p = &font->table[(*s - ' ') * font->Height * bpr];
    for (int row = 0; row < font->Height; row++)
      for (int col = 0; col < font->Width; col++)
        if (pgm_read_byte(p + row * bpr + col / 8) & (0x80 >> (col % 8)))
          fill(x + col * scale, y + row * scale, x + (col + 1) * scale, y + (row + 1) * scale, fg);
  }
}

static void button(int x0, int y0, int x1, int y1, const char *s, COLOR fg, COLOR border = C_BORDER)
{
  card(x0, y0, x1, y1, border);
  textCentered(x0, x1, y0 + (y1 - y0 - Font16.Height) / 2, s, &Font16, fg);
}

static bool inside(POINT x, POINT y, int x0, int y0, int x1, int y1)
{
  return x >= x0 && x < x1 && y >= y0 && y < y1;
}

static void effectName(int8_t fx, char *out)           { strcpy_P(out, kEffects[fx].name); }
static void effectCategory(int8_t fx, char *out)       { strcpy_P(out, kEffects[fx].category); }
static void paramName(int8_t fx, uint8_t p, char *out) { strcpy_P(out, kEffects[fx].params[p]); }
static bool isModulation(int8_t fx)                    { return pgm_read_byte(&kEffects[fx].modulation); }

static bool slotOn(int i) { return cur.enabled & (1 << i); }

// "P3" itd.
static void presetLabel(int i, char *out)
{
  out[0] = 'P';
  out[1] = '1' + i;
  out[2] = 0;
}

// x.y z wartości w dziesiątych
static void fmtTenths(char *out, long v10)
{
  ltoa(v10 / 10, out, 10);
  char *e = out + strlen(out);
  e[0] = '.';
  e[1] = '0' + v10 % 10;
  e[2] = 0;
}

// grafika efektu z icons.h: 2 bity/piksel + paleta, powiększona scale razy,
// wysyłana ciągiem w jedno okno (szybciej niż piksel po pikselu)
static void drawIcon(int8_t fx, int x0, int y0, int scale)
{
  COLOR pal[4];
  for (int c = 0; c < 4; c++) pal[c] = pgm_read_word(&kIconPalette[fx][c]);

  fill(x0 - 1, y0 - 1, x0 + ICON_W * scale + 1, y0 + ICON_H * scale + 1, C_BORDER);
  LCD_SetWindow(x0, y0, x0 + ICON_W * scale, y0 + ICON_H * scale);
  LCD_DC_1;
  LCD_CS_0;
  for (int row = 0; row < ICON_H; row++) {
    for (int rep = 0; rep < scale; rep++) {
      for (int col = 0; col < ICON_W; col++) {
        uint8_t b = pgm_read_byte(&kIcons[fx][(row * ICON_W + col) / 4]);
        COLOR c = pal[(b >> (6 - 2 * (col % 4))) & 0x03];
        for (int k = 0; k < scale; k++) {
          SPI4W_Write_Byte(c >> 8);
          SPI4W_Write_Byte(c & 0xFF);
        }
      }
    }
  }
  LCD_CS_1;
}

// dioda jak na pedale: ciemna obwódka, jasny środek gdy ON
static void drawLed(int x, int y, bool on, COLOR c = C_GREEN, COLOR dark = C_GREEN_DARK)
{
  roundRect(x, y, x + 12, y + 12, on ? c : dark, on ? c : dark);
  if (on) fill(x + 3, y + 3, x + 7, y + 7, c == C_GREEN ? C_GREEN_GLOW : C_TEXT);
}

// miernik segmentowy: zielone, 2 ostatnie szóstki żółte/czerwone
static void drawMeter(int x0, int x1, int y0, int y1, int nseg, int lit)
{
  for (int s = 0; s < nseg; s++) {
    int sx0 = x0 + (long)(x1 - x0) * s / nseg;
    int sx1 = x0 + (long)(x1 - x0) * (s + 1) / nseg - 1;
    COLOR c = C_TRACK;
    if (s < lit) c = s >= nseg * 11 / 12 ? C_RED : (s >= nseg * 9 / 12 ? C_ORANGE : C_GREEN);
    fill(sx0, y0, sx1, y1, c);
  }
}

// nagłówek podekranu: tytuł + [Back]
static void drawSubHeader(const char *title)
{
  LCD_Clear(C_BG);
  text(10, 10, title, &Font16, C_TEXT);
  button(BACK_X0, TOP_Y0, BACK_X1, TOP_Y1, "Back", C_TEXT);
  fill(6, 35, SCR_W - 6, 36, C_BORDER);
}

static bool backPressed(POINT x, POINT y)
{
  return inside(x, y, BACK_X0, TOP_Y0, BACK_X1, TOP_Y1);
}

// --- symulacja wejścia (na UNO nie ma audio) -------------------------------
// co ~2 s "szarpnięcie struny": poziom skacze i opada, czasem do przesteru

#define IN_LEVELS     12

static unsigned long pluckStart = 0;
static uint8_t pluckPeak = 0;
static uint8_t inLevel = 0;
static unsigned long clipUntil = 0;
static bool newPluck = false;

static void updateInput(unsigned long now)
{
  unsigned long t = now - pluckStart;
  if (t > 2200) {
    pluckStart = now;
    pluckPeak = random(0, 4) == 0 ? IN_LEVELS : random(7, 11);
    newPluck = true;
    t = 0;
  }
  int lvl = pluckPeak - (int)(t / 160);
  inLevel = lvl < 0 ? 0 : lvl;
  if (inLevel >= IN_LEVELS) clipUntil = now + 1200;
}

static bool clipping() { return millis() < clipUntil; }

// CPU: czas na slot w us (symulacja; na Daisy z DWT / CpuLoadMeter)
static uint16_t cpuTenths()   // obciążenie w dziesiątych %
{
  uint16_t sum = 30;          // reszta firmware
  for (int i = 0; i < NUM_SLOTS; i++)
    if (slotOn(i)) sum += 40 + cur.effect[i] * 6;
  return sum + random(0, 15);
}

// --- ekrany ---------------------------------------------------------------
enum Screen { SCR_MAIN, SCR_PICKER, SCR_PRESETS, SCR_TUNER, SCR_SETTINGS };
static Screen screen = SCR_MAIN;
static int8_t dragParam = -1;
static void setScreen(Screen s);

// ======================= ekran główny ======================================

static int tileX0(int i) { return 6 + i * (TILE_W + TILE_GAP); }

static void drawPresetCell()
{
  char buf[4];
  card(PRE_X0, TOP_Y0, PRE_X1, TOP_Y1, C_BORDER);
  presetLabel(curPreset, buf);
  text(PRE_X0 + 8, TOP_Y0 + 6, buf, &Font16, C_GREEN);
  text(PRE_X0 + 8 + 3 * Font16.Width, TOP_Y0 + 6, cur.name, &Font16, C_TEXT);
  if (modified)
    text(PRE_X0 + 8 + (4 + strlen(cur.name)) * Font16.Width, TOP_Y0 + 6, "*", &Font16, C_ORANGE);
  text(PRE_X1 - 14, TOP_Y0 + 7, ">", &Font12, C_DIM);
}

static void setModified()
{
  if (modified) return;
  modified = true;
  drawPresetCell();
}

static uint8_t shownLevel = 255;
static int8_t shownClip = -1;

static void drawInputMeter(bool force)
{
  bool clip = clipping();
  if (force) {
    card(IN_X0, TOP_Y0, IN_X1, TOP_Y1, C_BORDER);
    text(IN_X0 + 8, TOP_Y0 + 8, "IN", &Font12, C_DIM);
  }
  if (force || inLevel != shownLevel) {
    drawMeter(IN_X0 + 26, IN_X1 - 24, TOP_Y0 + 9, TOP_Y0 + 17, IN_LEVELS, inLevel);
    if (focused == FOCUS_MASTER)
      drawMeter(BAR_X0, BAR_X1, BAR_Y(1), BAR_Y(1) + BAR_H, 2 * IN_LEVELS, 2 * inLevel);
  }
  if (force || clip != shownClip)
    drawLed(IN_X1 - 18, TOP_Y0 + 7, clip, C_RED, C_RED_DARK);
  shownLevel = inLevel;
  shownClip = clip;
}

static void drawVolCell()
{
  char buf[4];
  bool f = focused == FOCUS_MASTER;
  card(VOL_X0, TOP_Y0, VOL_X1, TOP_Y1, f ? C_GREEN : C_BORDER, f);
  text(VOL_X0 + 8, TOP_Y0 + 8, "VOL", &Font12, C_DIM);
  itoa(master, buf, 10);
  textRight(VOL_X1 - 8, TOP_Y0 + 6, buf, &Font16, f ? C_GREEN : C_TEXT);
}

// ikonki rysowane prostokątami: kamerton i suwaki miksera
static void drawTopButtons()
{
  card(TUN_X0, TOP_Y0, TUN_X1, TOP_Y1, C_BORDER);
  int cx = (TUN_X0 + TUN_X1) / 2, y = TOP_Y0;
  fill(cx - 6, y + 5, cx - 3, y + 15, C_TEXT);
  fill(cx + 3, y + 5, cx + 6, y + 15, C_TEXT);
  fill(cx - 6, y + 15, cx + 6, y + 18, C_TEXT);
  fill(cx - 1, y + 18, cx + 2, y + 22, C_TEXT);

  card(SET_X0, TOP_Y0, SET_X1, TOP_Y1, C_BORDER);
  static const uint8_t knob[3] = { 14, 6, 11 };
  for (int i = 0; i < 3; i++) {
    int ly = TOP_Y0 + 7 + i * 6;
    fill(SET_X0 + 7, ly, SET_X1 - 7, ly + 1, C_MUTED);
    fill(SET_X0 + 5 + knob[i], ly - 2, SET_X0 + 9 + knob[i], ly + 3, C_TEXT);
  }
}

static void drawTile(int i)
{
  int x0 = tileX0(i), x1 = x0 + TILE_W;
  bool on = slotOn(i);
  char buf[14];

  card(x0, TILE_Y0, x1, TILE_Y1, i == focused ? C_GREEN : C_BORDER, i == focused);
  drawIcon(cur.effect[i], x0 + 10, TILE_Y0 + 9, 1);

  int tx = x0 + 60;
  effectName(cur.effect[i], buf);
  text(tx, TILE_Y0 + 9, buf, &Font12, C_TEXT);
  effectCategory(cur.effect[i], buf);
  text(tx, TILE_Y0 + 24, buf, &Font12, C_DIM);

  drawLed(tx, TILE_Y0 + 43, on);
  text(tx + 18, TILE_Y0 + 43, on ? "ON" : "OFF", &Font12, on ? C_GREEN : C_DIM);
}

// trójkąt pod kaflem w fokusie, wskazuje na panel
static void drawPointer(int i, COLOR c)
{
  if (i >= NUM_SLOTS) return;
  int cx = tileX0(i) + TILE_W / 2;
  for (int r = 0; r < 8; r++)
    fill(cx - 8 + r, TILE_Y1 + 1 + r, cx + 8 - r, TILE_Y1 + 2 + r, c);
}

static uint8_t *barValue(uint8_t p)
{
  return focused == FOCUS_MASTER ? &master : &cur.params[focused][p];
}

static void drawBar(uint8_t p)
{
  int y = BAR_Y(p);
  int v = *barValue(p);
  int knob = BAR_X0 + (long)(BAR_X1 - BAR_X0) * v / 100;

  // tło pod paskiem i uchwytem (uchwyt wystaje poza tor)
  fill(BAR_X0 - 8, y - 7, BAR_X1 + 9, y + BAR_H + 7, C_PANEL);
  roundRect(BAR_X0, y, BAR_X1, y + BAR_H, C_TRACK, C_TRACK);
  if (knob > BAR_X0 + 4) roundRect(BAR_X0, y, knob, y + BAR_H, C_GREEN_GLOW, C_GREEN);
  // podziałka co 25 %
  for (int t = 0; t <= 4; t++) {
    int tx = BAR_X0 + (BAR_X1 - BAR_X0) * t / 4;
    fill(tx, y + BAR_H + 3, tx + 1, y + BAR_H + 6, C_DIM);
  }
  roundRect(knob - 7, y - 6, knob + 8, y + BAR_H + 6, C_TEXT, C_KNOB);

  char buf[5];
  itoa(v, buf, 10);
  fill(BAR_X1 + 12, y - 4, VAL_X1, y + 14, C_PANEL);
  textRight(VAL_X1, y - 3, buf, &Font16, C_GREEN);

  // master: duża wartość po prawej
  if (focused == FOCUS_MASTER) {
    fill(ICON_X0 - 10, ICON_Y0 + 10, FX_X1 - 4, ICON_Y0 + 70, C_PANEL);
    int w = strlen(buf) * Font16.Width * 3;
    textBig(ICON_X0 + (ICON_W * ICON_SCALE - w) / 2, ICON_Y0 + 14, buf, &Font16, 3, C_TEXT);
  }
}

static void drawFocusedPanel()
{
  char buf[14];
  int8_t fx = cur.effect[focused < NUM_SLOTS ? focused : 0];
  bool m = focused == FOCUS_MASTER;

  card(FX_X0, FX_Y0, FX_X1, FX_Y1, C_BORDER, false, C_PANEL);

  if (m) strcpy(buf, "Master");
  else effectName(fx, buf);
  textBig(FX_X0 + 12, FX_Y0 + 8, buf, &Font16, 2, C_TEXT);   // tytuł: Font16 x2 (bez Font20, brak flasha na UNO)
  if (m) strcpy(buf, "output");
  else effectCategory(fx, buf);
  text(FX_X0 + 12, FX_Y0 + 42, buf, &Font12, C_DIM);
  fill(FX_X0 + 12, FX_Y0 + 58, ICON_X0 - 16, FX_Y0 + 59, C_BORDER);

  if (m) {
    text(FX_X0 + 12, BAR_Y(0) - 1, "volume", &Font12, C_MUTED);
    drawBar(0);
    text(FX_X0 + 12, BAR_Y(1) - 1, "input", &Font12, C_MUTED);
    shownLevel = 255;   // miernik dorysuje się przy następnym odświeżeniu
    text(ICON_X0 + 16, ICON_Y0 + 78, "master", &Font12, C_DIM);
  } else {
    for (uint8_t p = 0; p < 3; p++) {
      paramName(fx, p, buf);
      text(FX_X0 + 12, BAR_Y(p) - 1, buf, &Font12, C_MUTED);
      drawBar(p);
    }
    drawIcon(fx, ICON_X0, ICON_Y0, ICON_SCALE);
  }
}

static void drawButtons()
{
  char buf[8];
  bool m = focused == FOCUS_MASTER;
  button(BTN_L_X0, BTN_Y0, BTN_L_X1, BTN_Y1, "Change effect", m ? C_DIM : C_TEXT);

  card(BTN_R_X0, BTN_Y0, BTN_R_X1, BTN_Y1, C_BORDER);
  int ty = BTN_Y0 + 13;
  if (!m && isModulation(cur.effect[focused])) {
    // "TAP  120 BPM": TAP biały, tempo zielone
    itoa(bpm, buf, 10);
    strcat(buf, " BPM");
    int w = (5 + strlen(buf)) * Font16.Width;
    int x = BTN_R_X0 + (BTN_R_X1 - BTN_R_X0 - w) / 2;
    text(x, ty, "TAP", &Font16, C_TEXT);
    text(x + 5 * Font16.Width, ty, buf, &Font16, C_GREEN);
  } else {
    textCentered(BTN_R_X0, BTN_R_X1, ty, "TAP", &Font16, C_DIM);
  }
}

static void drawMain()
{
  LCD_Clear(C_BG);
  drawPresetCell();
  drawInputMeter(true);
  drawVolCell();
  drawTopButtons();
  for (int i = 0; i < NUM_SLOTS; i++) drawTile(i);
  drawPointer(focused, C_GREEN);
  drawFocusedPanel();
  drawButtons();
}

static void setFocus(int8_t f)
{
  int8_t old = focused;
  focused = f;
  drawPointer(old, C_BG);
  if (old < NUM_SLOTS) drawTile(old); else drawVolCell();
  if (f < NUM_SLOTS) drawTile(f); else drawVolCell();
  drawPointer(f, C_GREEN);
  drawFocusedPanel();
  drawButtons();
}

static void mainPress(POINT x, POINT y)
{
  for (int i = 0; i < NUM_SLOTS; i++) {
    if (!inside(x, y, tileX0(i), TILE_Y0, tileX0(i) + TILE_W, TILE_Y1)) continue;
    if (i == focused) {
      cur.enabled ^= 1 << i;
      drawTile(i);
      setModified();
    } else {
      setFocus(i);
    }
    return;
  }

  if (inside(x, y, PRE_X0, TOP_Y0, PRE_X1, TOP_Y1)) { setScreen(SCR_PRESETS); return; }
  if (inside(x, y, TUN_X0, TOP_Y0, TUN_X1, TOP_Y1)) { setScreen(SCR_TUNER); return; }
  if (inside(x, y, SET_X0, TOP_Y0, SET_X1, TOP_Y1)) { setScreen(SCR_SETTINGS); return; }
  if (inside(x, y, VOL_X0, TOP_Y0, VOL_X1, TOP_Y1)) { if (focused != FOCUS_MASTER) setFocus(FOCUS_MASTER); return; }

  bool m = focused == FOCUS_MASTER;
  if (!m && inside(x, y, BTN_L_X0, BTN_Y0, BTN_L_X1, BTN_Y1)) { setScreen(SCR_PICKER); return; }

  if (!m && inside(x, y, BTN_R_X0, BTN_Y0, BTN_R_X1, BTN_Y1) && isModulation(cur.effect[focused])) {
    unsigned long now = millis();
    unsigned long dt = now - lastTap;
    lastTap = now;
    if (dt > 200 && dt < 2000) {   // 30..300 BPM
      bpm = 60000UL / dt;
      drawButtons();
    }
    return;
  }

  for (uint8_t p = 0; p < (m ? 1 : 3); p++) {
    int by = BAR_Y(p);
    if (inside(x, y, BAR_X0 - 10, by - 12, BAR_X1 + 10, by + BAR_H + 12)) {
      dragParam = p;
      return;
    }
  }
}

static int barFromX(POINT x, int x0, int x1)
{
  long v = ((long)x - x0) * 100 / (x1 - x0);
  return v < 0 ? 0 : (v > 100 ? 100 : v);
}

static void mainDrag(POINT x)
{
  uint8_t *val = barValue(dragParam);
  int v = barFromX(x, BAR_X0, BAR_X1);
  if (v == *val) return;
  *val = v;
  drawBar(dragParam);
  if (focused == FOCUS_MASTER) drawVolCell();
  else setModified();
}

static void mainRelease()
{
  if (focused == FOCUS_MASTER) saveSettings();   // master jest globalny, nie w presecie
}

static unsigned long lastMeter = 0;

static void mainTick(unsigned long now)
{
  if (now - lastMeter < 60) return;
  lastMeter = now;
  drawInputMeter(false);
}

// ======================= wybór efektu =======================================

#define PICK_COLS 3
#define PICK_W    152
#define PICK_H    52
#define PICK_Y0   42

static void pickerCell(int idx, int *x0, int *y0)
{
  *x0 = 6 + (idx % PICK_COLS) * (PICK_W + TILE_GAP);
  *y0 = PICK_Y0 + (idx / PICK_COLS) * (PICK_H + 2);
}

static void drawPicker()
{
  char buf[14];
  drawSubHeader("Choose effect");
  strcpy(buf, "Slot ");
  buf[5] = '1' + focused;
  buf[6] = 0;
  text(10 + 14 * Font16.Width, 10, buf, &Font16, C_GREEN);

  for (int k = 0; k < NUM_EFFECTS; k++) {
    int x0, y0;
    pickerCell(k, &x0, &y0);
    bool current = k == cur.effect[focused];
    card(x0, y0, x0 + PICK_W, y0 + PICK_H, current ? C_GREEN : C_BORDER, current);
    drawIcon(k, x0 + 6, y0 + 2, 1);
    effectName(k, buf);
    text(x0 + 54, y0 + 12, buf, &Font12, current ? C_GREEN : C_TEXT);
    effectCategory(k, buf);
    text(x0 + 54, y0 + 27, buf, &Font12, C_DIM);
  }
}

static void pickerPress(POINT x, POINT y)
{
  if (backPressed(x, y)) { setScreen(SCR_MAIN); return; }
  for (int k = 0; k < NUM_EFFECTS; k++) {
    int x0, y0;
    pickerCell(k, &x0, &y0);
    if (!inside(x, y, x0, y0, x0 + PICK_W, y0 + PICK_H)) continue;
    if (k != cur.effect[focused]) {
      cur.effect[focused] = k;
      cur.enabled &= ~(1 << focused);
      for (int p = 0; p < 3; p++) cur.params[focused][p] = 50;
      modified = true;
    }
    setScreen(SCR_MAIN);
    return;
  }
}

// ======================= presety =============================================

#define PR_W      230
#define PR_H      52
#define PR_Y0     42

static int8_t prSelected = 0;

static void presetCell(int i, int *x0, int *y0)
{
  *x0 = 6 + (i % 2) * (PR_W + 8);
  *y0 = PR_Y0 + (i / 2) * (PR_H + 4);
}

static void drawPresetCard(int i)
{
  Preset p;
  char buf[14];
  int x0, y0;
  presetCell(i, &x0, &y0);
  readPreset(i, p);

  bool sel = i == prSelected;
  card(x0, y0, x0 + PR_W, y0 + PR_H, sel ? C_GREEN : C_BORDER, sel);
  presetLabel(i, buf);
  text(x0 + 10, y0 + 8, buf, &Font16, C_GREEN);
  text(x0 + 10 + 3 * Font16.Width, y0 + 8, p.name, &Font16, C_TEXT);
  if (i == curPreset) drawLed(x0 + PR_W - 22, y0 + 10, true);

  // skład łańcucha: po 7 znaków z nazwy efektu, zapalone jaśniej
  for (int s = 0; s < NUM_SLOTS; s++) {
    effectName(p.effect[s], buf);
    buf[7] = 0;
    text(x0 + 10 + s * 8 * Font12.Width, y0 + 31, buf, &Font12, (p.enabled & (1 << s)) ? C_MUTED : C_DIM);
  }
}

static void drawPresetButtons()
{
  char buf[12];
  strcpy(buf, "Load ");
  presetLabel(prSelected, buf + 5);
  button(BTN_L_X0, BTN_Y0, BTN_L_X1, BTN_Y1, buf, C_TEXT);
  strcpy(buf, "Save to ");
  presetLabel(prSelected, buf + 8);
  button(BTN_R_X0, BTN_Y0, BTN_R_X1, BTN_Y1, buf, C_GREEN);
}

static void drawPresets()
{
  drawSubHeader("Presets");
  if (modified) text(10 + 8 * Font16.Width, 10, "* unsaved changes", &Font12, C_ORANGE);
  for (int i = 0; i < NUM_PRESETS; i++) drawPresetCard(i);
  drawPresetButtons();
}

static void presetsPress(POINT x, POINT y)
{
  if (backPressed(x, y)) { setScreen(SCR_MAIN); return; }

  for (int i = 0; i < NUM_PRESETS; i++) {
    int x0, y0;
    presetCell(i, &x0, &y0);
    if (!inside(x, y, x0, y0, x0 + PR_W, y0 + PR_H) || i == prSelected) continue;
    int8_t old = prSelected;
    prSelected = i;
    drawPresetCard(old);
    drawPresetCard(i);
    drawPresetButtons();
    return;
  }

  if (inside(x, y, BTN_L_X0, BTN_Y0, BTN_L_X1, BTN_Y1)) {
    loadPreset(prSelected);
    setScreen(SCR_MAIN);
  } else if (inside(x, y, BTN_R_X0, BTN_Y0, BTN_R_X1, BTN_Y1)) {
    Preset p;
    readPreset(prSelected, p);
    memcpy(cur.name, p.name, sizeof(cur.name));   // nazwa zostaje (brak klawiatury)
    writePreset(prSelected, cur);
    curPreset = prSelected;
    modified = false;
    saveSettings();
    drawPresets();
  }
}

// ======================= tuner ==============================================

static const char kStringNames[6] = { 'E', 'A', 'D', 'G', 'B', 'e' };
static const uint16_t kStringFreq10[6] = { 824, 1100, 1468, 1960, 2469, 3296 };   // Hz * 10

#define TN_STR_Y0   44
#define TN_STR_Y1   80
#define TN_MET_X0   30
#define TN_MET_X1   450
#define TN_MET_Y0   196
#define TN_MET_Y1   230

static int8_t tunerString = 0;
static int tunerCents = 0;
static bool tunerValid = false;
static int8_t shownString = -1;
static int needleX = -1;
static int8_t shownState = -2;   // -1 brak, 0 w stroju, 1 za wysoko, 2 za nisko

static int stringX0(int s) { return 30 + s * 72; }

static void drawStringBox(int s, bool active)
{
  char buf[2] = { kStringNames[s], 0 };
  int x0 = stringX0(s);
  card(x0, TN_STR_Y0, x0 + 60, TN_STR_Y1, active ? C_GREEN : C_BORDER, active);
  textCentered(x0, x0 + 60, TN_STR_Y0 + 10, buf, &Font16, active ? C_GREEN : C_MUTED);
}

static void drawTuner()
{
  drawSubHeader("Tuner");
  for (int s = 0; s < 6; s++) drawStringBox(s, false);

  // skala -50..+50 centów, kreska co 10
  fill(TN_MET_X0, TN_MET_Y1 + 2, TN_MET_X1, TN_MET_Y1 + 3, C_BORDER);
  for (int c = -50; c <= 50; c += 10) {
    int tx = TN_MET_X0 + (long)(TN_MET_X1 - TN_MET_X0) * (c + 50) / 100;
    fill(tx, TN_MET_Y1 + 3, tx + 1, TN_MET_Y1 + (c == 0 ? 14 : 8), c == 0 ? C_TEXT : C_DIM);
  }
  text(TN_MET_X0 - 10, TN_MET_Y1 + 18, "-50", &Font12, C_DIM);
  text((TN_MET_X0 + TN_MET_X1) / 2 - 3, TN_MET_Y1 + 18, "0", &Font12, C_DIM);
  text(TN_MET_X1 - 14, TN_MET_Y1 + 18, "+50", &Font12, C_DIM);

  shownString = -1;
  needleX = -1;
  shownState = -2;
}

static void updateTuner()
{
  bool valid = inLevel > 2;
  if (valid && newPluck) {               // nowe szarpnięcie
    tunerString = random(0, 6);
    tunerCents = random(-40, 41);
  } else if (valid) {
    tunerCents = tunerCents * 70 / 100 + random(-3, 4);   // "dokręcanie" do zera
  }
  newPluck = false;
  tunerValid = valid;
}

static void drawTunerReading()
{
  int8_t s = tunerValid ? tunerString : -1;
  int c = tunerCents < -50 ? -50 : (tunerCents > 50 ? 50 : tunerCents);
  int8_t state = !tunerValid ? -1 : (abs(c) < 5 ? 0 : (c > 0 ? 1 : 2));
  COLOR col = state == 0 ? C_GREEN : C_ORANGE;

  // struna + duża litera
  if (s != shownString) {
    if (shownString >= 0) drawStringBox(shownString, false);
    if (s >= 0) drawStringBox(s, true);
    fill(150, 90, 330, 186, C_BG);
    if (s >= 0) {
      char buf[2] = { kStringNames[s], 0 };
      textBig(212, 98, buf, &Font16, 5, C_TEXT);
    }
    shownString = s;
  }

  // częstotliwość i centy
  fill(300, 110, 474, 170, C_BG);
  if (tunerValid) {
    char buf[12];
    long f10 = kStringFreq10[tunerString] + (long)kStringFreq10[tunerString] * c * 578 / 1000000L;
    fmtTenths(buf, f10);
    strcat(buf, " Hz");
    text(300, 116, buf, &Font16, C_MUTED);
    itoa(c, buf, 10);
    strcat(buf, " ct");
    text(300, 142, buf, &Font16, col);
  } else {
    text(140, 130, "No signal... play a note", &Font16, C_DIM);
  }

  // wskazówka
  int nx = TN_MET_X0 + (long)(TN_MET_X1 - TN_MET_X0) * (c + 50) / 100 - 3;
  if (needleX >= 0) fill(needleX, TN_MET_Y0, needleX + 7, TN_MET_Y1, C_BG);
  int cx = (TN_MET_X0 + TN_MET_X1) / 2;
  fill(cx, TN_MET_Y0 + 6, cx + 1, TN_MET_Y1, C_BORDER);   // środek skali
  if (tunerValid) {
    roundRect(nx, TN_MET_Y0, nx + 7, TN_MET_Y1, col, col);
    needleX = nx;
  } else {
    needleX = -1;
  }

  // komunikat
  if (state != shownState) {
    fill(6, 268, SCR_W - 6, 300, C_BG);
    const char *msg = state == 0 ? "*** IN TUNE! ***"
                    : state == 1 ? "too high - loosen the string"
                    : state == 2 ? "too low - tighten the string" : "";
    textCentered(0, SCR_W, 276, msg, &Font16, col);
    shownState = state;
  }
}

static unsigned long lastTuner = 0;

static void tunerTick(unsigned long now)
{
  if (now - lastTuner < 120) return;
  lastTuner = now;
  updateTuner();
  drawTunerReading();
}

static void tunerPress(POINT x, POINT y)
{
  if (backPressed(x, y)) setScreen(SCR_MAIN);
}

// ======================= ustawienia =========================================

#define ST_X0       6
#define ST_X1       474
#define ST_ROW_H    50
#define ST_Y(r)     (42 + (r) * (ST_ROW_H + 6))
#define ST_CTL_X0   170
#define ST_BAR_X1   392
#define FS_X0       170
#define FS_MID      320
#define FS_X1       468

static bool resetArmed = false;
static unsigned long resetArmedAt = 0;

static void drawRowCard(int r, const char *label)
{
  card(ST_X0, ST_Y(r), ST_X1, ST_Y(r) + ST_ROW_H, C_BORDER, false, C_PANEL);
  text(ST_X0 + 12, ST_Y(r) + 17, label, &Font16, C_TEXT);
}

static void drawBrightness()
{
  int y = ST_Y(0) + 20;
  int knob = ST_CTL_X0 + (long)(ST_BAR_X1 - ST_CTL_X0) * brightness / 100;
  fill(ST_CTL_X0 - 8, y - 7, ST_BAR_X1 + 9, y + BAR_H + 7, C_PANEL);
  roundRect(ST_CTL_X0, y, ST_BAR_X1, y + BAR_H, C_TRACK, C_TRACK);
  if (knob > ST_CTL_X0 + 4) roundRect(ST_CTL_X0, y, knob, y + BAR_H, C_GREEN_GLOW, C_GREEN);
  roundRect(knob - 7, y - 6, knob + 8, y + BAR_H + 6, C_TEXT, C_KNOB);

  char buf[6];
  itoa(brightness, buf, 10);
  strcat(buf, "%");
  fill(ST_BAR_X1 + 12, y - 4, ST_X1 - 8, y + 14, C_PANEL);
  textRight(ST_X1 - 12, y - 3, buf, &Font16, C_GREEN);
}

static void drawFsMode()
{
  int y0 = ST_Y(1) + 6, y1 = y0 + 22;
  card(FS_X0, y0, FS_MID - 2, y1, fsMode == 0 ? C_GREEN : C_BORDER, fsMode == 0);
  card(FS_MID + 2, y0, FS_X1, y1, fsMode == 1 ? C_GREEN : C_BORDER, fsMode == 1);
  textCentered(FS_X0, FS_MID - 2, y0 + 5, "Effects", &Font12, fsMode == 0 ? C_GREEN : C_MUTED);
  textCentered(FS_MID + 2, FS_X1, y0 + 5, "Presets", &Font12, fsMode == 1 ? C_GREEN : C_MUTED);
  fill(FS_X0, y1 + 2, FS_X1, y1 + 16, C_PANEL);
  text(FS_X0, y1 + 4, fsMode == 0 ? "FS1-FS3: effect on/off"
                                  : "FS1/FS2: preset -/+, FS3: tap", &Font12, C_DIM);
}

static void drawCpu()
{
  char buf[10];
  int y = ST_Y(2) + 17;
  fmtTenths(buf, cpuTenths());
  strcat(buf, " %");
  fill(ST_CTL_X0, y, ST_CTL_X0 + 100, y + Font16.Height, C_PANEL);
  text(ST_CTL_X0, y, buf, &Font16, C_GREEN);
}

static void drawResetButton()
{
  button(BTN_L_X0, BTN_Y0, BTN_L_X1, BTN_Y1, resetArmed ? "Tap again to reset" : "Reset settings",
         C_RED, resetArmed ? C_RED : C_BORDER);
}

static void drawSettings()
{
  drawSubHeader("Settings");

  drawRowCard(0, "Brightness");
  drawBrightness();

  drawRowCard(1, "Footswitch");
  drawFsMode();

  drawRowCard(2, "CPU load");
  drawCpu();
  text(ST_CTL_X0 + 110, ST_Y(2) + 19, "(simulated on UNO)", &Font12, C_DIM);

  drawRowCard(3, "Info");
  text(ST_CTL_X0, ST_Y(3) + 10, "Multieffect " FW_VERSION "  " __DATE__, &Font12, C_MUTED);
  text(ST_CTL_X0, ST_Y(3) + 27, "Daisy Seed, 48 kHz, block 48", &Font12, C_DIM);

  resetArmed = false;
  drawResetButton();
  text(BTN_R_X0 + 8, BTN_Y0 + 8, "Presets back to factory,", &Font12, C_DIM);
  text(BTN_R_X0 + 8, BTN_Y0 + 22, "brightness/footswitch/VOL", &Font12, C_DIM);
}

static void settingsPress(POINT x, POINT y)
{
  if (backPressed(x, y)) { setScreen(SCR_MAIN); return; }

  int by = ST_Y(0) + 20;
  if (inside(x, y, ST_CTL_X0 - 10, by - 14, ST_BAR_X1 + 10, by + BAR_H + 14)) { dragParam = 0; return; }

  if (inside(x, y, FS_X0, ST_Y(1), FS_X1, ST_Y(1) + 30)) {
    uint8_t m = x < FS_MID ? 0 : 1;
    if (m != fsMode) { fsMode = m; saveSettings(); drawFsMode(); }
    return;
  }

  if (inside(x, y, BTN_L_X0, BTN_Y0, BTN_L_X1, BTN_Y1)) {
    if (!resetArmed) {
      resetArmed = true;
      resetArmedAt = millis();
      drawResetButton();
    } else {
      factoryReset();
      applyBrightness();
      focused = 0;
      setScreen(SCR_MAIN);
    }
  }
}

static void settingsDrag(POINT x)
{
  int v = barFromX(x, ST_CTL_X0, ST_BAR_X1);
  if (v == brightness) return;
  brightness = v;
  applyBrightness();
  drawBrightness();
}

static unsigned long lastCpu = 0;

static void settingsTick(unsigned long now)
{
  if (resetArmed && now - resetArmedAt > 3000) {   // potwierdzenie wygasa
    resetArmed = false;
    drawResetButton();
  }
  if (now - lastCpu < 500) return;
  lastCpu = now;
  drawCpu();
}

// ======================= przełączanie ekranów / pętla ========================

static void setScreen(Screen s)
{
  screen = s;
  dragParam = -1;
  switch (s) {
  case SCR_MAIN:     drawMain(); break;
  case SCR_PICKER:   drawPicker(); break;
  case SCR_PRESETS:  prSelected = curPreset; drawPresets(); break;
  case SCR_TUNER:    drawTuner(); break;
  case SCR_SETTINGS: drawSettings(); break;
  }
}

static void onPress(POINT x, POINT y)
{
  switch (screen) {
  case SCR_MAIN:     mainPress(x, y); break;
  case SCR_PICKER:   pickerPress(x, y); break;
  case SCR_PRESETS:  presetsPress(x, y); break;
  case SCR_TUNER:    tunerPress(x, y); break;
  case SCR_SETTINGS: settingsPress(x, y); break;
  }
}

void setup()
{
  System_Init();

  LCD_SCAN_DIR dir = SCAN_DIR_DFT;   // D2U_L2R, poziomo 480x320
  LCD_Init(dir, 200);
  TP_Init(dir);
  TP_GetAdFac();                     // domyślna kalibracja Waveshare

  if (EEPROM.read(EE_ADDR_MAGIC) != EE_MAGIC) {
    factoryReset();                  // pierwszy start: presety fabryczne
  } else {
    brightness = EEPROM.read(EE_ADDR_BRIGHT);
    fsMode = EEPROM.read(EE_ADDR_FS);
    master = EEPROM.read(EE_ADDR_MASTER);
    loadPreset(EEPROM.read(EE_ADDR_CUR) % NUM_PRESETS);
  }
  applyBrightness();

  setScreen(SCR_MAIN);
}

void loop()
{
  static bool wasPressed = false;
  POINT x, y;
  bool pressed = TP_GetPoint(&x, &y);

  if (pressed && !wasPressed) onPress(x, y);
  if (pressed && dragParam >= 0) {
    if (screen == SCR_MAIN) mainDrag(x);
    else if (screen == SCR_SETTINGS) settingsDrag(x);
  }
  if (!pressed && wasPressed && dragParam >= 0) {
    if (screen == SCR_MAIN) mainRelease();
    else if (screen == SCR_SETTINGS) saveSettings();
    dragParam = -1;
  }
  wasPressed = pressed;

  unsigned long now = millis();
  updateInput(now);
  if (screen == SCR_MAIN) mainTick(now);
  else if (screen == SCR_TUNER) tunerTick(now);
  else if (screen == SCR_SETTINGS) settingsTick(now);
}
