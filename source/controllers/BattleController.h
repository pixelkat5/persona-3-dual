#pragma once

#include "components/menu/BattleMenuComponent.h"
#include "controllers/MusicController.h"

#include <algorithm>
#include <array>
#include <nds.h>
#include <string>
#include <vector>

#include "./battleActions/actions/AttackAction.h"
#include "./battleActions/actions/Guard.h"
#include "./battleActions/actions/PersonaAction.h"
#include "./battleActions/actions/SwitchPersona.h"

#include "./battleActions/BattleParticipant.h"
#include "./battleActions/BattlePhase.h"
#include "./battleActions/BattleResult.h"
#include "./battleActions/BattleStartCondition.h"
#include "./battleActions/TurnResult.h"
#include "./battleActions/enemies/Enemy.h"
#include "./battleActions/enemies/EnemyProfileDb.h"
#include "./battleActions/party/CharacterProfileDb.h"
#include "./battleActions/party/PartyMember.h"
#include "./battleActions/party/Player.h"

class BattleController
{
  private:
    // Singleton
    BattleController();

    BattleController(const BattleController&) = delete;
    BattleController& operator=(const BattleController&) = delete;

    static constexpr u32 ACTION_ATTACK = 0;
    static constexpr u32 ACTION_GUARD = 1;
    static constexpr u32 ACTION_PERSONA = 2;
    static constexpr u32 ACTION_SWITCH = 3;

    bool active = false;
    u32 turnsTaken = 0;

    BattlePhase phase;
    BattleResult battleResult;

    BattleParticipant* currentParticipantTurn = nullptr;
    u32 currentParticipantIndex = 0;

    int menuIndex = 0;
    Skill* selectedSkill = nullptr;

    bool pendingPersonaSwitch = false;
    bool switchedPersonaThisTurn = false;
    PersonaBase* personaBeforeSwitch = nullptr;

    std::string pendingAlert;
    BattlePhase alertReturnPhase = BattlePhase::Done;

    bool allOutAttackWasPossibleThisKnockDown = false;

    // Current battle data
    std::vector<BattleParticipant*> battleParticipants;
    std::vector<Enemy*> enemies;
    std::vector<PartyMember*> partyMembers;
    Player* player = nullptr;

    BattleStartCondition battleStartCondition = BattleStartCondition::Even;

    // Actions
    AttackAction attack;
    Guard guard;
    PersonaAction persona;
    SwitchPersona switchPersona;

    std::array<ActionBase*, 4> actions = {&attack, &guard, &persona, &switchPersona};

    // Internal helpers
    void applyResult(const TurnResult& r, BattleParticipant* target = nullptr);
    void advanceTurn();
    void setNextPhase(BattlePhase nextPhase);
    void calculateTurnOrder();
    void handleDeadParticipants();

    std::vector<BattleParticipant*> getAliveEnemies();

    bool allEnemiesKnockedDown();
    bool isSingleTarget(SkillType type);

    static bool getParticipantByHigherAgility(BattleParticipant* a, BattleParticipant* b)
    {
        return a->currentTurnOrderAgility > b->currentTurnOrderAgility;
    }

    MusicController* musicCtrl = MusicController::getInstance();
    BattleMenuComponent* battleMenuCmpt = BattleMenuComponent::getInstance();

  public:
    static BattleController* getInstance()
    {
        static BattleController instance;
        return &instance;
    }

    bool isActive() const
    {
        return active;
    }

    BattlePhase getPhase() const
    {
        return phase;
    }

    void execute(CharacterProfile& player,
                 std::vector<CharacterProfile>& characterProfiles,
                 std::vector<EnemyProfile>& enemyProfiles,
                 BattleStartCondition battleStartCondition);

    BattleResult update(u32 keys);

    void exit();

    ~BattleController()
    {
    }

    uint16_t* textVideoBufferSub;
};
