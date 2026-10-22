// Ekrany: główny (pasek, 3 kafle efektów, panel z 3 suwakami = 3 enkodery, soft-keys), menu,
// wybór efektu, presety, tuner, ustawienia. Styl jak w wersji Qt (ciemny motyw,
// zielony akcent), grafiki pedałów z Qt. Rysuje do framebuffera tylko to, co się zmieniło.
//
// Sterowanie enkoderami (dotyk tylko w symulatorze, na Daisy wyłączony):
//   główny:  E1..E3 kręcą parametry 1..3 (master: E1 = głośność),
//            E1 wciśnięty = następny efekt, E2 = ON/OFF, E3 = menu (modulacja: tap, przytrzymanie = menu)
//   reszta:  E1 kręcenie = wybór, E1 wciśnięcie = OK, E2 kręcenie = wartość, E3 wciśnięcie = wstecz
#include "ui.h"
#include "ui_layout.h"
#include "gfx.h"
#include "platform.h"
#include "assets/fonts.h"
#include "assets/icons.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

using namespace gfx;

#define FW_VERSION "v0.2"

// --- kolory (Main.qml, darkMode) ----------------------------------------
static constexpr Color C_BG_TOP     = rgb(0x22, 0x22, 0x22);
static constexpr Color C_BG_BOT     = rgb(0x17, 0x17, 0x17);
static constexpr Color C_CARD_TOP   = rgb(0x33, 0x33, 0x33);
static constexpr Color C_CARD_BOT   = rgb(0x27, 0x27, 0x27);
static constexpr Color C_PANEL_TOP  = rgb(0x2d, 0x2d, 0x2d);   // panelColor #2a2a2a
static constexpr Color C_PANEL_BOT  = rgb(0x26, 0x26, 0x26);
static constexpr Color C_BORDER     = rgb(0x44, 0x44, 0x44);   // borderColor
static constexpr Color C_TRACK      = rgb(0x15, 0x15, 0x15);   // trackColor
static constexpr Color C_TEXT       = rgb(0xff, 0xff, 0xff);
static constexpr Color C_MUTED      = rgb(0xaa, 0xaa, 0xaa);   // mutedTextColor
static constexpr Color C_DIM        = rgb(0x70, 0x70, 0x70);   // dimTextColor
static constexpr Color C_GREEN      = rgb(0x4c, 0xaf, 0x50);   // highlight
static constexpr Color C_GREEN_HI   = rgb(0x7d, 0xd8, 0x81);
static constexpr Color C_GREEN_DARK = rgb(0x1f, 0x3a, 0x21);
static constexpr Color C_ORANGE     = rgb(0xe0, 0xa0, 0x30);   // Qt: CPU > 50 %, tuner nie w stroju
static constexpr Color C_RED        = rgb(0xe5, 0x48, 0x4d);
static constexpr Color C_RED_DARK   = rgb(0x45, 0x1e, 0x20);
static constexpr Color C_BLACK      = 0;

// --- efekty: nazwy parametrów jak w wersji PC ---------------------------
struct EffectInfo {
	const char* name;
	const char* params[3];
	const char* category;
	bool        modulation;   // tap tempo
};

static const EffectInfo kEffects[] = {
	{ "Fuzz",         { "Gain", "Volume", "Clip" },                     "distortion", false },
	{ "Tremolo",      { "frequency", "depth", "None" },                 "modulation", true },
	{ "NoiseGate",    { "threshold", "attack_coeff", "release_coeff" }, "dynamics",   false },
	{ "Rat",          { "drive", "volume", "tone" },                    "distortion", false },
	{ "TubeScreamer", { "drive", "volume", "tone" },                    "distortion", false },
	{ "Delay",        { "Time", "Feedback", "Mix" },                    "time",       false },
	{ "Compressor",   { "treshold", "ratio", "makeupGain" },            "dynamics",   false },
	{ "Vibrato",      { "rate", "depth", "mix" },                       "modulation", true },
	{ "Flanger",      { "rate", "depth", "feedback" },                  "modulation", true },
	{ "Phaser",       { "rate", "depth", "feedback" },                  "modulation", true },
	{ "AutoWah",      { "Sensitivity", "Resonance", "Mix" },            "filter",     false },
	{ "Equalizer",    { "Low", "Mid", "High" },                         "filter",     false },
	{ "Boost",        { "gain", "tone", "volume" },                     "gain",       false },
};
static constexpr int kNumEffects = sizeof(kEffects) / sizeof(kEffects[0]);
static_assert(kNumEffects == ICON_COUNT, "grafiki i efekty muszą się zgadzać");

static const Preset kFactoryPresets[UI_NUM_PRESETS] = {
	{ "Clean",   { 6, 11, 5 }, 0b011, { { 40, 30, 55 }, { 50, 45, 60 }, { 35, 30, 25 } } },
	{ "Crunch",  { 4, 12, 5 }, 0b001, { { 55, 60, 50 }, { 40, 50, 60 }, { 30, 25, 20 } } },
	{ "Lead",    { 3, 12, 5 }, 0b111, { { 70, 55, 60 }, { 60, 55, 65 }, { 45, 40, 30 } } },
	{ "Ambient", { 6, 7, 5 },  0b110, { { 35, 30, 50 }, { 25, 40, 50 }, { 70, 65, 55 } } },
	{ "Funk",    { 6, 10, 9 }, 0b011, { { 50, 45, 55 }, { 65, 60, 70 }, { 40, 50, 30 } } },
	{ "Fuzz",    { 2, 0, 1 },  0b011, { { 30, 40, 50 }, { 80, 55, 70 }, { 45, 60, 50 } } },
	{ "Surf",    { 1, 11, 5 }, 0b111, { { 60, 70, 50 }, { 45, 55, 70 }, { 30, 35, 40 } } },
	{ "Default", { 4, 1, 5 },  0b000, { { 50, 50, 50 }, { 50, 50, 50 }, { 50, 50, 50 } } },
};

static constexpr uint32_t kMagic = 0x4D465831;   // "MFX1"

// --- stan ----------------------------------------------------------------
#define FOCUS_MASTER UI_NUM_SLOTS   // fokus na master volume zamiast efektu

static Stored  st;               // to, co leży w pamięci nieulotnej
static Preset  cur;              // aktualny łańcuch (może różnić się od zapisanego presetu)
static bool    modified = false;
static int     focused = 0;
static int     bpm = 120;
static uint32_t lastTap = 0;

enum Screen { SCR_MAIN, SCR_MENU, SCR_PICKER, SCR_PRESETS, SCR_TUNER, SCR_SETTINGS };
static Screen screen = SCR_MAIN;
static int  dragParam = -1;
static bool wasPressed = false;

static void setScreen(Screen s);

static bool slotOn(int i) { return cur.enabled & (1 << i); }
static bool focusedIsModulation() { return focused != FOCUS_MASTER && kEffects[cur.effect[focused]].modulation; }

static void save()
{
	plat_save(&st, sizeof(st));
}

static void applyOutputs()
{
	plat_backlight(st.brightness);
	plat_set_master(st.master / 100.0f);
}

static void loadPreset(int i)
{
	cur = st.presets[i];
	st.curPreset = i;
	modified = false;
	save();
}

