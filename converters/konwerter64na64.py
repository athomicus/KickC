#!/usr/bin/env python3
# konwerter64px.py — konwertuje obrazek 64x64px do kafla C64 hires
# Uzycie: python3 konwerter64px.py obrazek.png nazwa_kafla [bg_color_index]
# Wynik:  nazwa_kafla.h z static const unsigned char nazwa_kafla[576]
#
# Kafel: 64x64px = 8x8 komorek 8x8
# Format: [512B pikseli][64B kolorow] = 576B razem
# DRAW_TILE(nazwa, dest_cx, dest_cy, 8, 8, fg, bg);
#
# pip install Pillow

import sys
from PIL import Image

TILE_W      = 64
TILE_H      = 64
CELLS_W     = 8
CELLS_H     = 8
PIXEL_BYTES = 512
COLOR_BYTES = 64
SCRIPT_NAME = "konwerter64px.py"

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
    if img.size != (TILE_W, TILE_H):
        print(f"Skaluje z {{img.size}} do {{TILE_W}}x{{TILE_H}}px...")
        img = img.resize((TILE_W, TILE_H), Image.LANCZOS)

    pixels = list(img.getdata())

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

    for cy in range(CELLS_H):
        for cx in range(CELLS_W):
            cell_idx = cy * CELLS_W + cx
            color_data[cell_idx] = (global_fg << 4) | (forced_bg & 0x0F)
            for py in range(8):
                byte = 0
                for px in range(8):
                    r, g, b = pixels[(cy*8+py) * TILE_W + (cx*8+px)]
                    df = (r-fg_color[0])**2 + (g-fg_color[1])**2 + (b-fg_color[2])**2
                    db = (r-bg_color[0])**2 + (g-bg_color[1])**2 + (b-bg_color[2])**2
                    if df < db:
                        byte |= (1 << (7 - px))
                pixel_data[cell_idx * 8 + py] = byte

    data     = pixel_data + color_data
    out_path = tile_name + ".h"
    guard    = tile_name.upper() + "_H"
    total    = PIXEL_BYTES + COLOR_BYTES

    with open(out_path, "w") as f:
        f.write(f"#ifndef {guard}\n#define {guard}\n\n")
        f.write(f"/* Kafel {TILE_W}x{TILE_H}px ({CELLS_W}x{CELLS_H} komorek), rozmiar={total}B */\n")
        f.write(f"/* BG={forced_bg}, FG={global_fg} (globalny dla calego kafla) */\n")
        f.write(f"/* Uzycie: DRAW_TILE({tile_name}, dest_cx, dest_cy, {CELLS_W}, {CELLS_H}, TILE_COLOR_AUTO, TILE_COLOR_AUTO); */\n")
        f.write(f"/* lub:    DRAW_TILE({tile_name}, dest_cx, dest_cy, {CELLS_W}, {CELLS_H}, {global_fg}, {forced_bg}); */\n\n")
        f.write(f"static const unsigned char {tile_name}[{total}] = {{\n")
        f.write(f"  /* --- piksele ({PIXEL_BYTES}B) --- */\n")
        for i in range(0, PIXEL_BYTES, 8):
            chunk = data[i:i+8]
            f.write("  " + ", ".join(f"0x{{b:02X}}" for b in chunk) + ",\n")
        f.write(f"  /* --- kolory ({COLOR_BYTES}B: gorny nibble=fg, dolny=bg) --- */\n")
        for i in range(PIXEL_BYTES, total, 8):
            chunk = data[i:min(i+8, total)]
            f.write("  " + ", ".join(f"0x{{b:02X}}" for b in chunk) + ",\n")
        f.write("};\n\n#endif\n")

    print(f"Zapisano: {out_path} ({total} bajtow)")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(f"Uzycie:   python3 {SCRIPT_NAME} obrazek.png nazwa_kafla [bg]")
        print(f"          bg = indeks koloru tla C64 (domyslnie 0 = czarny)")
        print(f"Przyklad: python3 {SCRIPT_NAME} sprite.png sprite")
        print(f"          python3 {SCRIPT_NAME} sprite.png sprite 1   <- biale tlo")
        sys.exit(1)
    bg_arg = int(sys.argv[3]) if len(sys.argv) >= 4 else 0
    convert(sys.argv[1], sys.argv[2], forced_bg=bg_arg)
