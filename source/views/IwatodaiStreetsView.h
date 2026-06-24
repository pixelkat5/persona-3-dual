#pragma once
#include "controllers/CharacterController.h"
#include "controllers/ICameraStrategy.h"
#include "environments/iwatodai_streets.h"
#include "views/BaseView3D.h"
#include <nds/arm9/console.h>

class IwatodaiStreetsView : public BaseView3D
{
  public:
    void init() override;
    ViewState update() override;
    void cleanup() override;
    void setupEnvironment() override;

  private:
    touchPosition touch;

    // sub screen
    int bgSharedSub1;
    int bgSharedSub2;
    int bgSharedSub3;
    PrintConsole console;

    ViewPhase phase;
    bool prevPauseState;
    bool prevEnvironmentState;

    // 3D
    iwatodai_streets_Environment iwatodaiStreetsEnv;

    CharacterController* playerCtrl;
    ICameraStrategy* cameraStrategy;
    CameraPosition camPos;
    const float tileSize = 0.062500f;
    const float worldOffsetX = IWATODAI_STREETS_WORLD_OFFSET_X;
    const float worldOffsetZ = IWATODAI_STREETS_WORLD_OFFSET_Z;
    const Point2D<float> characterSize = Point2D<float>(0.1f, 0.1f);
    const float speed = 0.03f;
    const float angleIncrement = 0.05f;
    // spawn in the middle of the street, facing along it (X axis)
    const Point2D<float> characterTranslate = Point2D<float>(0.60f, 0.60f);
    const float height = 0.05f;
    const float angle = 1.5708f * 2; // 180 degrees (rad)
    const float characterFacingAngle = 0.0f;

    const Point2D<float> fixedCameraOrigin = Point2D<float>(1.6f, 2.1f);
    const float fixedCameraHeight = 1.8f;
    const float fixedCameraSmoothing = 0.06f;
};
