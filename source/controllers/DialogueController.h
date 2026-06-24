#pragma once
#include "core/structs.h"
#include <nds.h>
#include <string>
#include <vector>

class DialogueController
{
  public:
    DialogueController();
    void start(Dialogue* firstLine);
    void update(u32 keys);
    void exit();
    bool isActive() const
    {
        return active;
    }

    void setLoader(void (*loader)(int bgIndex))
    {
        bgLoader = loader;
    }

  private:
    void advanceTo(Dialogue* next);
    void renderAnimFrame();
    void renderOptions();

    Dialogue* current = nullptr;
    int optionCount = 0;
    int selectedOption = 0;
    bool active = false;
    bool animDone = false;
    bool doRenderOptions = false;

    // track currently loaded imageId
    int loadedImageId = -1;

    void (*bgLoader)(int bgIndex) = nullptr;

    int animIndex = 0;
    u32 prevKeys = 0;
};
