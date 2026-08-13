#!/usr/bin/env python3
# konwerter32px.py — konwertuje obrazek 32x32px do kafla C64 hires
# Uzycie: python3 konwerter32px.py obrazek.png nazwa_kafla [bg_color_index]
# Wynik:  nazwa_kafla.h z static const unsigned char nazwa_kafla[144]
#
# Kafel: 32x32px = 4x4 komorek 8x8
# Format: [128B pikseli][16B kolorow] = 144B razem
# DRAW_TILE(nazwa, dest_cx, dest_cy, 4, 4, fg, bg);
#
# pip install Pillow

import sys
from PIL import Image

CELLS       = 4
TILE_PX     = 32
PIXEL_BYTES = 128
COLOR_BYTES = 16
TOTAL       = 144

C64_PALETTE = [
    (0,   0,   0  ),  # 0  czarny
    (255, 255, 255),  # 1  bialy
    (136, 0,   0  ),  # 2  czerwony
    (170, 255, 238),  # 3  cyan
    (204, 68,  204),  # 4  fioletowy
    (0,   204, 85 ),  # 5  zielony
    (0,   0,   170),  # 6  niebieski
    (238, 238, 119),  # 7  zolty
    (221, 136, 85 ),  # 8  pomaranczowy
    (102, 68,  0  ),  # 9  brazowy
    (255, 119, 119),  # 10 jasno-czerwony
    (51,  51,  51 ),  # 11 ciemnoszary
    (119, 119, 119),  # 12 szary
    (170, 255, 102),  # 13 jasno-zielony
    (0,   136, 255),  # 14 jasno-niebieski
    (187, 187, 187),  # 15 jasnoszary
]

def nearest_c64(r, g, b):
    best, best_dist = 0, float("inf")
    for i, (cr, cg, cb) in enumerate(C64_PALETTE):
        d = (r - cr)**2 + (g - cg)**2 + (b - cb)**2
        if d < best_dist:
            best_dist = d
            best = i
    return best

def convert(input_path, tile_name, forced_bg=0):
    img = Image.open(input_path).convert("RGB")
    if img.size != (TILE_PX, TILE_PX):
        print(f"Skaluje z {img.size} do {TILE_PX}x{TILE_PX}px...")
        img = img.resize((TILE_PX, TILE_PX), Image.LANCZOS)

    pixels = list(img.getdata())

    # Wyznacz globalny FG — najczestszy kolor rozny od BG
    global_counts = {}
    for r, g, b in pixels:
        c = nearest_c64(r, g, b)
        global_counts[c] = global_counts.get(c, 0) + 1

    candidates = sorted(
        [(c, cnt) for c, cnt in global_counts.items() if c != forced_bg],
        key=lambda x: -x[1]
    )
    global_fg = candidates[0][0] if candidates else (1 if forced_bg != 1 else 2)

    print(f"BG (tlo)     = {forced_bg}  {C64_PALETTE[forced_bg]}")
    print(f"FG (rysunek) = {global_fg}  {C64_PALETTE[global_fg]}")

    fg_color = C64_PALETTE[global_fg]
    bg_color = C64_PALETTE[forced_bg]

    pixel_data = bytearray(PIXEL_BYTES)
    color_data  = bytearray(COLOR_BYTES)

    for cy in range(CELLS):
        for cx in range(CELLS):
            cell_idx = cy * CELLS + cx
            # Globalny kolor dla wszystkich komorek — brak mieszania fg/bg
            color_data[cell_idx] = (global_fg << 4) | (forced_bg & 0x0F)
            for py in range(8):
                byte = 0
                for px in range(8):
                    r, g, b = pixels[(cy*8+py) * TILE_PX + (cx*8+px)]
                    df = (r-fg_color[0])**2 + (g-fg_color[1])**2 + (b-fg_color[2])**2
                    db = (r-bg_color[0])**2 + (g-bg_color[1])**2 + (b-bg_color[2])**2
                    if df < db:
                        byte |= (1 << (7 - px))
                pixel_data[cell_idx * 8 + py] = byte

    data     = pixel_data + color_data
    out_path = tile_name + ".h"
    guard    = tile_name.upper() + "_H"

    with open(out_path, "w") as f:
        f.write(f"#ifndef {guard}\n#define {guard}\n\n")
        f.write(f"/* Kafel 32x32px (4x4 komorek), rozmiar=144B */\n")
        f.write(f"/* BG={forced_bg}, FG={global_fg} (globalny dla calego kafla) */\n")
        f.write(f"/* Uzycie: DRAW_TILE({tile_name}, dest_cx, dest_cy, 4, 4, TILE_COLOR_AUTO, TILE_COLOR_AUTO); */\n")
        f.write(f"/* lub:    DRAW_TILE({tile_name}, dest_cx, dest_cy, 4, 4, {global_fg}, {forced_bg}); */\n\n")
        f.write(f"static const unsigned char {tile_name}[{TOTAL}] = {{\n")
        f.write("  /* --- piksele (128B) --- */\n")
        for i in range(0, PIXEL_BYTES, 8):
            chunk = data[i:i+8]
            f.write("  " + ", ".join(f"0x{b:02X}" for b in chunk) + ",\n")
        f.write("  /* --- kolory (16B: gorny nibble=fg, dolny=bg) --- */\n")
        for i in range(PIXEL_BYTES, TOTAL, 8):
            chunk = data[i:min(i+8, TOTAL)]
            f.write("  " + ", ".join(f"0x{b:02X}" for b in chunk) + ",\n")
        f.write("};\n\n#endif\n")

    print(f"Zapisano: {out_path} ({TOTAL} bajtow)")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Uzycie:   python3 konwerter32px.py obrazek.png nazwa_kafla [bg]")
        print("          bg = indeks koloru tla C64 (domyslnie 0 = czarny)")
        print("Przyklad: python3 konwerter32px.py n05.png n05")
        print("          python3 konwerter32px.py grass.png grass 5")
        sys.exit(1)
    bg_arg = int(sys.argv[3]) if len(sys.argv) >= 4 else 0
    convert(sys.argv[1], sys.argv[2], forced_bg=bg_arg)