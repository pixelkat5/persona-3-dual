#include "FontRenderer.h"
#include <string.h>

void FontRenderer::init(
    int iBgSlot,
    const unsigned int*   fontTiles,    unsigned int fontTilesLen,
    const unsigned short* fontPal,      unsigned int fontPalLen,
    const unsigned char*  iCharMap,     int iCharCount,
    int iCharBase, int iCellTilesW, int iCellTilesH, int iSheetCols
) {
    bgSlot     = iBgSlot;
    charMap    = iCharMap;
    charCount  = iCharCount;
    charBase   = iCharBase;
    cellTilesW = iCellTilesW;
    cellTilesH = iCellTilesH;
    sheetCols  = iSheetCols;

    // Load font tiles into the GFX region of the provided bgSlot
    dmaCopy(fontTiles, bgGetGfxPtr(bgSlot), fontTilesLen);

    // 4bpp: copy palette to sub BG palette slot 2 (16 colours at offset 2*16)
    dmaCopy(fontPal, BG_PALETTE_SUB + (16 * 2), fontPalLen > 32 ? 32 : fontPalLen);

    clear();
}

void FontRenderer::clear() {
    u16* map = (u16*)bgGetMapPtr(bgSlot);
    memset(map, 0, FONT_SCREEN_COLS * FONT_SCREEN_ROWS * sizeof(u16));
}

void FontRenderer::placeTiles(int tileX, int tileY, int startTile) {
    u16* map = (u16*)bgGetMapPtr(bgSlot);
    for (int row = 0; row < cellTilesH; row++) {
        for (int col = 0; col < cellTilesW; col++) {
            int mapX = tileX + col;
            int mapY = tileY + row;
            if (mapX >= FONT_SCREEN_COLS || mapY >= FONT_SCREEN_ROWS) continue;
            // tile entry: bits 0-9 = tile index, bits 12-15 = palette slot (2)
            map[mapY * FONT_SCREEN_COLS + mapX] = (u16)((2 << 12) | (startTile + row * sheetCols + col));
        }
    }
}

void FontRenderer::drawChar(int tileX, int tileY, char c) {
    int code = (int)(unsigned char)c - charBase;
    if (code < 0 || code >= charCount) return;
    int cellIndex  = charMap[code];              // which cell in the character grid
    int gridCols   = sheetCols / cellTilesW;     // number of character columns in the sheet
    int cellRow    = cellIndex / gridCols;
    int cellCol    = cellIndex % gridCols;
    // top-left tile of this cell in the flat tile array
    int startTile  = (cellRow * cellTilesH) * sheetCols + cellCol * cellTilesW;
    placeTiles(tileX, tileY, startTile);
}

void FontRenderer::draw(int tileX, int tileY, const char* str) {
    int x = tileX;
    while (*str) {
        drawChar(x, tileY, *str);
        x += cellTilesW;
        str++;
    }
}

void FontRenderer::drawCentered(int tileY, const char* str) {
    int len       = strlen(str);
    int totalTiles = len * cellTilesW;
    int startX    = (FONT_SCREEN_COLS - totalTiles) / 2;
    draw(startX, tileY, str);
}
