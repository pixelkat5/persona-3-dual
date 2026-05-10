#pragma once
// Font char->tile map — gen_fontsheet.py
// Font: /mnt/c/Users/Greg/Documents/Persona-3-Dual/assets/fonts/Ultimate-Serial/Ultimate-Serial-Heavy Regular.ttf  Size: 24px  Cell: 32x24px

#define ULTIMATE_SERIAL_SHEET_CELL_W     32
#define ULTIMATE_SERIAL_SHEET_CELL_H     24
#define ULTIMATE_SERIAL_SHEET_COLS       16
#define ULTIMATE_SERIAL_SHEET_CHAR_COUNT 47
#define ULTIMATE_SERIAL_SHEET_TILES_PER  12

// int tile = ultimate_serial_sheet_CHAR_MAP[c - 32] * ULTIMATE_SERIAL_SHEET_TILES_PER;

static const unsigned char ultimate_serial_sheet_CHAR_MAP[47] = {
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 1, 31, 14, 12, 26, 13, 15, 8, 9, 14, 0
};
