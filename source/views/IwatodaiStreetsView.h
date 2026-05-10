#pragma once
#include "core/View.h"
#include "controllers/CharacterController.h"
#include "environments/iwatodai_streets.h"
#include <nds/arm9/console.h>

class IwatodaiStreetsView : public View {
    public:
        void Init() override;
        ViewState Update() override;
        void Cleanup() override;

    private:
        PrintConsole console;
        int bgSharedSlot;
        int bgFontSlot;

        CharacterController* playerCtrl;
            cameraPosition camPos;
            const float tileSize = 0.062500f;
            const float worldOffsetX = IWATODAI_STREETS_WORLD_OFFSET_X;
            const float worldOffsetZ = IWATODAI_STREETS_WORLD_OFFSET_Z;
            const Point2D<float> characterSize = Point2D<float>(0.1f, 0.1f);
            const float speed = 0.02f;
            const float angleIncrement = 0.05f;
            const float distance = 1.0f;
            const float lookAhead = 0.7f;
            // spawn in the middle of the street, facing along it (X axis)
            const Point2D<float> characterTranslate = Point2D<float>(0.0f, 0.0f);
            const float angle = 1.5708f;   // 90 degrees in radians — faces along X
            const float characterFacingAngle = 90.0f;
        int totalPolyCount = 0;
};