static void factoryReset()
{
	st.magic = kMagic;
	st.brightness = 80;
	st.fsMode = 0;
	st.master = 80;
	st.curPreset = 0;
	memcpy(st.presets, kFactoryPresets, sizeof(st.presets));
	loadPreset(0);
	applyOutputs();
}

static int clamp100(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }
static int wrap(int v, int n) { return ((v % n) + n) % n; }

// --- elementy wspólne --------------------------------------------------

static const Rect kScreen = { 0, 0, W, H };

static void bg(int x0, int y0, int x1, int y1)
{
	gradientPart(kScreen, { x0, y0, x1, y1 }, C_BG_TOP, C_BG_BOT);
}

// karta: cień + ramka + gradient; focused = zielona ramka 2 px z poświatą (fokus / kursor)
static void card(const Rect& r, bool focused = false, bool panel = false, Color border = C_BORDER)
{
	float rad = (r.y1 - r.y0) > 60 ? 9.0f : 7.0f;
	shadow(r.x0, r.y0, r.x1, r.y1, rad, 4, 110);
	if (focused) {
		roundRect(r.x0 - 3, r.y0 - 3, r.x1 + 3, r.y1 + 3, rad + 3, C_GREEN, C_GREEN, 45);
		border = C_GREEN;
	}
	int t = focused ? 2 : 1;
	roundRect(r.x0, r.y0, r.x1, r.y1, rad, border, border);
	roundRect(r.x0 + t, r.y0 + t, r.x1 - t, r.y1 - t, rad - t,
	          panel ? C_PANEL_TOP : C_CARD_TOP, panel ? C_PANEL_BOT : C_CARD_BOT);
	fillAlpha(r.x0 + (int)rad, r.y0 + t, r.x1 - (int)rad, r.y0 + t + 1, C_TEXT, 14);   // odblask u góry
}

// odmaluj kawałek wnętrza karty (pod elementem, który się zmienia)
static void cardPart(const Rect& card, const Rect& part, bool panel = false)
{
	gradientPart(card, part, panel ? C_PANEL_TOP : C_CARD_TOP, panel ? C_PANEL_BOT : C_CARD_BOT);
}

// tło pod kartą razem z cieniem i poświatą (przy przerysowaniu całej karty)
static void bgUnder(const Rect& r)
{
	bg(r.x0 - 5, r.y0 - 5, r.x1 + 5, r.y1 + 7);
}

static void button(const Rect& r, const char* s, Color fg, Color border = C_BORDER)
{
	card(r, false, false, border);
	textCentered(r.x0, r.x1, r.y0 + (r.y1 - r.y0 - kFont_label.height) / 2, s, kFont_label, fg);
}

static void led(float cx, float cy, bool on, Color c = C_GREEN, Color dark = C_GREEN_DARK)
{
	if (on) {
		glow(cx, cy, 13, c, 110);
		circle(cx, cy, 5.5f, c);
		circle(cx - 1.5f, cy - 1.5f, 2.0f, C_TEXT, 150);
	} else {
		circle(cx, cy, 5.5f, dark);
		circle(cx, cy, 4.5f, blend(dark, C_BLACK, 80));
	}
}

static void icon(int fx, int x, int y, int div, bool dimmed = false)
{
	int w = ICON_W / div, h = ICON_H / div;
	roundRect(x - 1, y - 1, x + w + 1, y + h + 1, 2, C_BORDER, C_BORDER);
	image(x, y, kIcons[fx], ICON_W, ICON_H, div, dimmed ? 130 : 0);
}

// miernik segmentowy: zielony, potem żółty i czerwony
static Color meterColor(int s, int nseg, int lit)
{
	if (s >= lit) return C_TRACK;
	return s >= nseg * 11 / 12 ? C_RED : (s >= nseg * 9 / 12 ? C_ORANGE : C_GREEN);
}

static void meter(int x0, int x1, int y0, int y1, int nseg, int lit)
{
	for (int s = 0; s < nseg; s++) {
		int sx0 = x0 + (x1 - x0) * s / nseg;
		int sx1 = x0 + (x1 - x0) * (s + 1) / nseg - 2;
		Color c = meterColor(s, nseg, lit);
		roundRect(sx0, y0, sx1, y1, 1.5f, c, c);
	}
}

// suwak poziomy
static void slider(int x0, int x1, int cy, int value)
{
	int knob = x0 + (x1 - x0) * value / 100;
	roundRect(x0, cy - 4, x1, cy + 4, 4, C_TRACK, C_TRACK);
	fillAlpha(x0 + 4, cy - 4, x1 - 4, cy - 3, C_BLACK, 120);   // wgłębienie
	if (knob > x0 + 6) roundRect(x0, cy - 4, knob, cy + 4, 4, C_GREEN_HI, C_GREEN);
	circle(knob, cy + 1.5f, 10, C_BLACK, 90);
	circle(knob, cy, 9, rgb(0xc8, 0xc8, 0xc8));
	circle(knob, cy - 0.5f, 7.5f, rgb(0xf4, 0xf4, 0xf4));
}

// "12.3" + suffix, bez printf("%f") (newlib-nano na Daisy go nie ma)
static void fmt1(char* out, int size, float v, const char* suffix)
{
	int t = (int)lroundf(v * 10.0f);
	snprintf(out, size, "%s%d.%d%s", t < 0 ? "-" : "", abs(t) / 10, abs(t) % 10, suffix);
}

// nazwa w ograniczonym miejscu: font f, a jak się nie mieści — mniejszy (wyśrodkowany w pionie)
static void fitText(int x, int y, int maxW, const char* s, const Font& f, Color c)
{
	if (textWidth(s, f) <= maxW) text(x, y, s, f, c);
	else text(x, y + (f.height - kFont_small.height) / 2, s, kFont_small, c);
}

static bool inside(int x, int y, int x0, int y0, int x1, int y1)
{
	return x >= x0 && x < x1 && y >= y0 && y < y1;
}

static int valueFromX(int x, int x0, int x1)
{
	return clamp100((x - x0) * 100 / (x1 - x0));
}

// --- soft-keys: 3 pola nad enkoderami, co robi ich wciśnięcie -------------------

struct SoftKey {
	const char* label;   // nullptr = nic
	const char* sub;     // druga linia (mniejsza), może być nullptr
	Color       color;
};

static void softKeys(const SoftKey& a, const SoftKey& b, const SoftKey& c)
{
	const SoftKey* keys[3] = { &a, &b, &c };
	bg(0, SK_Y0 - 5, W, H);
	for (int i = 0; i < 3; i++) {
		Rect r = { SK_X0(i), SK_Y0, SK_X0(i) + SK_W, SK_Y1 };
		const SoftKey& k = *keys[i];
		card(r);
		// numer enkodera w kółku
		float cx = r.x0 + 18, cy = (SK_Y0 + SK_Y1) / 2.0f;
		circle(cx, cy, 10, k.label ? C_GREEN_DARK : C_TRACK);
		char n[2] = { (char)('1' + i), 0 };
		textCentered((int)cx - 10, (int)cx + 10, (int)cy - kFont_small.height / 2 - 1, n, kFont_small,
		             k.label ? C_GREEN_HI : C_DIM);
		if (!k.label) continue;
		int tx = r.x0 + 36, maxW = SK_W - 42;
		if (k.sub) {
			fitText(tx, SK_Y0 + 2, maxW, k.label, kFont_label, k.color);
			text(tx, SK_Y0 + 20, k.sub, kFont_small, C_DIM);
		} else {
			fitText(tx, SK_Y0 + (SK_Y1 - SK_Y0 - kFont_label.height) / 2, maxW, k.label, kFont_label, k.color);
		}
	}
}

