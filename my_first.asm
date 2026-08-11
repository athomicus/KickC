// KickC: sprite wczytywany z obrazka PNG + poruszanie joystickiem (port 2)
// Wymaga pliku "sprite.png" w tym samym folderze co ten plik .c
// sprite.png musi byc czarno-bialy, rozmiar 24x21 pikseli (standard sprite C64)
//  COMPILE bin/kickc.sh -e my_examples/my_first.c
  // Commodore 64 PRG executable file
.file [name="my_first.prg", type="prg", segments="Program"]
.segmentdef Program [segments="Basic, Code, Data"]
.segmentdef Basic [start=$0801]
.segmentdef Code [start=$80d]
.segmentdef Data [startAfter="Code"]
.segment Basic
:BasicUpstart(main)
  .const SIZEOF_UNSIGNED_INT = 2
  .label VIC = $d000
  // rejestry VIC-II
  .label SPRITE_PTR = $7f8
  // wskaznik sprite 0
  .label CIA1_PRA = $dc00
  // joystick port 2
  .label DDRA = $dc02
  // wylaczmy klawiature na czas czytania joysticka (port 2)
  .label SCREEN_RAM = $400
.segment Code
main: {
    .label i = 5
    .label __26 = 2
    // Wskaznik sprite'a = adres_danych / 64
    lda #$2000/$40
    sta SPRITE_PTR
    // = 128
    lda #$2040/$40
    sta SPRITE_PTR+1
    // kolejny sprite: do 0x2040 + 0x40( 64 dziesietnie)
    lda #$2080/$40
    sta SPRITE_PTR+2
    //2^0 = bit 1- maska Sprite 1       2^1 = bit 2  2^2 = bit 4
    lda #1|2|4
    sta VIC+$15
    //Sprite:0	pozX: VIC[0x00]	  pozY:  VIC[0x01]
    lda #<$96
    sta spriteXArray
    lda #>$96
    sta spriteXArray+1
    //VIC[0x00] = 150;    // X sprite 0
    lda #$64
    sta VIC+1
    // Y sprite 0
    lda #1
    sta VIC+$27
    // kolor sprite 0 (bialy)
    lda #<$50
    sta spriteXArray+1*SIZEOF_UNSIGNED_INT
    lda #>$50
    sta spriteXArray+1*SIZEOF_UNSIGNED_INT+1
    // X sprite 1
    lda #$3c
    sta VIC+3
    // Y sprite 1
    lda #2
    sta VIC+$28
    // kolor sprite 1 (czerwony)
    lda #<$c8
    sta spriteXArray+2*SIZEOF_UNSIGNED_INT
    lda #>$c8
    sta spriteXArray+2*SIZEOF_UNSIGNED_INT+1
    // X sprite 2
    lda #$3c
    sta VIC+5
    // Y sprite 2
    lda #3
    sta VIC+$29
    // kolor sprite 2 (zielony)
    lda #0
    sta VIC+$20
    // $D020 - ramka na czarno
    sta VIC+$21
    sta.z i
    sta.z i+1
  // $D020- tlo na niebiesko (przyklad, dowolny kolor)
  __b1:
    lda.z i+1
    cmp #>$3e8
    bcc __b2
    bne !+
    lda.z i
    cmp #<$3e8
    bcc __b2
  !:
  // czekaj na koniec ekranu (linia 250), synchronizacja z 1 ramka = 1 krok
  __b3:
    lda #$fa
    cmp VIC+$12
    bne __b3
  //50fps
  __b4:
    lda #$fa
    cmp VIC+$12
    beq __b4
    lda #$e0
    sta DDRA
    // POKE 56322,224 - wylacz skanowanie klawiatury
    ldx CIA1_PRA
    txa
    and #1
    cmp #0
    bne __b7
    dec VIC+1
    dec VIC+1
  __b7:
    txa
    and #2
    cmp #0
    bne __b8
    lda VIC+1
    clc
    adc #2
    sta VIC+1
  __b8:
    txa
    and #4
    cmp #0
    bne __b9
    sec
    lda spriteXArray
    sbc #2
    sta spriteXArray
    lda spriteXArray+1
    sbc #0
    sta spriteXArray+1
  __b9:
    txa
    and #8
    cmp #0
    bne __b10
    lda #2
    clc
    adc spriteXArray
    sta spriteXArray
    bcc !+
    inc spriteXArray+1
  !:
  __b10:
    // prawo
    lda #$ff
    sta DDRA
  // POKE 56322,255 - wlacz skanowanie klawiatury
    jsr updateAllSpritesX
    jmp __b3
  __b2:
    lda.z i
    clc
    adc #<SCREEN_RAM
    sta.z __26
    lda.z i+1
    adc #>SCREEN_RAM
    sta.z __26+1
    lda #$20
    ldy #0
    sta (__26),y
    inc.z i
    bne !+
    inc.z i+1
  !:
    jmp __b1
}
// Funkcja aktualizuje pozycje X wszystkich sprite'ów (0-7) w rejestrach VIC 
// normalnie w VIC mamy char (byte) dla X, ale jeśli sprite ma X>255, to trzeba ustawić odpowiedni bit w rejestrze $D010 (VIC[0x10])
// więc dla każdego sprite'a sprawdzamy czy jego X>255 i ustawiamy odpowiedni bit w VIC[0x10]
// nasz ekran ma szerokość 320 pikseli, więc X sprite'a może być w zakresie 0-319, ale w VIC możemy ustawić X=0-511 (bo mamy dodatkowy bit w $D010)
updateAllSpritesX: {
    .label x = 2
    .label t = 4
    ldx #0
    txa
    sta.z t
  __b1:
    lda.z t
    cmp #8
    bcc __b2
    stx VIC+$10
    rts
  __b2:
    lda.z t
    asl
    tay
    lda spriteXArray,y
    sta.z x
    lda spriteXArray+1,y
    sta.z x+1
    bne __b4
    lda.z x
    cmp #$18
    bcs __b4
  !:
    lda #<$18
    sta.z x
    lda #>$18
    sta.z x+1
  __b4:
    lda.z x+1
    cmp #>$158
    bne !+
    lda.z x
    cmp #<$158
  !:
    bcc __b5
    beq __b5
    lda #<$158
    sta.z x
    lda #>$158
    sta.z x+1
  __b5:
    lda.z x
    sta spriteXArray,y
    lda.z x+1
    sta spriteXArray+1,y
    lda #$ff
    and.z x
    sta VIC,y
    lda #$ff
    cmp.z x
    bcc !+
    lda.z x+1
    beq __b6
  !:
    lda #1
    ldy.z t
    cpy #0
    beq !e+
  !:
    asl
    dey
    bne !-
  !e:
    stx.z $ff
    ora.z $ff
    tax
  __b6:
    inc.z t
    jmp __b1
}
.segment Data
  //#define VIC ((byte*)0xD000)
  spriteXArray: .fill 2*8, 0
