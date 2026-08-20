// KickC: sprite (8x) + joystick + CUSTOM CHARSET (8x8 hires) + MAPA jako plaska tablica
//  COMPILE bin/kickc.sh -e my_examples/my_charset_base.c
//
// BAZA DO DALSZEJ GRY - bitmapa hi-res zostala calkowicie usunieta.
// Zamiast rysowac piksel po pikselu (BITMAP[8000], 8000 bajtow RAM), uzywamy
// TRYBU TEKSTOWEGO z WLASNYM ZESTAWEM ZNAKOW (charset). Kazdy "kafelek" na mapie
// to tylko JEDEN BAJT (numer znaku) wpisany do SCREEN_RAM - VIC-II sam narysuje
// odpowiedni wzor 8x8 z charsetu. To duzo mniej RAM-u i szybsze rysowanie.
//
// UKLAD PAMIECI (bank VIC-II 0, adresy 0x0000-0x3FFF):
//   0x0400-0x07E7  SCREEN_RAM   (1000 bajtow, mapa znakow 40x25)
//   0x07F8-0x07FF  SPRITE_PTR   (8 bajtow, wskazniki duszkow - koniec bloku SCREEN_RAM)
//   0x1000         MUSIC        (plik SID)
//   0x3000-0x37FF  CHARSET      (2048 bajtow, 256 znakow x 8 bajtow, wlasny zestaw)
//   0x3E00-0x3FFF  SPRITE0..7   (8 duszkow x 64 bajty = 512 bajtow, ostatni obszar banku)
//
// POPRAWKA KRYTYCZNA #1 (mnozenie): 6502 nie ma instrukcji mnozenia. KickC umie
// mnozyc zmienna PRZEZ STALA znana w czasie kompilacji (np. row*40 - zamienia to na
// dodawania/przesuniecia), ale NIE umie pomnozyc DWOCH ZMIENNYCH runtime przez siebie.
// FIX: rzutujemy pierwszy skladnik na word PRZED mnozeniem/dodawaniem, zeby cale
// wyrazenie liczylo sie w 16 bit, a nie w 8 bit (inaczej zawija sie modulo 256).
//
// POPRAWKA KRYTYCZNA #2 (mapa jako tablica 2D): KickC NIE generuje kodu dla
// podwojnego indeksowania zmiennymi runtime, np. map[row][col] gdy oba "row" i "col"
// sa zmiennymi w petli - kompilator zglasza blad "Missing ASM fragment"
// (vbuz1=(qbuc1_derefidx_vbuz2)_derefidx_vbuz3). FIX: mapa jest PLASKA tablica 1D
// (map[MAP_WIDTH*MAP_HEIGHT]), a dostep do kafelka liczymy recznie jako
// map[(word)row * MAP_WIDTH + col] - to JEDNO indeksowanie wynikiem wyrazenia,
// a nie dwa zagniezdzone, wiec KickC ma dla tego gotowy fragment ASM.


#include <c64.h>


unsigned char* VIC        = (unsigned char*)0xD000;


// SPRITE_PTR to adres startowy 0x07F8 (2040) w pamieci VIC-II, gdzie leza wskazniki
// do danych duszkow. Kazdy wskaznik zajmuje 1 bajt i wskazuje NIE na adres, ale na
// NUMER 64-BAJTOWEGO BLOKU (adres/64). Adresy danych sprite'a musza byc podzielne
// przez 64. SPRITE_PTR[0] to duszek 0, SPRITE_PTR[1] duszek 1, ..., SPRITE_PTR[7] duszek 7.
unsigned char* SPRITE_PTR = (unsigned char*)0x07F8;
unsigned char* CIA1_PRA   = (unsigned char*)0xDC00;
unsigned char* DDRA       = (unsigned char*)0xDC02;
unsigned char* SCREEN_RAM = (unsigned char*)0x0400;
unsigned char* COLOR_RAM  = (unsigned char*)0xD800;


////////////////////////////////////////////////////////////////////////////////
// MUZYKA SID (bez zmian wzgledem oryginalu)
////////////////////////////////////////////////////////////////////////////////


__address(0x1000) char MUSIC[] = kickasm(resource "toiletrensdyr.sid") {{
    .const music = LoadSid("toiletrensdyr.sid")
    .fill music.size, music.getData(i)
}};


typedef void(*PROC_PTR)();


PROC_PTR musicInit = (PROC_PTR) MUSIC;
PROC_PTR musicPlay = (PROC_PTR) MUSIC+3;


////////////////////////////////////////////////////////////////////////////////
// CUSTOM CHARSET - 8x8 hires, wlasny zestaw znakow zamiast bitmapy
// Adres 0x3000 jest podzielny przez 0x0800 (wymog sprzetowy VIC-II).
// Kazdy znak to 8 bajtow (jeden bajt na wiersz, bit=1 to zapalony piksel).
//
// NAZWANE STALE - to Ty decydujesz co znaczy kazdy numer znaku. Trzymaj sie
// jednej numeracji od poczatku projektu, nowe typy dopisuj na koniec.
////////////////////////////////////////////////////////////////////////////////


