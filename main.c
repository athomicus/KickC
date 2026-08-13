// KickC: sprite + joystick + BITMAP HI-RES + kafelki jako NAZWANE tablice, dowolny rozmiar
//  COMPILE bin/kickc.sh -e my_examples/my_first_bitmap_final.c
//
// POPRAWKA KRYTYCZNA: 6502 nie ma instrukcji mnozenia. KickC umie mnozyc zmienna
// PRZEZ STALA znana w czasie kompilacji (np. bitmapRow*40 - zamienia to na dodawania/
// przesuniecia), ale NIE umie pomnozyc DWOCH ZMIENNYCH runtime przez siebie
// (np. cellRow * widthCells - oba sa zmiennymi w oczach kompilatora, mimo ze Ty
// wywolujesz funkcje z konkretna liczba). Blad: "Runtime multiplication not supported".
// FIX: zamiast mnozenia w petli, uzywamy AKUMULATORA - dodajemy widthCells*8 do licznika
// przy kazdym przejsciu przez rzad, zamiast liczyc to mnozeniem od zera kazdy raz.
// To jest ta sama technika, ktora uzywa sie w assemblerze 6502 do "udawania" mnozenia.
#include <c64.h>
#include "gfx.h"
unsigned char* VIC        = (unsigned char*)0xD000;
unsigned char* SPRITE_PTR = (unsigned char*)0x07F8;
unsigned char* CIA1_PRA   = (unsigned char*)0xDC00;
unsigned char* DDRA       = (unsigned char*)0xDC02;
unsigned char* SCREEN_RAM = (unsigned char*)0x0400; 


////////////////////////////////////////////////////////////////////////

__address(0x1000) char MUSIC[] = kickasm(resource "toiletrensdyr.sid") {{
    .const music = LoadSid("toiletrensdyr.sid")
    .fill music.size, music.getData(i)
}};

typedef void(*PROC_PTR)();

// Pointer to the music init routine
PROC_PTR musicInit = (PROC_PTR) MUSIC;
// Pointer to the music play routine
PROC_PTR musicPlay = (PROC_PTR) MUSIC+3;
/////////////////////////////////////////

//__export __address(0x2000) char BITMAP[8000];

word spriteXArray[8];
byte spriteYArray[8];



void updateEnemyAI() {
    if (spriteXArray[2] < spriteXArray[0]) spriteXArray[2] += 1;
    if (spriteXArray[2] > spriteXArray[0]) spriteXArray[2] -= 1;
    if (spriteYArray[2] < spriteYArray[0]) spriteYArray[2] += 1;
    if (spriteYArray[2] > spriteYArray[0]) spriteYArray[2] -= 1;
}

void main() {

     
    (*musicInit)(); // Initialize the music


    SPRITE_PTR[0] = 0x3F40 / 64;
    SPRITE_PTR[1] = 0x3F80 / 64;
    SPRITE_PTR[2] = 0x3FC0 / 64;

    VIC[0x15] = 0x01 | 0x02 | 0x04;

    spriteXArray[0] = 150;
    spriteYArray[0] = 100;
    VIC[0x27] = 1;

    spriteXArray[1] = 80;
    spriteYArray[1] = 60;
    VIC[0x28] = 2;

    spriteXArray[2] = 200;
    spriteYArray[2] = 60;
    VIC[0x29] = 11;

    VIC[0x20] = 0x00;
    VIC[0x21] = 0x00;

    VIC[0x11] = 0x3B;
    VIC[0x16] = 0x08;
    VIC[0x18] = 0x18;

    drawBackground();

    while (1) {
        while (VIC[0x12] != 250) {}
        while (VIC[0x12] == 250) {}

        *DDRA = 224;
        unsigned char joy = *CIA1_PRA;
        if (!(joy & 0x01)) spriteYArray[0] -= 2;
        if (!(joy & 0x02)) spriteYArray[0] += 2;
        if (!(joy & 0x04)) spriteXArray[0] -= 2;
        if (!(joy & 0x08)) spriteXArray[0] += 2;
        *DDRA = 255;

        updateEnemyAI();
        updateAllSpritesXY();
        (*musicPlay)(); //Play the music 
    }
}