.pc = $2000 "SPRITE_DATA"
  // Dane sprite'a wczytane z obrazka PNG (czarny = 0, bialy = 1)
  // __address(0x2000) char SPRITE_DATA[63] = kickasm(resource "sprite.png") {{
  //     .var pic = LoadPicture("sprite.png", List().add($000000, $ffffff))
  //     .for (var y=0; y<21; y++)
  //         .for (var x=0; x<3; x++)
  //             .byte pic.getSinglecolorByte(x,y)
  // }};
  SPRITE_DATA: .byte $fe, 0, $7f, $83, 0, $81, $80, $81, 1, $80, $42, 1, $40, $24, 2, $20, $18, 4, $10, 0, 8, 8, 0, $10, 4, 0, $20, 2, 0, $40, 1, 0, $80, 2, 0, $40, 4, 0, $20, 8, 0, $10, $10, 0, 8, $20, $18, 4, $40, $24, 2, $80, $42, 1, $80, $81, 1, $81, 0, $83, $fe, 0, $7f, 1
.pc = $2040 "SPRITE1_DATA"
  // Drugi sprite - własny blok pamięci, adres musi być podzielny przez 64
  SPRITE1_DATA: .byte $fe, 0, $7f, $83, 0, $81, $80, $81, 1, $80, $42, 1, $40, $24, 2, $20, $18, 4, $10, 0, 8, 8, 0, $10, 4, 0, $20, 2, 0, $40, 1, 0, $80, 2, 0, $40, 4, 0, $20, 8, 0, $10, $10, 0, 8, $20, $18, 4, $40, $24, 2, $80, $42, 1, $80, $81, 1, $81, 0, $83, $fe, 0, $7f, 1
.pc = $2080 "SPRITE2_DATA"
  SPRITE2_DATA: .byte 0, 0, 0, 0, 0, 0, 3, $c0, 0, 7, $e0, 0, $f, $f0, 0, $1d, $b8, 0, $1f, $f8, 0, $12, $48, 0, $1e, $78, 0, $13, $c8, 0, $1f, $fc, $14, $1f, $fc, $14, $1f, $fe, $1c, $1f, $ff, $3c, $1f, $ff, $fc, $b, $af, $f8, $b, $af, $f0, $18, $30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
