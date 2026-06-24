#include "views/BaseView3D.h"
#include <nds/arm9/console.h>
// controllers
#include "controllers/CharacterController.h"
#include "controllers/DialogueController.h"
#include "controllers/ICameraStrategy.h"
// environments
#include "environments/paulownia_mall.h"
// battle-related
#include "./battleActions/BattleParticipant.h"
#include "./battleActions/BattleStartCondition.h"
#include "./battleActions/enemies/EnemyDb.h"

class PaulowniaMallView : public BaseView3D
{
  public:
    void init() override;
    ViewState update() override;
    void cleanup() override;
    void setupEnvironment() override;
    PaulowniaMallView();

  private:
    touchPosition touch;

    // sub screen
    int bgSharedSub1;
    int bgSharedSub2;
    int bgSharedSub3;
    PrintConsole console;

    // 3D
    paulownia_mall_Environment paulowniaMallEnv;

    ViewPhase phase;
    bool prevPauseState;
    bool prevDialogueState;
    bool prevEnvironmentState;

    CharacterController* playerCtrl;
    ICameraStrategy* cameraStrategy;

    // camera pos
    CameraPosition camPos;
    // world
    const float tileSize = 0.062500f;
    const float worldOffsetX = PAULOWNIA_MALL_WORLD_OFFSET_X;
    const float worldOffsetZ = PAULOWNIA_MALL_WORLD_OFFSET_Z;
    const Point2D<float> characterSize = Point2D<float>(0.1f, 0.1f);
    // movement and viewpoint
    const float speed = 0.03f;
    const float angleIncrement = 0.05f;

    const Point2D<float> characterTranslate = Point2D<float>(0.0122f, 2.3355f);
    const float height = 1.999f;
    const float angle = 2.5f * 2; // 180 degrees (rad)
    const float characterFacingAngle = 180;

    const Point2D<float> fixedCameraOrigin = Point2D<float>(0.0122f, 3.1355f);
    const float fixedCameraHeight = 2.35f;
    const float fixedCameraSmoothing = 0.06f;

    DialogueController dialogueCtrl;
    bool isBattleMenuActive = false;

    void setMusic();
};