#define CHAR_EMPTY   0   // puste tlo
#define CHAR_WALL    1   // sciana / blok
#define CHAR_COIN    2   // moneta / zbieralny obiekt
#define CHAR_GRASS   3   // trawa / podloze

 
__export __address(0x3000) char charset[2048] = {
  0x00, 0x3f, 0x7f, 0x78, 0x70, 0x60, 0x60, 0x72, // char 0
  0x00, 0xff, 0xff, 0x71, 0x20, 0x00, 0x00, 0x00, // char 1
  0x78, 0x70, 0x60, 0x60, 0x60, 0x60, 0x62, 0x60, // char 2
  0x01, 0x00, 0x60, 0x00, 0x00, 0x00, 0x08, 0x00, // char 3
  0x00, 0xff, 0xff, 0x0e, 0x04, 0x00, 0x00, 0x40, // char 4
  0x00, 0xff, 0xff, 0x3c, 0x1c, 0x08, 0x00, 0x00, // char 5
  0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x41, 0x00, // char 6
  0x20, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x10, // char 7
  0x00, 0xff, 0xff, 0x8e, 0x04, 0x00, 0x00, 0x00, // char 8
  0x00, 0xfc, 0xfe, 0x1e, 0x0e, 0x06, 0x06, 0x4e, // char 9
  0x80, 0x00, 0x06, 0x00, 0x00, 0x00, 0x10, 0x00, // char 10
  0x1e, 0x0e, 0x06, 0x06, 0x06, 0x06, 0x46, 0x06, // char 11
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // char 12
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // char 13
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // char 14
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // char 15
};



////////////////////////////////////////////////////////////////////////////////
// MAPA - PLASKA tablica 1D (nie tablica tablic!). Kazdy bajt to numer znaku
// z charset. Indeks liczymy recznie: map[(word)row * MAP_WIDTH + col].
// Rozmiar MAP_WIDTH x MAP_HEIGHT dowolny, u nas cala szerokosc ekranu (40x25)
// jako przyklad startowy - jesli chcesz mapy mniejsze niz ekran, zmien te stale
// i wygeneruj mape edytorem HTML z nowymi wymiarami.
////////////////////////////////////////////////////////////////////////////////


#define MAP_WIDTH  40   // liczba kolumn
#define MAP_HEIGHT 25   // liczba wierszy

 

const unsigned char map[MAP_WIDTH * MAP_HEIGHT] = {
  0, 1, 4, 5, 8, 8, 4, 4, 5, 4, 4, 5, 8, 4, 5, 4, 5, 4, 4, 5, 4, 5, 4, 5, 4, 4, 8, 4, 5, 4, 5, 8, 4, 5, 4, 5, 4, 5, 4, 9,
  2, 3, 6, 7, 7, 7, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 6, 13, 6, 6, 10, 11,
  2, 7, 13, 13, 6, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 6, 6, 6, 6, 11,
  2, 13, 13, 13, 13, 6, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 6, 13, 6, 6, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 11,
  2, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 11,
};

// Odczyt kafelka: map[(word)row * MAP_WIDTH + col]


// Odczytuje numer znaku z mapy w danej kolumnie/wierszu mapy (nie ekranu!).
// map jest PLASKA (1D), wiec liczymy offset recznie. Rzutowanie "row" na (word)
// PRZED mnozeniem jest konieczne - inaczej mnozenie 8-bit * 8-bit zawinie sie
// modulo 256 dla wiekszych map (patrz komentarz na gorze pliku).
byte mapTileAt(byte col, byte row) {
    return map[(word)row * MAP_WIDTH + col];
}


// Kopiuje mape do SCREEN_RAM zaczynajac od pozycji (offsetCol, offsetRow) na
// ekranie (40x25). Dzieki temu mapa moze byc mniejsza niz caly ekran i wyswietlona
// w dowolnym miejscu, bez przepisywania kodu.
//
// UWAGA NA ROZMIAR: offsetCol + MAP_WIDTH musi byc <= 40, a offsetRow + MAP_HEIGHT
// musi byc <= 25, inaczej screenCol/screenRow wyjdzie poza SCREEN_RAM/COLOR_RAM
// (1000 bajtow, indeksy 0-999) i nadpisze pamiec poza ekranem. Przy mapie 40x25
// (cały ekran) jedyny bezpieczny offset to (0, 0).
void drawMap(byte offsetCol, byte offsetRow, byte color) {
    for (byte row = 0; row < MAP_HEIGHT; row++) {
        for (byte col = 0; col < MAP_WIDTH; col++) {
            byte screenCol = offsetCol + col;
            byte screenRow = offsetRow + row;
            // rzutowanie na word PRZED mnozeniem - identyczna zasada jak wyzej
            word screenIdx = (word)screenRow * 40 + screenCol;
            byte tile = map[(word)row * MAP_WIDTH + col];

            SCREEN_RAM[screenIdx] = tile;
            COLOR_RAM[screenIdx]  = color;
        }
    }
}


