#pragma once
#include <nds.h>

#define FONT_SCREEN_COLS 32
#define FONT_SCREEN_ROWS 24

class FontRenderer {
public:
    // Pass in an already-initialized bgSlot, the font tile/palette data,
    // and char map info from the grit-generated _map.h header.
    void init(
        int bgSlot,
        const unsigned int*   fontTiles,    unsigned int fontTilesLen,
        const unsigned short* fontPal,      unsigned int fontPalLen,
        const unsigned char*  charMap,      int charCount,
        int charBase,       // ASCII code of first char (usually 32 = space)
        int cellTilesW,     // cell width in 8px tiles
        int cellTilesH,     // cell height in 8px tiles
        int sheetCols       // columns in the font sheet
    );

    void clear();
    void draw(int tileX, int tileY, const char* str);
    void drawCentered(int tileY, const char* str);
    void drawChar(int tileX, int tileY, char c);

private:
    int bgSlot     = -1;
    int cellTilesW = 1;
    int cellTilesH = 1;
    int sheetCols  = 16;
    int charBase   = 32;
    int charCount  = 95;
    const unsigned char* charMap = nullptr;

    void placeTiles(int tileX, int tileY, int startTile);
};