static int softKeyAt(int x, int y)
{
	for (int i = 0; i < 3; i++)
		if (inside(x, y, SK_X0(i), SK_Y0, SK_X0(i) + SK_W, SK_Y1)) return i;
	return -1;
}

static void subHeader(const char* title)
{
	bg(0, 0, W, H);
	text(14, 9, title, kFont_value, C_TEXT);
	button({ BACK_X0, TOP_Y0, BACK_X1, TOP_Y1 }, "Back", C_TEXT);
	fill(6, 40, W - 6, 41, C_BORDER);
}

static bool backPressed(int x, int y)
{
	return inside(x, y, BACK_X0, TOP_Y0, BACK_X1, TOP_Y1);
}

// przyspieszenie enkodera dla wartości 0..100: szybkie kręcenie = większe kroki
static int accelerate(int index, int delta)
{
	static uint32_t last[3] = { 0, 0, 0 };
	uint32_t now = plat_millis();
	uint32_t dt = now - last[index];
	last[index] = now;
	int k = dt < 35 ? 4 : (dt < 90 ? 2 : 1);
	return delta * k;
}

// --- wejście / CPU ---------------------------------------------------------

static constexpr int kInSegments = 12;
static float    meterDb = -60;
static uint32_t clipUntil = 0;
static int      shownLit = -1;
static int      shownClip = -1;
static uint32_t lastMeter = 0;

static void updateInput(uint32_t now)
{
	float peak = plat_input_peak();
	float db = peak > 0.00001f ? 20.0f * log10f(peak) : -100;
	float dt = (now - lastMeter) / 1000.0f;
	meterDb = db > meterDb ? db : meterDb - 24.0f * dt;   // opada 24 dB/s
	if (peak >= 0.99f) clipUntil = now + 1200;
	lastMeter = now;
}

static int litSegments(int nseg)   // -42..0 dBFS na nseg segmentów
{
	int lit = (int)((meterDb + 42.0f) / 42.0f * nseg + 0.5f);
	return lit < 0 ? 0 : (lit > nseg ? nseg : lit);
}

// ======================= ekran główny ======================================

static const Rect kPreCell = { PRE_X0, TOP_Y0, PRE_X1, TOP_Y1 };
static const Rect kInCell  = { IN_X0, TOP_Y0, IN_X1, TOP_Y1 };
static const Rect kVolCell = { VOL_X0, TOP_Y0, VOL_X1, TOP_Y1 };
static const Rect kTunBtn  = { TUN_X0, TOP_Y0, TUN_X1, TOP_Y1 };
static const Rect kSetBtn  = { SET_X0, TOP_Y0, SET_X1, TOP_Y1 };
static const Rect kPanel   = { FX_X0, FX_Y0, FX_X1, FX_Y1 };

static Rect tileRect(int i) { return { TILE_X0(i), TILE_Y0, TILE_X0(i) + TILE_W, TILE_Y1 }; }

static void drawPresetCell()
{
	char buf[4] = { 'P', (char)('1' + st.curPreset), 0 };
	bgUnder(kPreCell);
	card(kPreCell);
	int y = TOP_Y0 + (TOP_Y1 - TOP_Y0 - kFont_label.height) / 2;
	int x = PRE_X0 + 10;
	x += text(x, y, buf, kFont_label, C_GREEN) + 8;
	x += text(x, y, cur.name, kFont_label, C_TEXT) + 4;
	if (modified) text(x, y, "*", kFont_label, C_ORANGE);
	text(PRE_X1 - 16, y, ">", kFont_label, C_DIM);
}

static void setModified()
{
	if (modified) return;
	modified = true;
	drawPresetCell();
}

static void drawMasterMeter()
{
	int cy = BAR_CY(1);
	meter(BAR_X0, BAR_X1, cy - 4, cy + 4, 2 * kInSegments, litSegments(2 * kInSegments));
}

static void drawInputMeter(bool full)
{
	int lit = litSegments(kInSegments);
	bool clip = plat_millis() < clipUntil;
	if (full) {
		bgUnder(kInCell);
		card(kInCell);
		text(IN_X0 + 10, TOP_Y0 + (TOP_Y1 - TOP_Y0 - kFont_small.height) / 2, "IN", kFont_small, C_DIM);
	}
	if (full || lit != shownLit) {
		meter(IN_X0 + 32, IN_X1 - 26, TOP_Y0 + 10, TOP_Y0 + 18, kInSegments, lit);
		if (screen == SCR_MAIN && focused == FOCUS_MASTER) drawMasterMeter();
	}
	if (full || (int)clip != shownClip) {
		float cx = IN_X1 - 14, cy = (TOP_Y0 + TOP_Y1) / 2.0f;
		cardPart(kInCell, { (int)cx - 9, TOP_Y0 + 2, (int)cx + 9, TOP_Y1 - 2 });
		led(cx, cy, clip, C_RED, C_RED_DARK);
	}
	shownLit = lit;
	shownClip = clip;
}

static void drawVolCell()
{
	char buf[4];
	bool f = focused == FOCUS_MASTER;
	bg(VOL_X0 - 5, TOP_Y0 - 5, VOL_X1 + 5, TOP_Y1 + 7);
	card(kVolCell, f);
	text(VOL_X0 + 10, TOP_Y0 + (TOP_Y1 - TOP_Y0 - kFont_small.height) / 2, "VOL", kFont_small, C_DIM);
	snprintf(buf, sizeof(buf), "%d", st.master);
	textRight(VOL_X1 - 10, TOP_Y0 + (TOP_Y1 - TOP_Y0 - kFont_label.height) / 2, buf, kFont_label,
	          f ? C_GREEN : C_TEXT);
}

// ikonki z kształtów: kamerton i suwaki miksera
static void drawTopButtons()
{
	card(kTunBtn);
	float cx = (TUN_X0 + TUN_X1) / 2.0f;
	int y = TOP_Y0;
	roundRect((int)cx - 7, y + 6, (int)cx - 4, y + 17, 1.5f, C_TEXT, C_TEXT);
	roundRect((int)cx + 4, y + 6, (int)cx + 7, y + 17, 1.5f, C_TEXT, C_TEXT);
	roundRect((int)cx - 7, y + 15, (int)cx + 7, y + 19, 2, C_TEXT, C_TEXT);
	roundRect((int)cx - 1, y + 18, (int)cx + 2, y + 24, 1, C_TEXT, C_TEXT);

	card(kSetBtn);
	static const int knob[3] = { 15, 7, 12 };
	for (int i = 0; i < 3; i++) {
		int ly = TOP_Y0 + 8 + i * 6;
		fill(SET_X0 + 8, ly, SET_X1 - 8, ly + 1, C_MUTED);
		circle(SET_X0 + 6.5f + knob[i], ly + 0.5f, 2.6f, C_TEXT);
	}
}

