#include "SpriteController.h"
#include "core/globals.h"
#include "data/spriteDb.h"

bool SpriteController::switchSpriteImpl(SpriteType type, int spriteId, SpriteRegister* out)
{
    std::string filename = getSpriteFilename(type, spriteId);
    if (filename.empty())
    {
        return false;
    }

    // TODO: pass path
    GraphicAsset asset = graphicsCtrl.loadGrit(fatBasePath + spritePath + filename);
    loadedAssets.push_back(asset);

    out->id = spriteId;
    out->tiles = asset.tiles;
    out->tilesLen = asset.tilesLen;
    out->pal = asset.pal;
    out->palLen = asset.palLen;

    return true;
}

void SpriteController::unloadAll()
{
    for (size_t i = 0; i < loadedAssets.size(); ++i)
    {
        graphicsCtrl.unloadGrit(loadedAssets[i]);
    }
    loadedAssets.clear();
}