// // KickC: sprite + joystick + BITMAP HI-RES + blit kafelkow 16x16
// //  COMPILE bin/kickc.sh -e my_examples/my_first_bitmap_blit_v5.c
// //
// // POPRAWKA v5: naprawiono przepelnienie bajtu w obliczeniu cellBase.
// // PRZED: word cellBase = (word)(bitmapRow * 40 + bitmapCol) * 8;
// //   -> "bitmapRow * 40 + bitmapCol" liczone jako BYTE (bo oba skladniki sa byte),
// //      zawija sie modulo 256 ZANIM (word) zdazy zadzialac. Np. dla row=20,col=2
// //      powinno wyjsc 6416, a wychodzilo 272 - dane trafialy w zle miejsce bitmapy.
// // PO: rzutujemy PIERWSZY skladnik na word, wiec cale wyrazenie liczy sie w 16 bit.

// unsigned char* VIC        = (unsigned char*)0xD000;
// unsigned char* SPRITE_PTR = (unsigned char*)0x07F8;
// unsigned char* CIA1_PRA   = (unsigned char*)0xDC00;
// unsigned char* DDRA       = (unsigned char*)0xDC02;
// unsigned char* SCREEN_RAM = (unsigned char*)0x0400;

// __export __address(0x2000) char BITMAP[8000];

// word spriteXArray[8];
// byte spriteYArray[8];

// void updateAllSpritesXY() {
//     byte msb = 0;
//     for (byte t = 0; t < 8; t++) {
//         word x = spriteXArray[t];
//         if (x < 24)  x = 24;
//         if (x > 344) x = 344;
//         spriteXArray[t] = x;
//         VIC[0x00 + t*2] = (byte)(x & 0xFF);
//         VIC[0x01 + t*2] = spriteYArray[t];
//         if (x > 255) msb |= (1 << t);
//     }
//     VIC[0x10] = msb;
// }

// // #define TILE_BRICK 0
// // #define TILE_STONE 1
// // #define TILE_SIZE 32

// // const char TILES[64] = {
// //     0xFF,0xFF, 0x80,0x01, 0x80,0x01, 0x80,0x01,
// //     0x80,0x01, 0x80,0x01, 0x80,0x01, 0xFF,0xFF,
// //     0x80,0x01, 0x80,0x01, 0x80,0x01, 0x80,0x01,
// //     0x80,0x01, 0x80,0x01, 0x80,0x01, 0xFF,0xFF,

// //     0x0F,0xF0, 0x3F,0xFC, 0x78,0x1E, 0xF0,0x0F,
// //     0xE3,0xC7, 0xC7,0xE3, 0xCF,0xF3, 0xDF,0xFB,
// //     0xDF,0xFB, 0xCF,0xF3, 0xC7,0xE3, 0xE3,0xC7,
// //     0xF0,0x0F, 0x78,0x1E, 0x3F,0xFC, 0x0F,0xF0
// // };

// // void blitTile16(word x, byte y, byte tileId, byte color) {
// //     word tileOffset = (word)tileId * TILE_SIZE;
// //     byte baseCol = (byte)(x / 8);
// //     byte baseRow = (byte)(y / 8);

// //     for (byte cellRow = 0; cellRow < 2; cellRow++) {
// //         for (byte cellCol = 0; cellCol < 2; cellCol++) {
// //             byte bitmapRow = baseRow + cellRow;
// //             byte bitmapCol = baseCol + cellCol;

// //             // <-- POPRAWKA: rzutuj bitmapRow na word PRZED mnozeniem,
// //             // zeby cale wyrazenie liczylo sie w 16-bit, nie w 8-bit
// //             word cellBase = ((word)bitmapRow * 40 + bitmapCol) * 8;

// //             for (byte py = 0; py < 8; py++) {
// //                 byte srcByte = TILES[tileOffset + (cellRow*8 + py)*2 + cellCol];
// //                 BITMAP[cellBase + py] = srcByte;
// //             }

// //             word screenIdx = (word)bitmapRow * 40 + bitmapCol;
// //             SCREEN_RAM[screenIdx] = (color << 4) | 0x00;
// //         }
// //     }
// // }

// // void clearBitmap() {
// //     for (word i = 0; i < 8000; i++) {
// //         BITMAP[i] = 0x00;
// //     }
// //     for (word i = 0; i < 1000; i++) {
// //         SCREEN_RAM[i] = 0x00;
// //     }
// // }

// // void drawBackground() {
// //     clearBitmap();
// //     blitTile16(16,  160, TILE_BRICK, 9);
// //     blitTile16(32,  160, TILE_BRICK, 9);
// //     blitTile16(48,  160, TILE_BRICK, 9);
// //     blitTile16(160, 160, TILE_STONE, 12);
// //     blitTile16(176, 160, TILE_STONE, 12);
// //     blitTile16(240, 80,  TILE_BRICK, 9);
// // }