static void drawTile(int i)
{
	Rect r = tileRect(i);
	bool on = slotOn(i);
	int fx = cur.effect[i];

	bgUnder(r);
	card(r, i == focused);
	icon(fx, r.x0 + 10, r.y0 + 8, 2, !on);

	int tx = r.x0 + 60;
	fitText(tx, r.y0 + 6, r.x1 - tx - 6, kEffects[fx].name, kFont_label, C_TEXT);
	text(tx, r.y0 + 25, kEffects[fx].category, kFont_small, C_DIM);
	led(tx + 6, r.y0 + 49, on);
	text(tx + 18, r.y0 + 41, on ? "ON" : "OFF", kFont_small, on ? C_GREEN : C_DIM);
}

static void drawPointer(int i, bool show)
{
	if (i >= UI_NUM_SLOTS) return;
	int cx = TILE_X0(i) + TILE_W / 2;
	if (show) triangleDown(cx, TILE_Y1 + 3, 7, C_GREEN);
	else bg(cx - 8, TILE_Y1 + 2, cx + 8, FX_Y0 - 3);
}

static uint8_t* paramValue(int p)
{
	return focused == FOCUS_MASTER ? &st.master : &cur.params[focused][p];
}

// suwak parametru p (enkoder p+1): tor, wypełnienie, gałka, wartość po prawej
static void drawBar(int p)
{
	int cy = BAR_CY(p);
	int v = *paramValue(p);
	char buf[5];

	cardPart(kPanel, { BAR_X0 - 12, cy - 11, VAL_X1 + 2, cy + 13 }, true);
	slider(BAR_X0, BAR_X1, cy, v);
	snprintf(buf, sizeof(buf), "%d", v);
	textRight(VAL_X1, cy - kFont_value.height / 2, buf, kFont_value, C_GREEN);

	if (focused == FOCUS_MASTER) {   // duża wartość po prawej
		cardPart(kPanel, { BIG_ICON_X0 - 10, BIG_ICON_Y0, FX_X1 - 4, BIG_ICON_Y0 + 60 }, true);
		textCentered(BIG_ICON_X0 - 10, BIG_ICON_X0 + 90, BIG_ICON_Y0 + 10, buf, kFont_title, C_TEXT);
	}
}

// etykieta wiersza: numer enkodera w kółku + nazwa parametru
static void barLabel(int p, const char* name)
{
	float cx = FX_X0 + 24, cy = BAR_CY(p);
	circle(cx, cy, 8, C_GREEN_DARK);
	char n[2] = { (char)('1' + p), 0 };
	textCentered((int)cx - 8, (int)cx + 8, (int)cy - kFont_small.height / 2 - 1, n, kFont_small, C_GREEN_HI);
	text(FX_X0 + 38, (int)cy - kFont_small.height / 2 - 1, name, kFont_small, C_MUTED);
}

static void drawFocusedPanel()
{
	bool m = focused == FOCUS_MASTER;
	const EffectInfo& e = kEffects[cur.effect[m ? 0 : focused]];

	bg(FX_X0 - 5, FX_Y0 - 4, FX_X1 + 5, FX_Y1 + 3);
	drawPointer(focused, true);
	card(kPanel, false, true);

	text(FX_X0 + 16, FX_Y0 + 6, m ? "Master" : e.name, kFont_title, C_TEXT);
	text(FX_X0 + 16, FX_Y0 + 40, m ? "output" : e.category, kFont_small, C_DIM);
	fill(FX_X0 + 16, FX_Y0 + 60, BIG_ICON_X0 - 18, FX_Y0 + 61, C_BORDER);

	if (m) {
		barLabel(0, "volume");
		drawBar(0);
		text(FX_X0 + 38, BAR_CY(1) - kFont_small.height / 2 - 1, "input", kFont_small, C_MUTED);
		drawMasterMeter();
		textCentered(BIG_ICON_X0 - 10, BIG_ICON_X0 + 90, BIG_ICON_Y0 + 52, "master %", kFont_small, C_DIM);
	} else {
		for (int p = 0; p < 3; p++) {
			barLabel(p, e.params[p]);
			drawBar(p);
		}
		icon(cur.effect[focused], BIG_ICON_X0, BIG_ICON_Y0, 1);
	}
}

static void drawMainKeys()
{
	bool m = focused == FOCUS_MASTER;
	char tap[16];
	snprintf(tap, sizeof(tap), "Tap %d", bpm);
	softKeys({ "Next", m ? "to effect 1" : nullptr, C_TEXT },
	         { m ? nullptr : (slotOn(focused) ? "Turn off" : "Turn on"), nullptr, C_TEXT },
	         focusedIsModulation() ? SoftKey { tap, "hold: menu", C_GREEN } : SoftKey { "Menu", nullptr, C_TEXT });
}

static void drawMain()
{
	bg(0, 0, W, H);
	drawPresetCell();
	drawInputMeter(true);
	drawVolCell();
	drawTopButtons();
	for (int i = 0; i < UI_NUM_SLOTS; i++) drawTile(i);
	drawFocusedPanel();
	drawMainKeys();
}

static void setFocus(int f)
{
	int old = focused;
	focused = f;
	drawPointer(old, false);
	if (old < UI_NUM_SLOTS) drawTile(old); else drawVolCell();
	if (f < UI_NUM_SLOTS) drawTile(f); else drawVolCell();
	drawFocusedPanel();
	drawMainKeys();
}

static void toggleFocused()
{
	if (focused == FOCUS_MASTER) return;
	cur.enabled ^= 1 << focused;
	drawTile(focused);
	drawPointer(focused, true);
	drawMainKeys();
	setModified();
}

static void setParam(int p, int v);

static void tapTempo()
{
	uint32_t now = plat_millis();
	uint32_t dt = now - lastTap;
	lastTap = now;
	if (dt > 200 && dt < 2000) {   // 30..300 BPM
		bpm = (60000 + dt / 2) / dt;
		drawMainKeys();
		int v = plat_tap(focused, (float)dt);   // efekt przelicza tempo na swój parametr 1
		if (v >= 0) setParam(0, v);
	}
}

static void setParam(int p, int v)
{
	uint8_t* val = paramValue(p);
	v = clamp100(v);
	if (v == *val) return;
	*val = v;
	drawBar(p);
	if (focused == FOCUS_MASTER) {
		drawVolCell();
		plat_set_master(st.master / 100.0f);
	} else {
		setModified();
	}
}

static void mainButton(int i, UiButtonEvent ev)
{
	if (i == 0) setFocus(focused == FOCUS_MASTER ? 0 : focused + 1);   // ... efekt 3 -> VOL -> efekt 1
	else if (i == 1) toggleFocused();
	else if (ev == UI_HOLD || !focusedIsModulation()) setScreen(SCR_MENU);
	else tapTempo();
}

