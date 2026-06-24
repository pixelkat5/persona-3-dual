#pragma once
#include "components/menu/BattleMenuComponent.h"
#include "components/menu/PauseMenuComponent.h"
#include "components/ui/DialogueScreen.h"
#include "components/ui/MenuHUDScreen.h"
#include "controllers/AnimationController.h"
#include "controllers/GraphicsController.h"
#include "controllers/MusicController.h"
#include "controllers/SaveController.h"
#include "controllers/SpriteController.h"
#include "controllers/UIController.h"
#include "controllers/VideoController.h"

class PauseMenuComponent;
class DialogueScreen;
class MenuHUDScreen;

// variables
extern volatile int frame;
extern int fps;
extern int fpsTimer;
extern std::string fatBasePath;
extern Save saveData;
extern unsigned int** bitmapsCharacter;

// controllers
extern SaveController saveCtrl;
extern MusicController musicCtrl;
extern VideoController videoCtrl;
extern AnimationController characterAnimationCtrl;
extern SpriteController spriteCtrl;
extern GraphicsController graphicsCtrl;
extern UIController uiCtrl;

// components
extern PauseMenuComponent pauseMenuCmpt;
extern bool enableBillboards;
extern bool enableDebugPrint;
extern bool enableCharacterAnim;
extern bool isPauseMenuActive;
extern BattleMenuComponent battleMenuCmpt;
extern MenuHUDScreen menuHUDScreen;
extern DialogueScreen dialogueScreen;
