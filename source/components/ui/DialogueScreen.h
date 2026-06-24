#pragma once
#include "components/ui/UIScreen.h"
#include "core/enums.h"
#include "core/globals.h"
#include "core/structs.h"
#include <nds.h>

class DialogueScreen : public UIScreen
{
  public:
    void load();
    void unload();
    void renderSprites() override;
    void removeSprites() override;

  private:
    // sprites
    // TODO: reduce allocated sprite/sprite registers
    Sprite sprites[50];
    SpriteRegister calendarSprite[2];
    SpriteRegister textBox[10];
    SpriteRegister nameTag[10];

    void renderBackground();
};
