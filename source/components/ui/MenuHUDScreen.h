#pragma once
#include "components/ui/UIScreen.h"
#include "core/enums.h"
#include "core/globals.h"
#include "core/structs.h"
#include <nds.h>

class MenuHUDScreen : public UIScreen
{
  public:
    void load();
    void unload();
    void renderSprites() override;
    void removeSprites() override;
    int onTouch(touchPosition* touch) override;

  private:
    // NOTE: we can have max:
    // 1 moon
    // 1 day of the week
    // 4 numbers
    // 4 times
    // 18 skill progress items (all same sprite)

    // sprites
    Sprite sprites[28]; // enough entries for moon, day, digits, times, and repeated skill markers
    SpriteRegister moonSprite;
    SpriteRegister dayOfWeekSprite;
    SpriteRegister numberSprites[4];
    SpriteRegister timeSprites[4];
    SpriteRegister skillSprites[18];
    SpriteRegister slashSprite;
    bool bgLoaded;
    void renderBackground();
};
