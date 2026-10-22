// Symulator UI na PC: ten sam szkic + prawdziwe LCD_Driver.cpp / LCD_GUI.cpp / fonty,
// tylko SPI trafia do emulacji ILI9486 (okno 0x2A/0x2B + zapis 0x2C) zamiast na ekran.
// Dotyk = skrypt (scenario.h), zrzuty ekranu jako PPM.
#include "../../Multieffect_UI.ino"
#include <vector>
#include <string>
#include <functional>

// --- emulacja ILI9486 (protokół shieldu: komenda 1 bajt, dane po 2 bajty) ---
static uint16_t fb[SCR_H][SCR_W];
static int pinDC = 0, pinCS = 1;
static uint8_t cmd = 0, hi = 0;
static int phase = 0, argIdx = 0;
static uint8_t args[4];
static int wx0, wx1, wy0, wy1, px, py;

static void onWord(uint16_t w)
{
  if (cmd == 0x2A || cmd == 0x2B) {
    if (argIdx < 4) args[argIdx++] = w & 0xFF;
    if (argIdx == 4) {
      int a = args[0] << 8 | args[1], b = args[2] << 8 | args[3];
      if (b < a) b = a;   // LCD_SetCursor ustawia koniec = start - 1
      if (cmd == 0x2A) { wx0 = a; wx1 = b; } else { wy0 = a; wy1 = b; }
    }
  } else if (cmd == 0x2C) {
    if (px < SCR_W && py < SCR_H) fb[py][px] = w;
    if (++px > wx1) { px = wx0; if (++py > wy1) py = wy0; }
  }
}

SPIClass SPI;
uint8_t SPIClass::transfer(uint8_t b)
{
  if (pinCS) return 0;
  if (!pinDC) {
    cmd = b; argIdx = 0; phase = 0;
    if (cmd == 0x2C) { px = wx0; py = wy0; }
  } else if (phase == 0) {
    hi = b; phase = 1;
  } else {
    onWord(hi << 8 | b); phase = 0;
  }
  return 0;
}

void digitalWrite(int pin, int v)
{
  if (pin == LCD_DC) pinDC = v;
  if (pin == LCD_CS) { pinCS = v; if (v) phase = 0; }
}
int digitalRead(int) { return HIGH; }

static unsigned long simTime = 0;
unsigned long millis() { return simTime++; }
void delay(unsigned long ms) { simTime += ms; }
SerialStub Serial;

// --- dotyk ze skryptu ---------------------------------------------------
enum ActType { TAP, PRESS, RELEASE, WAIT, SNAP, UNTIL };
struct Act { ActType t; int x, y; std::string name; std::function<bool()> cond; };
static std::vector<Act> acts;
static size_t ai = 0;
static int tapPhase = 0;
static const char *outDir = ".";

static void snap(const std::string &name)
{
  std::string path = std::string(outDir) + "/" + name + ".ppm";
  FILE *f = fopen(path.c_str(), "wb");
  fprintf(f, "P6 %d %d 255\n", SCR_W, SCR_H);
  for (int y = 0; y < SCR_H; y++)
    for (int x = 0; x < SCR_W; x++) {
      uint16_t c = fb[y][x];
      uint8_t rgb[3] = { (uint8_t)((c >> 11) << 3), (uint8_t)(((c >> 5) & 0x3F) << 2), (uint8_t)((c & 0x1F) << 3) };
      fwrite(rgb, 1, 3, f);
    }
  fclose(f);
  printf("%s\n", path.c_str());
}

void TP_Init(LCD_SCAN_DIR) {}
void TP_GetAdFac(void) {}

bool TP_GetPoint(POINT *x, POINT *y)
{
  static int heldX = -1, heldY = -1;
  while (true) {
    if (ai >= acts.size()) exit(0);
    Act &a = acts[ai];
    switch (a.t) {
    case SNAP: snap(a.name); ai++; continue;
    case WAIT: simTime += a.x; ai++; continue;
    case UNTIL:   // brak dotyku, aż warunek spełniony (stan zmienia się w loop())
      if (a.cond()) { ai++; continue; }
      return false;
    case PRESS: heldX = a.x; heldY = a.y; ai++; break;
    case RELEASE: heldX = -1; ai++; return false;
    case TAP:
      if (tapPhase == 0) { tapPhase = 1; *x = a.x; *y = a.y; return true; }
      tapPhase = 0; ai++; return false;
    }
    if (heldX >= 0) { *x = heldX; *y = heldY; return true; }
  }
}

static void tap(int x, int y)  { acts.push_back({ TAP, x, y, "" }); }
static void press(int x, int y) { acts.push_back({ PRESS, x, y, "" }); }
static void release()          { acts.push_back({ RELEASE, 0, 0, "" }); }
static void wait(int ms)       { acts.push_back({ WAIT, ms, 0, "" }); }
static void shot(const char *n) { acts.push_back({ SNAP, 0, 0, n }); }
static void until(std::function<bool()> c) { acts.push_back({ UNTIL, 0, 0, "", c }); }

#include "scenario.h"

int main(int argc, char **argv)
{
  if (argc > 1) outDir = argv[1];
  scenario();
  setup();
  while (true) loop();
}
