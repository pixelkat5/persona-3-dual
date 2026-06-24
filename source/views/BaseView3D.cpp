#include "BaseView3D.h"
#include "core/globals.h"
#include <nds.h>
#include <stdio.h>

void BaseView3D::init()
{
    // set modes
    videoSetMode(MODE_0_3D);
    videoSetModeSub(MODE_0_2D);

    // set vram
    vramSetBankA(VRAM_A_TEXTURE_SLOT0); // texture slot 0
    vramSetBankB(VRAM_B_TEXTURE_SLOT1); // texture slot 1

    vramSetBankC(VRAM_C_SUB_BG);
    vramSetBankD(VRAM_D_SUB_SPRITE);
    vramSetBankH(VRAM_H_SUB_BG_EXT_PALETTE);
    vramSetBankI(VRAM_I_SUB_SPRITE_EXT_PALETTE);
    bgExtPaletteEnableSub();

    // main screen, 3D
    glInit();
    glEnable(GL_ANTIALIAS);
    glEnable(GL_TEXTURE_2D); // enable texturing

    glClearColor(0, 0, 0, 31);
    glClearPolyID(63);
    glClearDepth(0x7FFF);

    // set size of main screen
    glViewport(0, 0, 255, 191);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // zNear is how close the camera can see, zFar is the maximum draw distance
    gluPerspective(55, 256.0 / 192.0, 0.1, 40);

    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
    glColor3b(255, 255, 255); // keep white so texture colors aren't tinted
}
