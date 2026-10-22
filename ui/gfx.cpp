#include "gfx.h"
#include "platform.h"
#include <math.h>
#include <string.h>

namespace gfx {

static Color* fb = nullptr;

// --- zmienione obszary -------------------------------------------------------

static constexpr int kMaxDirty = 16;
static Rect dirty[kMaxDirty];
static int  dirtyCount = 0;

static bool touches(const Rect& a, const Rect& b)
{
	return a.x0 <= b.x1 && b.x0 <= a.x1 && a.y0 <= b.y1 && b.y0 <= a.y1;
}

static void unite(Rect& a, const Rect& b)
{
	if (b.x0 < a.x0) a.x0 = b.x0;
	if (b.y0 < a.y0) a.y0 = b.y0;
	if (b.x1 > a.x1) a.x1 = b.x1;
	if (b.y1 > a.y1) a.y1 = b.y1;
}

static void markDirty(Rect r)
{
	// wchłoń wszystkie prostokąty, których dotyka (r rośnie, więc od nowa)
	for (int i = 0; i < dirtyCount;) {
		if (touches(dirty[i], r)) {
			unite(r, dirty[i]);
			dirty[i] = dirty[--dirtyCount];
			i = 0;
		} else {
			i++;
		}
	}
	if (dirtyCount < kMaxDirty) dirty[dirtyCount++] = r;
	else unite(dirty[0], r);
}

// przycina do ekranu; false = nic do rysowania
static bool clip(int& x0, int& y0, int& x1, int& y1)
{
	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 > W) x1 = W;
	if (y1 > H) y1 = H;
	if (x0 >= x1 || y0 >= y1) return false;
	markDirty({ x0, y0, x1, y1 });
	return true;
}

void init(Color* framebuffer)
{
	fb = framebuffer;
	dirtyCount = 0;
}

void flush()
{
	for (int i = 0; i < dirtyCount; i++)
		plat_lcd_write(dirty[i].x0, dirty[i].y0, dirty[i].x1, dirty[i].y1, fb);
	dirtyCount = 0;
}

// --- kolory ------------------------------------------------------------------

Color blend(Color d, Color s, int a)
{
	if (a >= 255) return s;
	if (a <= 0) return d;
	int r = ((s >> 11) * a + (d >> 11) * (255 - a)) / 255;
	int g = (((s >> 5) & 0x3F) * a + ((d >> 5) & 0x3F) * (255 - a)) / 255;
	int b = ((s & 0x1F) * a + (d & 0x1F) * (255 - a)) / 255;
	return (Color)((r << 11) | (g << 5) | b);
}

Color mix(Color a, Color b, int t, int n)
{
	return n <= 0 ? a : blend(a, b, t * 255 / n);
}

static inline void plot(int x, int y, Color c, int a)
{
	Color& p = fb[y * W + x];
	p = blend(p, c, a);
}

// --- prostokąty ---------------------------------------------------------------

void fill(int x0, int y0, int x1, int y1, Color c)
{
	if (!clip(x0, y0, x1, y1)) return;
	for (int y = y0; y < y1; y++) {
		Color* row = fb + y * W;
		for (int x = x0; x < x1; x++) row[x] = c;
	}
}

void fillAlpha(int x0, int y0, int x1, int y1, Color c, int alpha)
{
	if (!clip(x0, y0, x1, y1)) return;
	for (int y = y0; y < y1; y++)
		for (int x = x0; x < x1; x++) plot(x, y, c, alpha);
}

void gradientPart(const Rect& from, const Rect& r, Color top, Color bottom)
{
	int h = from.y1 - from.y0 - 1;
	for (int y = r.y0; y < r.y1; y++)
		fill(r.x0, y, r.x1, y + 1, mix(top, bottom, y - from.y0, h));
}

void gradient(int x0, int y0, int x1, int y1, Color top, Color bottom)
{
	Rect r = { x0, y0, x1, y1 };
	gradientPart(r, r, top, bottom);
}

