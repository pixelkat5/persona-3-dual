#pragma once
// Font char->tile map — gen_fontsheet.py
// Font: /mnt/c/Users/Greg/Documents/Persona-3-Dual/assets/fonts/Rodin/FOT-Rodin Pro EB.otf  Size: 16px  Cell: 24x24px

#define RODIN_SHEET_CELL_W     24
#define RODIN_SHEET_CELL_H     24
#define RODIN_SHEET_COLS       16
#define RODIN_SHEET_CHAR_COUNT 73
#define RODIN_SHEET_TILES_PER  9

// int tile = rodin_sheet_CHAR_MAP[c - 32] * RODIN_SHEET_TILES_PER;

static const unsigned char rodin_sheet_CHAR_MAP[73] = {
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 1, 31, 14, 12, 26, 13, 15, 8, 9, 14, 0
};
