#pragma once
#include "../enemies/Enemy.h"
#include "../party/PartyMember.h"
#include "ActionBase.h"

struct PersonaAction : ActionBase
{
    PersonaAction()
    {
        name = "Persona";
        possibleUsers = ParticipantType::Party;
    }

    TurnResult resolve(PartyMember* user, BattleParticipant* target, Skill* skill) override;
};