// pokrycie piksela (środek) przez prostokąt z zaokrąglonymi rogami
static float coverage(float px, float py, float x0, float y0, float x1, float y1, float r)
{
	float dx = 0, dy = 0;
	if (px < x0 + r) dx = x0 + r - px;
	else if (px > x1 - r) dx = px - (x1 - r);
	if (py < y0 + r) dy = y0 + r - py;
	else if (py > y1 - r) dy = py - (y1 - r);
	float d = (dx == 0 && dy == 0) ? 0 : sqrtf(dx * dx + dy * dy);
	float c = r - d + 0.5f;
	if (dx == 0 && dy == 0) {
		// krawędź prosta: pokrycie od odległości do boku
		float e = px - x0;
		if (x1 - px < e) e = x1 - px;
		if (py - y0 < e) e = py - y0;
		if (y1 - py < e) e = y1 - py;
		c = e + 0.5f;
	}
	return c < 0 ? 0 : (c > 1 ? 1 : c);
}

void roundRect(int x0, int y0, int x1, int y1, float radius, Color top, Color bottom, int alpha)
{
	int cx0 = x0, cy0 = y0, cx1 = x1, cy1 = y1;
	if (!clip(cx0, cy0, cx1, cy1)) return;
	int h = y1 - y0 - 1;
	int ri = (int)ceilf(radius) + 1;
	for (int y = cy0; y < cy1; y++) {
		Color c = mix(top, bottom, y - y0, h);
		bool edgeRow = y < y0 + ri || y >= y1 - ri;
		for (int x = cx0; x < cx1; x++) {
			bool edge = edgeRow || x < x0 + ri || x >= x1 - ri;
			if (!edge) {
				plot(x, y, c, alpha);
				continue;
			}
			float cov = coverage(x + 0.5f, y + 0.5f, (float)x0, (float)y0, (float)x1, (float)y1, radius);
			if (cov > 0) plot(x, y, c, (int)(cov * alpha));
		}
	}
}

void shadow(int x0, int y0, int x1, int y1, float radius, int spread, int alpha)
{
	// kilka coraz większych, coraz słabszych warstw = miękki cień
	for (int i = spread; i >= 1; i--) {
		int a = alpha / (spread + 1);
		roundRect(x0 - i, y0 - i + spread / 2, x1 + i, y1 + i + spread / 2, radius + i, 0, 0, a);
	}
}

void circle(float cx, float cy, float r, Color c, int alpha)
{
	int x0 = (int)floorf(cx - r - 1), x1 = (int)ceilf(cx + r + 1);
	int y0 = (int)floorf(cy - r - 1), y1 = (int)ceilf(cy + r + 1);
	if (!clip(x0, y0, x1, y1)) return;
	for (int y = y0; y < y1; y++)
		for (int x = x0; x < x1; x++) {
			float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
			float cov = r - sqrtf(dx * dx + dy * dy) + 0.5f;
			if (cov <= 0) continue;
			plot(x, y, c, cov >= 1 ? alpha : (int)(cov * alpha));
		}
}

void glow(float cx, float cy, float r, Color c, int alpha)
{
	int x0 = (int)floorf(cx - r), x1 = (int)ceilf(cx + r);
	int y0 = (int)floorf(cy - r), y1 = (int)ceilf(cy + r);
	if (!clip(x0, y0, x1, y1)) return;
	for (int y = y0; y < y1; y++)
		for (int x = x0; x < x1; x++) {
			float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
			float t = 1.0f - sqrtf(dx * dx + dy * dy) / r;
			if (t > 0) plot(x, y, c, (int)(t * t * alpha));
		}
}

void triangleDown(int cx, int y0, int halfWidth, Color c)
{
	for (int r = 0; r < halfWidth; r++)
		fill(cx - (halfWidth - r), y0 + r, cx + (halfWidth - r), y0 + r + 1, c);
}

