#include <nds.h>
#include <stdio.h>
#include <malloc.h>
#include "core/globals.h"
#include "math.h"
#include "IwatodaiDormView.h"

// model
#include "models/character.h"
// environment
#include "environments/iwatodai_dorm.h"
#include "texture.h"
// maps
#include "maps/iwatodai_dorm.h"
// dialogue
#include "dialogue/demo_dialogue.h"

// TODO: move to header
int characterTextureId;
iwatodai_dorm_Environment iwatodaiDormEnv;

// TODO: dont forget to clear in future
IwatodaiDormView::IwatodaiDormView() : enemies(new std::vector<Enemy *>({&merciless_Maya, &cowardly_Maya})),
                                       battleController(&player, enemies) {}

void IwatodaiDormView::Init()
{
    videoSetMode(MODE_0_3D);
    videoSetModeSub(MODE_0_2D);

    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankB(VRAM_B_TEXTURE);
    vramSetBankC(VRAM_C_SUB_BG);
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

    // setup shared bg slot on sub screen
    bgSharedSlot = bgInitSub(0, BgType_Text8bpp, BgSize_T_256x256, 0, 1);
    dmaFillHalfWords(0, bgGetMapPtr(bgSharedSlot), 2048);

    // setup console
    consoleInit(&console, 1, BgType_Text4bpp, BgSize_T_256x256, 4, 5, false, true);
    consoleSelect(&console);

    // adjust sub screen image and console to sit correctly on each other
    bgSetPriority(console.bgId, 0);
    bgSetPriority(bgSharedSlot, 1);

    bgUpdate();

    // setup player controller
    playerCtrl = new CharacterController(IWATODAI_DORM_MAP_WIDTH, IWATODAI_DORM_MAP_WIDTH, &iwatodai_dorm_map[0][0], tileSize, worldOffsetX, worldOffsetZ, characterSize, speed, angleIncrement, distance, lookAhead, angle, characterTranslate, characterFacingAngle);

    // setup music
    // 1/3 chance of a surprise
    if (rand() % 3 == 0) {
        musicCtrl.init("nitro:/music/toms_diner_suzanne_vega.pcm", 0.0f, 234.568f);
    } else {
        musicCtrl.init(IWATODAI_DORM_MUSIC, 0.0f, 920.973f);
    }

    // setup character model
    characterAnimationCtrl.loadModel("nitro:/models/character.bin");
    character_loadTextures(characterAnimationCtrl, bitmapsCharacter);
    characterAnimationCtrl.set(MODEL_CHARACTER_1, true);
    characterAnimationCtrl.play();

    // setup environment model
    const unsigned int* bitmapsEnv[IWATODAI_DORM_TEX_COUNT] = { textureBitmap };
    iwatodaiDormEnv.load("nitro:/environments/iwatodai_dorm.bin", bitmapsEnv);
    totalPolyCount = iwatodaiDormEnv.getPolyCount();

    // setup dialogue
    demo_dialogue_bg_slot = bgSharedSlot;

    // setup pause menu
    // use the same shared background slot as the demo dialogue
    pauseMenuCmpt.init(bgSharedSlot, &isPauseMenuActive);
}

ViewState IwatodaiDormView::Update()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    bgUpdate();

    scanKeys();
    u32 keys = keysHeld();
    u32 pressed = keysDown();

    if (pressed & KEY_START) {
        isPauseMenuActive = !isPauseMenuActive;
    }

    if (isPauseMenuActive) {
        ViewState menuResult = pauseMenuCmpt.update(pressed);
        if (menuResult != ViewState::KEEP_CURRENT) {
            musicCtrl.pause();
            return menuResult;
        }
    } else {    // only render world when pause menu is not active
        // only process world input when dialogue and battle are not active
        if (!dialogueCtrl.isActive() && !battleController.isActive()) {
            // move character
            camPos = playerCtrl->update(keys);

            // start battle
            if (keys & KEY_Y)
            {
                battleController.execute();
            }

            // trigger dialogue from interaction
            demo_unload();
            if (playerCtrl->isTileAt() == TileType::NEXT_SCENE) {
                musicCtrl.pause();
                return ViewState::IWATODAI_STREETS;
            } else if (playerCtrl->isTileAt() == TileType::CHARACTER_Akihiko) {
                iprintf("\x1b[0;0HPress A to talk");
                if (pressed & KEY_A) {
                    demo_yuki_guard_argument_load();
                    dialogueCtrl.setLoader(demo_yuki_guard_argument_load_bg);
                    dialogueCtrl.start(demo_yuki_guard_argument_first());
                }
            } else {
                consoleClear();
            }
        }

        // update camera position
        gluLookAt(camPos.cameraX, camPos.cameraY, camPos.cameraZ,
            camPos.targetX, camPos.targetY, camPos.targetZ,
            camPos.upX, camPos.upY, camPos.upZ);


        // draw environment
        glPushMatrix();
            iwatodaiDormEnv.draw();
            iwatodaiDormEnv.drawBillboards(
                enableBillboards,  // billboards face camera
                camPos.cameraX, camPos.cameraY, camPos.cameraZ
            );
        glPopMatrix(1);

        // draw character
        glPushMatrix();
            // move character
            characterPosition charPos = playerCtrl->isCharacterAt();
            glTranslatef(charPos.x, 0.1, charPos.z);
            glRotatef(charPos.facingAngle, 0.0f, 1.0f, 0.0f);

            // draw character
            characterAnimationCtrl.render();
        glPopMatrix(1);

        glFlush(0);

        // print coordinates (64x64 area from 0,0 to 64,64)
        if (enableDebugPrint) {
            iprintf("\x1b[21;0Htile(x,z): %d, %d",
                (int)((charPos.x + worldOffsetX) / tileSize),
                (int)((charPos.z + worldOffsetZ) / tileSize));
            iprintf("\x1b[22;0Htranslate(x,z): %d, %d",
                (int)(charPos.x * 100),
                (int)(charPos.z * 100));
            iprintf("\x1b[23;0Hangle(w,c): %d, %d", (int)(charPos.angle * 100), (int)(charPos.facingAngle * 100));
        }
    }

    // update controllers
    battleController.update(pressed);
    dialogueCtrl.update(keys);
    characterAnimationCtrl.update();
    musicCtrl.update();

    return ViewState::KEEP_CURRENT;
}

void IwatodaiDormView::Cleanup()
{
    // clear screen
    setBrightness(3, 0);
    consoleClear();

    // stop pause menu sfx
    pauseMenuCmpt.cancelSFX();
    isPauseMenuActive = false;

    // cleanup environment
    iwatodaiDormEnv.cleanup();
    // reset textures
    glDeleteTextures(1, &characterTextureId);
    // reset shared bg slot
    dmaFillHalfWords(0, bgGetMapPtr(bgSharedSlot), 2048);

    // reset vram
    vramSetBankA(VRAM_A_LCD);
    vramSetBankB(VRAM_B_LCD);
    vramSetBankC(VRAM_C_LCD);
    vramSetBankD(VRAM_D_LCD);
    vramSetBankH(VRAM_H_LCD);

    // cleanup controllers
    delete playerCtrl;
    playerCtrl = NULL;
}