static void mainPress(int x, int y)
{
	for (int i = 0; i < UI_NUM_SLOTS; i++) {
		Rect r = tileRect(i);
		if (!inside(x, y, r.x0, r.y0, r.x1, r.y1)) continue;
		if (i == focused) toggleFocused();
		else setFocus(i);
		return;
	}

	if (inside(x, y, PRE_X0, TOP_Y0, PRE_X1, TOP_Y1)) { setScreen(SCR_PRESETS); return; }
	if (inside(x, y, TUN_X0, TOP_Y0, TUN_X1, TOP_Y1)) { setScreen(SCR_TUNER); return; }
	if (inside(x, y, SET_X0, TOP_Y0, SET_X1, TOP_Y1)) { setScreen(SCR_SETTINGS); return; }
	if (inside(x, y, VOL_X0, TOP_Y0, VOL_X1, TOP_Y1)) {
		if (focused != FOCUS_MASTER) setFocus(FOCUS_MASTER);
		return;
	}

	int k = softKeyAt(x, y);
	if (k >= 0) { mainButton(k, UI_PRESS); return; }

	bool m = focused == FOCUS_MASTER;
	if (!m && inside(x, y, FX_X0, FX_Y0, FX_X1, FX_TITLE_Y1)) { setScreen(SCR_PICKER); return; }

	// przeciąganie suwaka
	for (int p = 0; p < (m ? 1 : 3); p++) {
		int cy = BAR_CY(p);
		if (inside(x, y, BAR_X0 - 12, cy - 12, BAR_X1 + 12, cy + 12)) {
			dragParam = p;
			return;
		}
	}
}

// ======================= menu ===============================================

static constexpr int kMenuItems = 4;
static int menuCursor = 0;

static void drawMenuItem(int i)
{
	static const char* const kTitles[kMenuItems] = { "Change effect", "Presets", "Tuner", "Settings" };
	Rect r = { MN_X0, MN_Y(i), MN_X1, MN_Y(i) + MN_H };
	char sub[32] = "";
	if (i == 0 && focused != FOCUS_MASTER)
		snprintf(sub, sizeof(sub), "Slot %d: %s", focused + 1, kEffects[cur.effect[focused]].name);
	if (i == 1) snprintf(sub, sizeof(sub), "P%d %s%s", st.curPreset + 1, cur.name, modified ? " *" : "");
	if (i == 2) strcpy(sub, "full screen");
	if (i == 3) strcpy(sub, "brightness, footswitch, system");

	bool enabled = !(i == 0 && focused == FOCUS_MASTER);
	bgUnder(r);
	card(r, i == menuCursor);
	text(r.x0 + 18, r.y0 + (MN_H - kFont_value.height) / 2, kTitles[i], kFont_value, enabled ? C_TEXT : C_DIM);
	textRight(r.x1 - 40, r.y0 + (MN_H - kFont_small.height) / 2, sub, kFont_small, C_DIM);
	text(r.x1 - 24, r.y0 + (MN_H - kFont_label.height) / 2, ">", kFont_label, C_DIM);
}

static void drawMenu()
{
	subHeader("Menu");
	for (int i = 0; i < kMenuItems; i++) drawMenuItem(i);
	softKeys({ "Open", nullptr, C_TEXT }, { nullptr, nullptr, C_TEXT }, { "Back", nullptr, C_TEXT });
}

static void menuMove(int d)
{
	int old = menuCursor;
	menuCursor = wrap(menuCursor + d, kMenuItems);
	drawMenuItem(old);
	drawMenuItem(menuCursor);
}

static void menuOpen(int i)
{
	static const Screen kTargets[kMenuItems] = { SCR_PICKER, SCR_PRESETS, SCR_TUNER, SCR_SETTINGS };
	if (i == 0 && focused == FOCUS_MASTER) return;
	setScreen(kTargets[i]);
}

static void menuPress(int x, int y)
{
	if (backPressed(x, y)) { setScreen(SCR_MAIN); return; }
	int k = softKeyAt(x, y);
	if (k == 0) { menuOpen(menuCursor); return; }
	if (k == 2) { setScreen(SCR_MAIN); return; }
	for (int i = 0; i < kMenuItems; i++)
		if (inside(x, y, MN_X0, MN_Y(i), MN_X1, MN_Y(i) + MN_H)) {
			menuCursor = i;
			menuOpen(i);
			return;
		}
}

// ======================= wybór efektu =======================================

static int pickCursor = 0;

static void drawPickCell(int k)
{
	Rect r = { PICK_X0(k), PICK_Y(k), PICK_X0(k) + PICK_W, PICK_Y(k) + PICK_H };
	bool current = k == cur.effect[focused];
	bgUnder(r);
	card(r, k == pickCursor);
	icon(k, r.x0 + 6, r.y0 + 5, 3);   // 26x32
	fitText(r.x0 + 42, r.y0 + 3, r.x1 - r.x0 - 60, kEffects[k].name, kFont_label, C_TEXT);
	text(r.x0 + 42, r.y0 + 22, kEffects[k].category, kFont_small, C_DIM);
	if (current) led(r.x1 - 12, r.y0 + 12, true);
}

static void drawPicker()
{
	char buf[24];
	subHeader("Choose effect");
	snprintf(buf, sizeof(buf), "Slot %d", focused + 1);
	text(14 + textWidth("Choose effect", kFont_value) + 12, 9, buf, kFont_value, C_GREEN);
	for (int k = 0; k < kNumEffects; k++) drawPickCell(k);
	softKeys({ "Select", nullptr, C_TEXT }, { nullptr, nullptr, C_TEXT }, { "Back", nullptr, C_TEXT });
}

static void pickerMove(int d)
{
	int old = pickCursor;
	pickCursor = wrap(pickCursor + d, kNumEffects);
	drawPickCell(old);
	drawPickCell(pickCursor);
}

static void pickerChoose(int k)
{
	if (k != cur.effect[focused]) {
		cur.effect[focused] = k;
		cur.enabled &= ~(1 << focused);
		for (int p = 0; p < 3; p++) cur.params[focused][p] = 50;
		modified = true;
	}
	setScreen(SCR_MAIN);
}

static void pickerPress(int x, int y)
{
	if (backPressed(x, y)) { setScreen(SCR_MAIN); return; }
	int sk = softKeyAt(x, y);
	if (sk == 0) { pickerChoose(pickCursor); return; }
	if (sk == 2) { setScreen(SCR_MAIN); return; }
	for (int k = 0; k < kNumEffects; k++)
		if (inside(x, y, PICK_X0(k), PICK_Y(k), PICK_X0(k) + PICK_W, PICK_Y(k) + PICK_H)) {
			pickerChoose(k);
			return;
		}
}

// ======================= presety =============================================

static int prSelected = 0;
static bool saveArmed = false;
static uint32_t saveArmedAt = 0;

static Rect presetRect(int i) { return { PR_X0(i), PR_Y(i), PR_X0(i) + PR_W, PR_Y(i) + PR_H }; }

static void drawPresetCard(int i)
{
	const Preset& p = st.presets[i];
	Rect r = presetRect(i);
	char buf[4] = { 'P', (char)('1' + i), 0 };

	bgUnder(r);
	card(r, i == prSelected);
	int ty = r.y0 + (PR_H - kFont_value.height) / 2;
	int x = r.x0 + 28;
	led(r.x0 + 15, r.y0 + PR_H / 2.0f, i == st.curPreset);   // wczytany preset
	x += text(x, ty, buf, kFont_value, C_GREEN) + 8;
	int iconsX0 = r.x1 - 10 - UI_NUM_SLOTS * 24;
	fitText(x, ty, iconsX0 - x - 6, p.name, kFont_value, C_TEXT);

	// skład łańcucha: miniatury 20x24, wyłączone przyciemnione
	for (int s = 0; s < UI_NUM_SLOTS; s++)
		icon(p.effect[s], iconsX0 + 4 + s * 24, r.y0 + 12, 4, !(p.enabled & (1 << s)));
}