// // __export __address(0x3F40) char SPRITE_DATA[64] = {
// //     0xFE,0x00,0x7F,0x83,0x00,0x81,0x80,0x81,
// //     0x01,0x80,0x42,0x01,0x40,0x24,0x02,0x20,
// //     0x18,0x04,0x10,0x00,0x08,0x08,0x00,0x10,
// //     0x04,0x00,0x20,0x02,0x00,0x40,0x01,0x00,
// //     0x80,0x02,0x00,0x40,0x04,0x00,0x20,0x08,
// //     0x00,0x10,0x10,0x00,0x08,0x20,0x18,0x04,
// //     0x40,0x24,0x02,0x80,0x42,0x01,0x80,0x81,
// //     0x01,0x81,0x00,0x83,0xFE,0x00,0x7F,0x01
// // };

// // __export __address(0x3F80) char SPRITE1_DATA[64] = {
// //     0xFE,0x00,0x7F,0x83,0x00,0x81,0x80,0x81,
// //     0x01,0x80,0x42,0x01,0x40,0x24,0x02,0x20,
// //     0x18,0x04,0x10,0x00,0x08,0x08,0x00,0x10,
// //     0x04,0x00,0x20,0x02,0x00,0x40,0x01,0x00,
// //     0x80,0x02,0x00,0x40,0x04,0x00,0x20,0x08,
// //     0x00,0x10,0x10,0x00,0x08,0x20,0x18,0x04,
// //     0x40,0x24,0x02,0x80,0x42,0x01,0x80,0x81,
// //     0x01,0x81,0x00,0x83,0xFE,0x00,0x7F,0x01
// // };

// // __export __address(0x3FC0) char SPRITE2_DATA[64] = {
// //     0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xC0,
// //     0x00,0x07,0xE0,0x00,0x0F,0xF0,0x00,0x1D,
// //     0xB8,0x00,0x1F,0xF8,0x00,0x12,0x48,0x00,
// //     0x1E,0x78,0x00,0x13,0xC8,0x00,0x1F,0xFC,
// //     0x14,0x1F,0xFC,0x14,0x1F,0xFE,0x1C,0x1F,
// //     0xFF,0x3C,0x1F,0xFF,0xFC,0x0B,0xAF,0xF8,
// //     0x0B,0xAF,0xF0,0x18,0x30,0x00,0x00,0x00,
// //     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
// // };

// // void updateEnemyAI() {
// //     if (spriteXArray[2] < spriteXArray[0]) spriteXArray[2] += 1;
// //     if (spriteXArray[2] > spriteXArray[0]) spriteXArray[2] -= 1;
// //     if (spriteYArray[2] < spriteYArray[0]) spriteYArray[2] += 1;
// //     if (spriteYArray[2] > spriteYArray[0]) spriteYArray[2] -= 1;
// // }

// // void main() {
// //     SPRITE_PTR[0] = 0x3F40 / 64;
// //     SPRITE_PTR[1] = 0x3F80 / 64;
// //     SPRITE_PTR[2] = 0x3FC0 / 64;

// //     VIC[0x15] = 0x01 | 0x02 | 0x04;

// //     spriteXArray[0] = 150;
// //     spriteYArray[0] = 100;
// //     VIC[0x27] = 1;

// //     spriteXArray[1] = 80;
// //     spriteYArray[1] = 60;
// //     VIC[0x28] = 2;

// //     spriteXArray[2] = 200;
// //     spriteYArray[2] = 60;
// //     VIC[0x29] = 11;

// //     VIC[0x20] = 0x00;
// //     VIC[0x21] = 0x00;

// //     VIC[0x11] = 0x3B;
// //     VIC[0x16] = 0x08;
// //     VIC[0x18] = 0x18;

// //     drawBackground();

// //     while (1) {
// //         while (VIC[0x12] != 250) {}
// //         while (VIC[0x12] == 250) {}

// //         *DDRA = 224;
// //         unsigned char joy = *CIA1_PRA;
// //         if (!(joy & 0x01)) spriteYArray[0] -= 2;
// //         if (!(joy & 0x02)) spriteYArray[0] += 2;
// //         if (!(joy & 0x04)) spriteXArray[0] -= 2;
// //         if (!(joy & 0x08)) spriteXArray[0] += 2;
// //         *DDRA = 255;

// //         updateEnemyAI();
// //         updateAllSpritesXY();
// //     }
// // }