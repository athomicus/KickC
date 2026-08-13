// gfx.h - deklaracje danych i funkcji graficznych (sprite'y, kafelki, bitmapa).
// Tylko extern/prototypy - zero danych, zero __address.
#ifndef GFX_H
#define GFX_H

// ===== SPRITE'Y - surowe dane pikselowe =====
extern char SPRITE_DATA[64];
extern char SPRITE1_DATA[64];
extern char SPRITE2_DATA[64];

// ===== KAFELKI - surowe dane pikselowe, bez wlasnych kolorow =====
extern const unsigned char tile[32];   // 2x2 komorki (16x16px) = 32 bajty
extern const unsigned char grzyb[128]; // 4x4 komorki (32x32px) = 128 bajtow

// ===== BITMAPA HI-RES =====
extern char BITMAP[8000];

// Rysuje kafelek o zadanym rozmiarze (widthCells x heightCells komorek 8x8px),
// jednym kolorem dla calosci.
void draw(const char* tileData, byte widthCells, byte heightCells, word x, byte y, byte color);

void clearBitmap();
void drawBackground();
void updateAllSpritesXY();
#endif