static void drawPresetKeys()
{
	char load[24], saveTo[24];
	snprintf(load, sizeof(load), "Load P%d", prSelected + 1);
	snprintf(saveTo, sizeof(saveTo), "Save to P%d", prSelected + 1);
	softKeys({ load, nullptr, C_TEXT },
	         saveArmed ? SoftKey { "Press again", "to overwrite", C_ORANGE } : SoftKey { saveTo, nullptr, C_GREEN },
	         { "Back", nullptr, C_TEXT });
}

static void drawPresets()
{
	subHeader("Presets");
	if (modified)
		text(14 + textWidth("Presets", kFont_value) + 14, 13, "* unsaved changes", kFont_small, C_ORANGE);
	for (int i = 0; i < UI_NUM_PRESETS; i++) drawPresetCard(i);
	drawPresetKeys();
}

static void presetsSelect(int i)
{
	if (i == prSelected) return;
	int old = prSelected;
	prSelected = i;
	saveArmed = false;
	drawPresetCard(old);
	drawPresetCard(i);
	drawPresetKeys();
}

static void presetsSave()
{
	if (!saveArmed) {   // nadpisanie presetu: potwierdzenie drugim wciśnięciem
		saveArmed = true;
		saveArmedAt = plat_millis();
		drawPresetKeys();
		return;
	}
	saveArmed = false;
	memcpy(cur.name, st.presets[prSelected].name, sizeof(cur.name));   // nazwa zostaje
	st.presets[prSelected] = cur;
	st.curPreset = prSelected;
	modified = false;
	save();
	drawPresets();
}

static void presetsLoad()
{
	loadPreset(prSelected);
	setScreen(SCR_MAIN);
}

static void presetsPress(int x, int y)
{
	if (backPressed(x, y)) { setScreen(SCR_MAIN); return; }
	int k = softKeyAt(x, y);
	if (k == 0) { presetsLoad(); return; }
	if (k == 1) { presetsSave(); return; }
	if (k == 2) { setScreen(SCR_MAIN); return; }
	for (int i = 0; i < UI_NUM_PRESETS; i++) {
		Rect r = presetRect(i);
		if (inside(x, y, r.x0, r.y0, r.x1, r.y1)) {
			presetsSelect(i);
			return;
		}
	}
}

// ======================= tuner ==============================================

static const char* const kStringLabels[6] = { "E2", "A2", "D3", "G3", "B3", "E4" };
static const char  kStringNotes[6] = { 'E', 'A', 'D', 'G', 'B', 'E' };
static const float kStringFreqs[6] = { 82.41f, 110.0f, 146.83f, 196.0f, 246.94f, 329.63f };

#define TN_STR_Y0   50
#define TN_STR_Y1   84
#define TN_NOTE_Y0  92
#define TN_NOTE_Y1  198
#define TN_G_X0     40
#define TN_G_X1     440
#define TN_G_CY     222
#define TN_MSG_Y    272

static int  shownString = -2;
static int  shownNoteKey = -100;   // struna * 4 + stan narysowanej dużej nuty
static int  shownTunerState = -2;  // -1 brak, 0 w stroju, 1 za wysoko, 2 za nisko
static uint32_t lastTuner = 0;

static Rect stringRect(int s) { return { 32 + s * 72, TN_STR_Y0, 32 + s * 72 + 56, TN_STR_Y1 }; }

static void drawStringBox(int s, bool active)
{
	Rect r = stringRect(s);
	bgUnder(r);
	card(r, active);
	textCentered(r.x0, r.x1, r.y0 + (34 - kFont_label.height) / 2, kStringLabels[s], kFont_label,
	             active ? C_GREEN : C_MUTED);
}

static int gaugeX(float cents)
{
	if (cents < -50) cents = -50;
	if (cents > 50) cents = 50;
	return TN_G_X0 + (int)((TN_G_X1 - TN_G_X0) * (cents + 50) / 100);
}

static void drawGauge(bool valid, float cents, Color c)
{
	bg(TN_G_X0 - 16, TN_G_CY - 20, TN_G_X1 + 16, TN_G_CY + 20);
	roundRect(TN_G_X0 - 1, TN_G_CY - 11, TN_G_X1 + 1, TN_G_CY + 11, 11, C_BORDER, C_BORDER);
	roundRect(TN_G_X0, TN_G_CY - 10, TN_G_X1, TN_G_CY + 10, 10, C_TRACK, rgb(0x22, 0x22, 0x22));
	roundRect(gaugeX(-5), TN_G_CY - 10, gaugeX(5), TN_G_CY + 10, 3, C_GREEN, C_GREEN, 70);   // ±5 centów
	for (int k = -50; k <= 50; k += 10) {
		int tx = gaugeX((float)k);
		fill(tx, TN_G_CY + 13, tx + 1, TN_G_CY + (k == 0 ? 20 : 17), k == 0 ? C_MUTED : C_DIM);
	}
	if (valid) {
		int nx = gaugeX(cents);
		glow(nx, TN_G_CY, 18, c, 90);
		roundRect(nx - 3, TN_G_CY - 17, nx + 4, TN_G_CY + 17, 3.5f, C_TEXT, c);
	}
}

static void drawTuner()
{
	subHeader("Tuner");
	text(14 + textWidth("Tuner", kFont_value) + 14, 13, "push 3: back", kFont_small, C_DIM);
	for (int s = 0; s < 6; s++) drawStringBox(s, false);
	text(TN_G_X0 - 10, TN_G_CY + 24, "-50", kFont_small, C_DIM);
	textCentered(TN_G_X0, TN_G_X1, TN_G_CY + 24, "0", kFont_small, C_DIM);
	textRight(TN_G_X1 + 10, TN_G_CY + 24, "+50", kFont_small, C_DIM);
	drawGauge(false, 0, C_GREEN);
	shownString = -2;
	shownNoteKey = -100;
	shownTunerState = -2;
}

