#pragma once
#include "core/enums.h"
#include "core/structs.h"
#include <nds.h>
#include <vector>

class SpriteController
{
  public:
    std::string spritePath;
    template <typename SpriteID> bool switchSprite(SpriteType type, SpriteID spriteId, SpriteRegister* out)
    {
        return switchSpriteImpl(type, static_cast<int>(spriteId), out);
    }

    void unloadAll();

  private:
    std::vector<GraphicAsset> loadedAssets;
    bool switchSpriteImpl(SpriteType type, int spriteId, SpriteRegister* out);
};
