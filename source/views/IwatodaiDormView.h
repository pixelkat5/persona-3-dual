#include "core/View.h"
#include <nds/arm9/console.h>
// controllers
#include "controllers/DialogueController.h"
#include "controllers/CharacterController.h"
// environments
#include "environments/iwatodai_dorm.h"
// battle-related
#include "./battleActions/party/curPlayer.h"
#include "./battleActions/enemies/Enemy.h"
#include "./battleActions/enemies/Cowardly_Maya.h"
#include "./battleActions/enemies/Merciless_Maya.h"
#include "./controllers/BattleController.h" // TODO: move somewhere

// implementing from View
class IwatodaiDormView : public View {
    public:
        // override tells compiler we intend to override a virtual fn in a base class (i.e. View)
        void Init() override;
        ViewState Update() override;
        void Cleanup() override;
        IwatodaiDormView();

    private:
        // sub screen
        PrintConsole console;

        // Battle participants
        curPlayer player;
        Cowardly_Maya cowardly_Maya;
        Merciless_Maya merciless_Maya;
        std::vector<Enemy*>* enemies;

        // controllers
        BattleController battleController;
        CharacterController* playerCtrl;
            // camera pos
            cameraPosition camPos;
            // world
            const float tileSize = 0.062500f;
            const float worldOffsetX = IWATODAI_DORM_WORLD_OFFSET_X;
            const float worldOffsetZ = IWATODAI_DORM_WORLD_OFFSET_Z;
            const Point2D<float> characterSize = Point2D<float>(0.1f, 0.1f);
            // movement and viewpoint
            const float speed = 0.02f;
            const float angleIncrement = 0.05f;
            const float distance = 0.5f;
            const float lookAhead = 0.3f;
            // set character initial translation position
            const Point2D<float> characterTranslate = Point2D<float>(0, 0);
            const float angle = -1.6;
            const float characterFacingAngle = 91.67;
        DialogueController dialogueCtrl;
            int bgSharedSlot;
            int bgFontSlot;
        int totalPolyCount = 0;
};