static void updateTuner()
{
	float hz = 0;
	bool demo = false;
	bool valid = plat_tuner(&hz, &demo);

	// najbliższa struna (jak w Tuner z wersji PC) i odchyłka w centach
	int s = -1;
	float cents = 0;
	if (valid) {
		float best = 1e9f;
		for (int i = 0; i < 6; i++) {
			float c = 1200.0f * log2f(hz / kStringFreqs[i]);
			if (fabsf(c) < fabsf(best)) { best = c; s = i; }
		}
		cents = best;
	}
	int state = !valid ? -1 : (fabsf(cents) < 5 ? 0 : (cents > 0 ? 1 : 2));
	Color c = state == 0 ? C_GREEN : C_ORANGE;

	if (s != shownString) {
		if (shownString >= 0) drawStringBox(shownString, false);
		if (s >= 0) drawStringBox(s, true);
		shownString = s;
	}

	// duża nuta / "No signal" — tylko gdy zmieni się struna albo stan (dużo pikseli po SPI)
	int noteKey = s * 4 + state;
	if (noteKey != shownNoteKey) {
		bg(20, TN_NOTE_Y0, W - 20, TN_NOTE_Y1);
		if (valid) {
			char note[2] = { kStringNotes[s], 0 };
			text(232 - textWidth(note, kFont_big), TN_NOTE_Y0 - 6, note, kFont_big, state == 0 ? C_GREEN : C_TEXT);
		} else {
			textCentered(0, W, TN_NOTE_Y0 + 40, "No signal... play a note", kFont_body, C_DIM);
		}
		shownNoteKey = noteKey;
	}

	// częstotliwość + centy: co odświeżenie, mały obszar obok nuty
	if (valid) {
		char buf[24];
		bg(258, TN_NOTE_Y0 + 20, W - 70, TN_NOTE_Y0 + 90);
		fmt1(buf, sizeof(buf), hz, " Hz");
		text(262, TN_NOTE_Y0 + 26, buf, kFont_value, C_MUTED);
		snprintf(buf, sizeof(buf), "%+d cents", (int)lroundf(cents));
		text(262, TN_NOTE_Y0 + 56, buf, kFont_value, c);
	}
	if (demo) {
		bg(W - 66, TN_NOTE_Y0, W - 20, TN_NOTE_Y0 + 18);
		text(W - 64, TN_NOTE_Y0, "DEMO", kFont_small, C_ORANGE);
	}

	drawGauge(valid, cents, c);

	if (state != shownTunerState) {
		bg(0, TN_MSG_Y - 4, W, TN_MSG_Y + 26);
		const char* msg = state == 0 ? "IN TUNE"
		                : state == 1 ? "too high - loosen the string"
		                : state == 2 ? "too low - tighten the string" : "";
		textCentered(0, W, TN_MSG_Y, msg, kFont_value, c);
		shownTunerState = state;
	}
}

// ======================= ustawienia =========================================
// wiersze: 0 jasność, 1 footswitche, 2 reset (wybieralne), 3 system (CPU + wersja)

static constexpr int kSettingRows = 3;
static int  stCursor = 0;
static bool resetArmed = false;
static uint32_t resetArmedAt = 0;
static uint32_t lastCpu = 0;

static Rect rowRect(int r) { return { ST_X0, ST_Y(r), ST_X1, ST_Y(r) + ST_ROW_H }; }

static void drawRowCard(int r, const char* label)
{
	Rect rr = rowRect(r);
	bgUnder(rr);
	card(rr, r == stCursor && r < kSettingRows, true);
	text(ST_X0 + 16, ST_Y(r) + (ST_ROW_H - kFont_label.height) / 2, label, kFont_label,
	     r == 2 ? C_RED : C_TEXT);
}

static void drawBrightness()
{
	char buf[6];
	int cy = ST_Y(0) + ST_ROW_H / 2;
	cardPart(rowRect(0), { ST_CTL_X0 - 12, cy - 12, ST_X1 - 6, cy + 14 }, true);
	slider(ST_CTL_X0, ST_BAR_X1, cy, st.brightness);
	snprintf(buf, sizeof(buf), "%d%%", st.brightness);
	textRight(ST_X1 - 14, cy - kFont_value.height / 2, buf, kFont_value, C_GREEN);
}

static void drawFsMode()
{
	int y0 = ST_Y(1) + 6, y1 = y0 + 22;
	cardPart(rowRect(1), { FS_X0 - 4, y0 - 3, FS_X1 + 3, ST_Y(1) + ST_ROW_H - 3 }, true);
	roundRect(FS_X0, y0, FS_X1, y1, 11, C_TRACK, C_TRACK);
	int sx0 = st.fsMode == 0 ? FS_X0 + 2 : FS_MID + 1;
	int sx1 = st.fsMode == 0 ? FS_MID - 1 : FS_X1 - 2;
	roundRect(sx0, y0 + 2, sx1, y1 - 2, 9, C_GREEN_HI, C_GREEN);
	textCentered(FS_X0, FS_MID, y0 + 3, "Effects", kFont_small, st.fsMode == 0 ? C_BLACK : C_MUTED);
	textCentered(FS_MID, FS_X1, y0 + 3, "Presets", kFont_small, st.fsMode == 1 ? C_BLACK : C_MUTED);
	text(FS_X0 + 4, y1 + 2, st.fsMode == 0 ? "FS1-FS3: effect on/off"
	                                      : "FS1 / FS2: preset - / +,  FS3: tap", kFont_small, C_DIM);
}

static void drawReset()
{
	int y = ST_Y(2) + (ST_ROW_H - kFont_small.height) / 2;
	cardPart(rowRect(2), { ST_CTL_X0 - 4, ST_Y(2) + 4, ST_X1 - 6, ST_Y(2) + ST_ROW_H - 4 }, true);
	text(ST_CTL_X0, y, resetArmed ? "press OK again to reset!" : "presets, brightness, footswitch, VOL",
	     kFont_small, resetArmed ? C_RED : C_DIM);
}

static void drawCpu()
{
	char buf[24];
	int cy = ST_Y(3) + 16;
	float load = plat_cpu_load() * 100.0f;
	cardPart(rowRect(3), { ST_CTL_X0 - 4, cy - 12, ST_X1 - 6, cy + 10 }, true);
	fmt1(buf, sizeof(buf), load, " %  CPU");
	text(ST_CTL_X0, cy - kFont_label.height / 2, buf, kFont_label, load > 50 ? C_ORANGE : C_GREEN);
	int bx0 = ST_CTL_X0 + 120, bx1 = ST_X1 - 16;
	roundRect(bx0, cy - 3, bx1, cy + 3, 3, C_TRACK, C_TRACK);
	int bx = bx0 + (int)((bx1 - bx0) * (load > 100 ? 100 : load) / 100);
	if (bx > bx0 + 4) roundRect(bx0, cy - 3, bx, cy + 3, 3, load > 50 ? C_ORANGE : C_GREEN_HI,
	                            load > 50 ? C_ORANGE : C_GREEN);
}

static void drawSettingsKeys()
{
	const char* ok = stCursor == 1 ? "Toggle" : (stCursor == 2 ? (resetArmed ? "Confirm" : "Reset") : nullptr);
	softKeys({ ok, nullptr, stCursor == 2 ? C_RED : C_TEXT },
	         { stCursor < 2 ? "Value" : nullptr, stCursor < 2 ? "turn 2" : nullptr, C_TEXT },
	         { "Back", nullptr, C_TEXT });
}

static void drawSettingRow(int r)
{
	static const char* const kLabels[4] = { "Brightness", "Footswitch", "Reset settings", "System" };
	drawRowCard(r, kLabels[r]);
	switch (r) {
	case 0: drawBrightness(); break;
	case 1: drawFsMode(); break;
	case 2: drawReset(); break;
	case 3:
		drawCpu();
		text(ST_CTL_X0, ST_Y(3) + 28, "Multieffect " FW_VERSION ",  48 kHz / 48,  " __DATE__, kFont_small, C_DIM);
		break;
	}
}

