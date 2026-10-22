// Rysowanie do framebuffera 480x320 RGB565 (na Daisy w SDRAM).
// Wszystko z wygładzaniem (alfa) mieszanym z tym, co już jest w buforze;
// flush() wysyła na ekran tylko zmienione prostokąty (platform.h).
#pragma once
#include <stdint.h>
#include "font.h"

namespace gfx {

constexpr int W = 480;
constexpr int H = 320;

using Color = uint16_t;

constexpr Color rgb(int r, int g, int b)
{
	return (Color)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

struct Rect {
	int x0, y0, x1, y1;   // x1, y1 wyłącznie
};

void init(Color* framebuffer);
void flush();

Color blend(Color dst, Color src, int alpha);           // alpha 0..255
Color mix(Color a, Color b, int t, int n);              // a -> b, t z 0..n

void fill(int x0, int y0, int x1, int y1, Color c);
void fillAlpha(int x0, int y0, int x1, int y1, Color c, int alpha);
void gradient(int x0, int y0, int x1, int y1, Color top, Color bottom);
// gradient liczony względem pionowego zakresu from (np. całej karty), rysowany tylko w r
void gradientPart(const Rect& from, const Rect& r, Color top, Color bottom);

void roundRect(int x0, int y0, int x1, int y1, float radius, Color top, Color bottom, int alpha = 255);
void shadow(int x0, int y0, int x1, int y1, float radius, int spread, int alpha);
void circle(float cx, float cy, float r, Color c, int alpha = 255);
void glow(float cx, float cy, float r, Color c, int alpha);   // miękka poświata, zanika do krawędzi
void triangleDown(int cx, int y0, int halfWidth, Color c);
// łuk o promieniu r i grubości w, kąty w radianach (0 = w prawo, rośnie zgodnie z zegarem
// bo y w dół), od a0 do a1 (a1 > a0), z zaokrąglonymi końcami
void arc(float cx, float cy, float r, float w, float a0, float a1, Color c, int alpha = 255);

int  textWidth(const char* s, const Font& f);
int  text(int x, int y, const char* s, const Font& f, Color c);   // y = góra linii; zwraca szerokość
void textCentered(int x0, int x1, int y, const char* s, const Font& f, Color c);
void textRight(int x1, int y, const char* s, const Font& f, Color c);

// obraz RGB565 w*h, pomniejszony div razy (średnia z bloku div x div), przyciemniony dim (0 = bez)
void image(int x, int y, const uint16_t* px, int w, int h, int div, int dim = 0);

}   // namespace gfx