void arc(float cx, float cy, float r, float w, float a0, float a1, Color c, int alpha)
{
	const float twoPi = 6.2831853f;
	float ro = r + w / 2 + 1;
	int x0 = (int)floorf(cx - ro), x1 = (int)ceilf(cx + ro);
	int y0 = (int)floorf(cy - ro), y1 = (int)ceilf(cy + ro);
	if (!clip(x0, y0, x1, y1)) return;
	for (int y = y0; y < y1; y++)
		for (int x = x0; x < x1; x++) {
			float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
			float cov = w / 2 - fabsf(sqrtf(dx * dx + dy * dy) - r) + 0.5f;
			if (cov <= 0) continue;
			float t = atan2f(dy, dx);
			while (t < a0) t += twoPi;
			if (t > a1) continue;
			plot(x, y, c, cov >= 1 ? alpha : (int)(cov * alpha));
		}
	// zaokrąglone końce
	circle(cx + r * cosf(a0), cy + r * sinf(a0), w / 2, c, alpha);
	circle(cx + r * cosf(a1), cy + r * sinf(a1), w / 2, c, alpha);
}

// --- tekst -------------------------------------------------------------------

static const Glyph* glyph(const Font& f, char ch)
{
	if (ch < 32 || ch > 126) ch = '?';
	return &f.glyphs[ch - 32];
}

int textWidth(const char* s, const Font& f)
{
	int w = 0;
	for (; *s; s++) w += glyph(f, *s)->adv;
	return w;
}

int text(int x, int y, const char* s, const Font& f, Color c)
{
	int start = x;
	for (; *s; s++) {
		const Glyph* g = glyph(f, *s);
		int gx = x + g->x, gy = y + g->y;
		int x0 = gx, y0 = gy, x1 = gx + g->w, y1 = gy + g->h;
		if (g->w && clip(x0, y0, x1, y1)) {
			int bpr = (g->w + 1) / 2;
			const uint8_t* bits = f.bits + g->offset;
			for (int row = 0; row < g->h; row++) {
				int py = gy + row;
				if (py < 0 || py >= H) continue;
				for (int col = 0; col < g->w; col++) {
					int px = gx + col;
					if (px < 0 || px >= W) continue;
					uint8_t b = bits[row * bpr + col / 2];
					int a = (col & 1) ? (b & 0x0F) : (b >> 4);
					if (a) plot(px, py, c, a * 17);
				}
			}
		}
		x += g->adv;
	}
	return x - start;
}

void textCentered(int x0, int x1, int y, const char* s, const Font& f, Color c)
{
	text(x0 + (x1 - x0 - textWidth(s, f)) / 2, y, s, f, c);
}

void textRight(int x1, int y, const char* s, const Font& f, Color c)
{
	text(x1 - textWidth(s, f), y, s, f, c);
}

// --- obrazy ------------------------------------------------------------------

void image(int x, int y, const uint16_t* px, int w, int h, int div, int dim)
{
	int ow = w / div, oh = h / div;
	int x0 = x, y0 = y, x1 = x + ow, y1 = y + oh;
	if (!clip(x0, y0, x1, y1)) return;
	for (int oy = 0; oy < oh; oy++) {
		int sy = y + oy;
		if (sy < 0 || sy >= H) continue;
		for (int ox = 0; ox < ow; ox++) {
			int sx = x + ox;
			if (sx < 0 || sx >= W) continue;
			int r = 0, g = 0, b = 0;
			for (int dy = 0; dy < div; dy++)
				for (int dx = 0; dx < div; dx++) {
					uint16_t p = px[(oy * div + dy) * w + ox * div + dx];
					r += p >> 11;
					g += (p >> 5) & 0x3F;
					b += p & 0x1F;
				}
			int n = div * div;
			Color c = (Color)(((r / n) << 11) | ((g / n) << 5) | (b / n));
			fb[sy * W + sx] = dim ? blend(c, 0, dim) : c;
		}
	}
}

}   // namespace gfx