static void drawSettings()
{
	subHeader("Settings");
	resetArmed = false;
	for (int r = 0; r < 4; r++) drawSettingRow(r);
	drawSettingsKeys();
}

static void settingsMove(int d)
{
	int old = stCursor;
	stCursor = wrap(stCursor + d, kSettingRows);
	if (resetArmed) resetArmed = false;
	drawSettingRow(old);
	drawSettingRow(stCursor);
	drawSettingsKeys();
}

static void setBrightness(int v)
{
	v = clamp100(v);
	if (v == st.brightness) return;
	st.brightness = v;
	plat_backlight(v);
	drawBrightness();
}

static void setFsMode(int m)
{
	if (m == st.fsMode) return;
	st.fsMode = m;
	save();
	drawFsMode();
}

static void settingsOk()
{
	if (stCursor == 1) {
		setFsMode(!st.fsMode);
	} else if (stCursor == 2) {
		if (!resetArmed) {
			resetArmed = true;
			resetArmedAt = plat_millis();
			drawReset();
			drawSettingsKeys();
		} else {
			factoryReset();
			focused = 0;
			setScreen(SCR_MAIN);
		}
	}
}

static void settingsPress(int x, int y)
{
	if (backPressed(x, y)) { setScreen(SCR_MAIN); return; }
	int k = softKeyAt(x, y);
	if (k == 0) { settingsOk(); return; }
	if (k == 2) { setScreen(SCR_MAIN); return; }

	for (int r = 0; r < kSettingRows; r++) {
		if (!inside(x, y, ST_X0, ST_Y(r), ST_X1, ST_Y(r) + ST_ROW_H)) continue;
		if (r != stCursor) settingsMove(r - stCursor);
		if (r == 0 && x >= ST_CTL_X0 - 12) dragParam = 0;
		if (r == 1 && x >= FS_X0) setFsMode(x < FS_MID ? 0 : 1);
		if (r == 2) settingsOk();
		return;
	}
}

// ======================= przełączanie / wejścia ==============================

static void setScreen(Screen s)
{
	screen = s;
	dragParam = -1;
	switch (s) {
	case SCR_MAIN:     drawMain(); break;
	case SCR_MENU:     drawMenu(); break;
	case SCR_PICKER:   pickCursor = cur.effect[focused]; drawPicker(); break;
	case SCR_PRESETS:  prSelected = st.curPreset; saveArmed = false; drawPresets(); break;
	case SCR_TUNER:    drawTuner(); break;
	case SCR_SETTINGS: stCursor = 0; drawSettings(); break;
	}
}

void ui_init(uint16_t* framebuffer)
{
	gfx::init(framebuffer);
	plat_load(&st, sizeof(st));
	if (st.magic != kMagic) {
		factoryReset();   // pierwszy start: presety fabryczne
	} else {
		cur = st.presets[st.curPreset % UI_NUM_PRESETS];
		applyOutputs();
	}
	setScreen(SCR_MAIN);
	gfx::flush();
}

const Preset& ui_current() { return cur; }

void ui_touch(bool pressed, int x, int y)
{
	if (pressed && !wasPressed) {
		switch (screen) {
		case SCR_MAIN:     mainPress(x, y); break;
		case SCR_MENU:     menuPress(x, y); break;
		case SCR_PICKER:   pickerPress(x, y); break;
		case SCR_PRESETS:  presetsPress(x, y); break;
		case SCR_TUNER:    if (backPressed(x, y)) setScreen(SCR_MAIN); break;
		case SCR_SETTINGS: settingsPress(x, y); break;
		}
	}
	if (pressed && dragParam >= 0) {
		if (screen == SCR_MAIN) setParam(dragParam, valueFromX(x, BAR_X0, BAR_X1));
		else if (screen == SCR_SETTINGS) setBrightness(valueFromX(x, ST_CTL_X0, ST_BAR_X1));
	}
	if (!pressed && wasPressed && dragParam >= 0) {
		if (screen == SCR_SETTINGS || focused == FOCUS_MASTER) save();   // ustawienia globalne
		dragParam = -1;
	}
	wasPressed = pressed;
}

static uint32_t pendingSaveAt = 0;   // zapis po kręceniu master/jasności, gdy enkoder się zatrzyma

void ui_encoder(int index, int delta)
{
	if (delta == 0 || index < 0 || index > 2) return;
	switch (screen) {
	case SCR_MAIN:
		if (focused == FOCUS_MASTER && index != 0) return;
		setParam(index, *paramValue(index) + accelerate(index, delta));
		if (focused == FOCUS_MASTER) pendingSaveAt = plat_millis() + 1000;
		break;
	case SCR_MENU:
		if (index == 0) menuMove(delta);
		break;
	case SCR_PICKER:
		if (index == 0) pickerMove(delta);
		break;
	case SCR_PRESETS:
		if (index == 0) presetsSelect(wrap(prSelected + delta, UI_NUM_PRESETS));
		break;
	case SCR_SETTINGS:
		if (index == 0) settingsMove(delta);
		else if (index == 1 && stCursor == 0) {
			setBrightness(st.brightness + accelerate(index, delta));
			pendingSaveAt = plat_millis() + 1000;
		} else if (index == 1 && stCursor == 1) {
			setFsMode(delta > 0 ? 1 : 0);
		}
		break;
	case SCR_TUNER:
		break;
	}
}

void ui_button(int index, UiButtonEvent ev)
{
	switch (screen) {
	case SCR_MAIN:
		mainButton(index, ev);
		break;
	case SCR_MENU:
		if (index == 0) menuOpen(menuCursor);
		else if (index == 2) setScreen(SCR_MAIN);
		break;
	case SCR_PICKER:
		if (index == 0) pickerChoose(pickCursor);
		else if (index == 2) setScreen(SCR_MAIN);
		break;
	case SCR_PRESETS:
		if (index == 0) presetsLoad();
		else if (index == 1) presetsSave();
		else setScreen(SCR_MAIN);
		break;
	case SCR_TUNER:
		if (index == 2) setScreen(SCR_MAIN);
		break;
	case SCR_SETTINGS:
		if (index == 0) settingsOk();
		else if (index == 2) setScreen(SCR_MAIN);
		break;
	}
}

void ui_tick()
{
	uint32_t now = plat_millis();

	if (now - lastMeter >= 30) {
		updateInput(now);
		if (screen == SCR_MAIN) drawInputMeter(false);
	}
	if (screen == SCR_TUNER && now - lastTuner >= 100) {
		lastTuner = now;
		updateTuner();
	}
	if (screen == SCR_SETTINGS) {
		if (resetArmed && now - resetArmedAt > 3000) {   // potwierdzenie wygasa
			resetArmed = false;
			drawReset();
			drawSettingsKeys();
		}
		if (now - lastCpu >= 500) {
			lastCpu = now;
			drawCpu();
		}
	}
	if (screen == SCR_PRESETS && saveArmed && now - saveArmedAt > 3000) {
		saveArmed = false;
		drawPresetKeys();
	}
	if (pendingSaveAt && (int32_t)(now - pendingSaveAt) >= 0) {
		pendingSaveAt = 0;
		save();
	}
	gfx::flush();
}
