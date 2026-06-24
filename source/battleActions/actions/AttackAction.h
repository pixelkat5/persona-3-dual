#pragma once
#include "../enemies/Enemy.h"
#include "../party/PartyMember.h"
#include "ActionBase.h"

struct AttackAction : ActionBase
{
    AttackAction()
    {
        name = "Attack";
        possibleUsers = ParticipantType::Party;
    }

    TurnResult resolve(PartyMember* user, BattleParticipant* target, Skill* skill = nullptr) override;
};
