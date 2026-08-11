// KickC: sprite wczytywany z obrazka PNG + poruszanie joystickiem (port 2)
// Wymaga pliku "sprite.png" w tym samym folderze co ten plik .c
// sprite.png musi byc czarno-bialy, rozmiar 24x21 pikseli (standard sprite C64)
//  COMPILE bin/kickc.sh -e my_examples/my_first.c
unsigned char* VIC        = (unsigned char*)0xD000; // rejestry VIC-II
unsigned char* SPRITE_PTR = (unsigned char*)0x07F8;  // wskaznik sprite 0
unsigned char* CIA1_PRA   = (unsigned char*)0xDC00;  // joystick port 2
unsigned char* DDRA = (unsigned char*)0xDC02;  // wylaczmy klawiature na czas czytania joysticka (port 2)
unsigned char* SCREEN_RAM = (unsigned char*)0x0400; // poczatek pamieci ekranu
unsigned int spriteX = 150;
//#define VIC ((byte*)0xD000)

word spriteXArray[8];   // 0-511, więc word (unsigned int), nie byte

// Funkcja aktualizuje pozycje X wszystkich sprite'ów (0-7) w rejestrach VIC 
// normalnie w VIC mamy char (byte) dla X, ale jeśli sprite ma X>255, to trzeba ustawić odpowiedni bit w rejestrze $D010 (VIC[0x10])
// więc dla każdego sprite'a sprawdzamy czy jego X>255 i ustawiamy odpowiedni bit w VIC[0x10]
// nasz ekran ma szerokość 320 pikseli, więc X sprite'a może być w zakresie 0-319, ale w VIC możemy ustawić X=0-511 (bo mamy dodatkowy bit w $D010)
void updateAllSpritesX() {
    byte msb = 0;
    for (byte t = 0; t < 8; t++) {
        word x = spriteXArray[t];   // word, nie int

        if (x < 24)  x = 24;
        if (x > 344) x = 344;
        spriteXArray[t] = x;

        VIC[0x00 + t*2] = (byte)(x & 0xFF);
        if (x > 255) msb |= (1 << t);
    }
    VIC[0x10] = msb;
}




// Dane sprite'a wczytane z obrazka PNG (czarny = 0, bialy = 1)
// __address(0x2000) char SPRITE_DATA[63] = kickasm(resource "sprite.png") {{
//     .var pic = LoadPicture("sprite.png", List().add($000000, $ffffff))
//     .for (var y=0; y<21; y++)
//         .for (var x=0; x<3; x++)
//             .byte pic.getSinglecolorByte(x,y)
// }};



__export __address(0x2000) char SPRITE_DATA[64] = {
    0xFE,0x00,0x7F,0x83,0x00,0x81,0x80,0x81,
    0x01,0x80,0x42,0x01,0x40,0x24,0x02,0x20,
    0x18,0x04,0x10,0x00,0x08,0x08,0x00,0x10,
    0x04,0x00,0x20,0x02,0x00,0x40,0x01,0x00,
    0x80,0x02,0x00,0x40,0x04,0x00,0x20,0x08,
    0x00,0x10,0x10,0x00,0x08,0x20,0x18,0x04,
    0x40,0x24,0x02,0x80,0x42,0x01,0x80,0x81,
    0x01,0x81,0x00,0x83,0xFE,0x00,0x7F,0x01
};


// Drugi sprite - własny blok pamięci, adres musi być podzielny przez 64
__export __address(0x2040) char SPRITE1_DATA[64] = {
    0xFE,0x00,0x7F,0x83,0x00,0x81,0x80,0x81,
    0x01,0x80,0x42,0x01,0x40,0x24,0x02,0x20,
    0x18,0x04,0x10,0x00,0x08,0x08,0x00,0x10,
    0x04,0x00,0x20,0x02,0x00,0x40,0x01,0x00,
    0x80,0x02,0x00,0x40,0x04,0x00,0x20,0x08,
    0x00,0x10,0x10,0x00,0x08,0x20,0x18,0x04,
    0x40,0x24,0x02,0x80,0x42,0x01,0x80,0x81,
    0x01,0x81,0x00,0x83,0xFE,0x00,0x7F,0x01
};