void clearScreen() {
    for (word i = 0; i < 1000; i++) {
        SCREEN_RAM[i] = CHAR_EMPTY;
        COLOR_RAM[i]  = 0;
    }
}


////////////////////////////////////////////////////////////////////////////////
// SPRITE'Y - 8 slotow gotowych do wypelnienia. Ponizej TYLKO sprite 0 (gracz)
// ma realny wzor graficzny - sloty 1..7 sa zarezerwowane (puste dane), gotowe
// do wypelnienia gdy dodasz wrogow/pociski/etc. Kazdy adres jest wielokrotnoscia
// 64 i wszystkie razem (8 x 64 = 512 bajtow) miesca sie w koncu banku VIC (0x3E00-0x3FFF).
////////////////////////////////////////////////////////////////////////////////


__export __address(0x3E00) char SPRITE0_DATA[64] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x3C, 0x00,
    0x00, 0x3C, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};


__export __address(0x3E40) char SPRITE1_DATA[64] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
__export __address(0x3E80) char SPRITE2_DATA[64] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
__export __address(0x3EC0) char SPRITE3_DATA[64] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
__export __address(0x3F00) char SPRITE4_DATA[64] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
__export __address(0x3F40) char SPRITE5_DATA[64] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
__export __address(0x3F80) char SPRITE6_DATA[64] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
__export __address(0x3FC0) char SPRITE7_DATA[64] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


word spriteXArray[8];
byte spriteYArray[8];


// Przepisuje pozycje z tablic spriteXArray/spriteYArray do rejestrow VIC-II.
// Ogranicza X do widocznego zakresu (24-344) i ustawia bit MSB dla X > 255.
void updateAllSpritesXY() {
    byte msb = 0;
    for (byte t = 0; t < 8; t++) {
        word x = spriteXArray[t];
        if (x < 24)  x = 24;
        if (x > 344) x = 344;
        spriteXArray[t] = x;
        VIC[0x00 + t*2] = (byte)(x & 0xFF);
        VIC[0x01 + t*2] = spriteYArray[t];
        if (x > 255) msb |= (1 << t);
    }
    VIC[0x10] = msb;
}


////////////////////////////////////////////////////////////////////////////////
// GLOWNA PETLA GRY
////////////////////////////////////////////////////////////////////////////////


void main() {


    (*musicInit)(); // Initialize the music


    // Wskazniki sprite'ow - kazdy adres/64. Wszystkie 8 slotow ustawione,
    // nawet te puste (1..7), zeby VIC-II mial poprawne dane od startu.
    SPRITE_PTR[0] = 0x3E00 / 64;
    SPRITE_PTR[1] = 0x3E40 / 64;
    SPRITE_PTR[2] = 0x3E80 / 64;
    SPRITE_PTR[3] = 0x3EC0 / 64;
    SPRITE_PTR[4] = 0x3F00 / 64;
    SPRITE_PTR[5] = 0x3F40 / 64;
    SPRITE_PTR[6] = 0x3F80 / 64;
    SPRITE_PTR[7] = 0x3FC0 / 64;


    // Wlacz tylko sprite 0 (gracz) na start. Odkomentuj kolejne bity gdy
    // dorysujesz wzory dla sprite 1..7, np. VIC[0x15] = 0x01 | 0x02;
    VIC[0x15] = 0x01;


    spriteXArray[0] = 100;
    spriteYArray[0] = 100;
    VIC[0x27] = 1; // kolor sprite 0


    VIC[0x20] = 0x00; // kolor obramowania
    VIC[0x21] = 0x00; // kolor tla


    // WAZNE: BMM (bit 5, wartosc 0x20) jest WYLACZONY - tryb tekstowy, nie bitmapowy.
    VIC[0x11] = 0x1B;
    // MCM (bit 4, wartosc 0x10) WYLACZONY - hires, nie multicolor. Wlacz go (|0x10)
    // gdy przejdziesz na charset multicolor.
    VIC[0x16] = 0x08;
    // D018: gorny nibble = SCREEN_RAM/0x400, dolny nibble = CHARSET/0x800 (przesuniete o 1 bit)
    // screen=0x0400 -> nibble 1 ; charset=0x3000 -> nibble 6 -> (1<<4)|(6<<1) = 0x1C
    VIC[0x18] = 0x1C;


    clearScreen();
    drawMap(0, 0, 14); // rysuj mape od (0,0) - mapa 40x25 wypelnia caly ekran


    while (1) {
        while (VIC[0x12] != 250) {}
        while (VIC[0x12] == 250) {}


        *DDRA = 224; // wylacz klawiature na czas odczytu joysticka
        unsigned char joy = *CIA1_PRA;
        if (!(joy & 0x01)) spriteYArray[0] -= 2; // gora
        if (!(joy & 0x02)) spriteYArray[0] += 2; // dol
        if (!(joy & 0x04)) spriteXArray[0] -= 2; // lewo
        if (!(joy & 0x08)) spriteXArray[0] += 2; // prawo
        *DDRA = 255;


        updateAllSpritesXY();
        (*musicPlay)(); // Play the music
    }
}
