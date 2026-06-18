#include "views/BaseView3D.h"
#include <nds/arm9/console.h>
// controllers
#include "controllers/CharacterController.h"
#include "controllers/DialogueController.h"
#include "controllers/ICameraStrategy.h"
// environments
#include "environments/iwatodai_dorm_floor_1.h"
// battle-related
#include "./battleActions/BattleParticipant.h"
#include "./battleActions/BattleStartCondition.h"
#include "./battleActions/enemies/EnemyDb.h"
#include "./battleActions/party/CharacterProfileDb.h"
#include "./controllers/BattleController.h" // TODO: move somewhere
#include <memory>

class IwatodaiDormView : public BaseView3D
{
  public:
    void init() override;
    ViewState update() override;
    void cleanup() override;
    void setupEnvironment() override;
    IwatodaiDormView();

  private:
    touchPosition touch;

    // sub screen
    int bgSharedSub1;
    int bgSharedSub2;
    int bgSharedSub3;
    PrintConsole console;

    // 3D
    iwatodai_dorm_floor_1_Environment iwatodaiDormFloor1Env;

    ViewPhase phase;
    bool prevBattleState;
    bool prevPauseState;
    bool prevDialogueState;
    bool prevEnvironmentState;

    // Battle participants
    Enemy* mercilessMaya = new Enemy(EnemyDb::mercilessMaya);
    Enemy* cowardlyMaya = new Enemy(EnemyDb::cowardlyMaya);
    Player* player = new Player(CharacterProfileDb::player);
    PartyMember* yukari = new PartyMember(CharacterProfileDb::yukari);
    PartyMember* junpei = new PartyMember(CharacterProfileDb::junpei);

    std::vector<BattleParticipant*> battleParticipants = {mercilessMaya, cowardlyMaya, player, yukari, junpei};
    std::vector<Enemy*> enemies = {mercilessMaya, cowardlyMaya};
    std::vector<PartyMember*> partyMembers = {player, yukari, junpei};

    // hardcoded for now, we will have to build a battle creater for tartarus anyways
    BattleStartCondition battleStartCondition = BattleStartCondition::Even;

    // controllers
    BattleController battleController;
    CharacterController* playerCtrl;
    ICameraStrategy* cameraStrategy;
    // camera pos
    CameraPosition camPos;
    // world
    const float tileSize = 0.062500f;
    const float worldOffsetX = IWATODAI_DORM_FLOOR_1_WORLD_OFFSET_X;
    const float worldOffsetZ = IWATODAI_DORM_FLOOR_1_WORLD_OFFSET_Z;
    const Point2D<float> characterSize = Point2D<float>(0.1f, 0.1f);
    // movement and viewpoint
    const float speed = 0.03f;
    const float angleIncrement = 0.07f;

    const Point2D<float> characterTranslate = Point2D<float>(0.4f, 2.8f);
    const float height = 0.0;
    const float angle = -1.6;
    const float characterFacingAngle = 180.0f;

    const Point2D<float> fixedCameraOrigin = Point2D<float>(-0.3997f, 2.8234f);
    const float fixedCameraHeight = 0.6f;
    const float fixedCameraSmoothing = 0.06f;

    DialogueController dialogueCtrl;

    bool isBattleMenuActive = false;

    void setMusic();
};