__export __address(0x2080) char SPRITE2_DATA[64] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xC0,
    0x00,0x07,0xE0,0x00,0x0F,0xF0,0x00,0x1D,
    0xB8,0x00,0x1F,0xF8,0x00,0x12,0x48,0x00,
    0x1E,0x78,0x00,0x13,0xC8,0x00,0x1F,0xFC,
    0x14,0x1F,0xFC,0x14,0x1F,0xFE,0x1C,0x1F,
    0xFF,0x3C,0x1F,0xFF,0xFC,0x0B,0xAF,0xF8,
    0x0B,0xAF,0xF0,0x18,0x30,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

void main() {
    // Wskaznik sprite'a = adres_danych / 64
    SPRITE_PTR[0] = 0x2000 / 64; // = 128
    SPRITE_PTR[1]= 0x2040 / 64; // kolejny sprite: do 0x2040 + 0x40( 64 dziesietnie)
    SPRITE_PTR[2]= 0x2080 / 64;  
   
    //2^0 = bit 1- maska Sprite 1       2^1 = bit 2  2^2 = bit 4
    VIC[0x15] = 0x01 | 0x02 | 0x04; 
   
    //Sprite:0	pozX: VIC[0x00]	  pozY:  VIC[0x01]
    spriteXArray[0] = 150; //VIC[0x00] = 150;    // X sprite 0
    VIC[0x01] = 100;    // Y sprite 0
    VIC[0x27] = 1;      // kolor sprite 0 (bialy)


    spriteXArray[1] = 80;     // X sprite 1
    VIC[0x03] = 60;     // Y sprite 1
    VIC[0x28] = 2;      // kolor sprite 1 (czerwony)

    spriteXArray[2] = 200;     // X sprite 2
    VIC[0x05] = 60;     // Y sprite 2
    VIC[0x29] = 3;      // kolor sprite 2 (zielony)

  VIC[0x20] = 0x00; // $D020 - ramka na czarno
 VIC[0x21] = 0x00; // $D020- tlo na niebiesko (przyklad, dowolny kolor)
 for (unsigned int i = 0; i < 1000; i++) {
        SCREEN_RAM[i] = 0x20; // wypelnij caly ekran spacjami
    }

  // Odczekaj ~2 sekunde (100 ramek przy PAL), zeby uzytkownik zobaczyl sprite'y
    // zanim program zacznie odczytywac joystick
    // for (word frame = 0; frame < 400; frame++) {
    //     while (VIC[0x12] != 250) {}
    //     while (VIC[0x12] == 250) {}
    // }

    while (1) {
   // czekaj na koniec ekranu (linia 250), synchronizacja z 1 ramka = 1 krok
        while (VIC[0x12] != 250) {} //50fps
        while (VIC[0x12] == 250) {}

        *DDRA = 224;               // POKE 56322,224 - wylacz skanowanie klawiatury
        unsigned char joy = *CIA1_PRA;
        // Bity joysticka aktywne w stanie niskim (0 = wcisniete)
        if (!(joy & 0x01)) VIC[0x01]-=2; // gora
        if (!(joy & 0x02)) VIC[0x01]+=2; // dol
        if (!(joy & 0x04)) spriteXArray[0] -= 2; // lewo
        if (!(joy & 0x08)) spriteXArray[0] +=2; // prawo
        *DDRA = 255;               // POKE 56322,255 - wlacz skanowanie klawiatury

        updateAllSpritesX();  // jedno wywołanie aktualizuje X + MSB dla wszystkich

        // // ogranicz zakres, zeby nie zawijalo sie w nieskonczonosc
        // if (spriteX < 24)  spriteX = 24;
        // if (spriteX > 344) spriteX = 344;
        //   // rozbij spriteX na mlodszy bajt + 9. bit (MSB)
        // VIC[0x00] = spriteX & 0xFF;
        // if (spriteX > 255)
        //     VIC[0x10] |= 0x01;  // ustaw bit 0 w $D010 (MSB sprite 0)
        // else
        //     VIC[0x10] &= ~0x01; // wyczysc bit 0 w $D010

    }
}