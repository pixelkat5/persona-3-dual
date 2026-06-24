#pragma once
#include "controllers/CharacterController.h"
#include "controllers/ICameraStrategy.h"
#include "environments/station.h"
#include "views/BaseView3D.h"
#include <nds/arm9/console.h>

class StationView : public BaseView3D
{
  public:
    void init() override;
    ViewState update() override;
    void cleanup() override;
    void setupEnvironment() override;

  private:
    touchPosition touch;

    ViewPhase phase;
    bool prevPauseState;
    bool prevEnvironmentState;

    // sub screen
    int bgSharedSub1;
    int bgSharedSub2;
    int bgSharedSub3;
    PrintConsole console;

    // 3D
    station_Environment stationEnv;

    CharacterController* playerCtrl;
    ICameraStrategy* cameraStrategy;
    CameraPosition camPos;
    const float tileSize = 0.062500f;
    const float worldOffsetX = STATION_WORLD_OFFSET_X;
    const float worldOffsetZ = STATION_WORLD_OFFSET_Z;
    const Point2D<float> characterSize = Point2D<float>(0.1f, 0.1f);
    const float speed = 0.02f;
    const float angleIncrement = 0.05f;
    
    const Point2D<float> characterTranslate = Point2D<float>(-0.0175f, 1.3216f);
    const float height = 0.0f;
    const float angle = 1.5708f * 2; // 180 degrees (rad)
    const float characterFacingAngle = 180.0f;

    const Point2D<float> fixedCameraOrigin = Point2D<float>(-0.0175f, 2.0216f);
    const float fixedCameraHeight = 0.6f;
    const float fixedCameraSmoothing = 0.06f;
};
