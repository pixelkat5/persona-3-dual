#pragma once
#include "../ParticipantType.h"
#include "../personas/PersonaBase.h"
#include "../shoes/Shoe.h"
#include "../skills/Skill.h"
#include "../weapons/Weapon.h"
#include <nds.h>
#include <vector>

struct CharacterProfile
{
    std::string name;
    s32 maxHp;
    s32 hp;
    s32 maxSp;
    s32 sp;
    u32 lv;
    ParticipantType participantType;

    ArmourType armourType;
    Armour armour;
    Shoe shoe;

    WeaponType weaponType;
    Weapon weapon;

    Skill* baseAttackAction;
    std::vector<PersonaBase*> personas;

    PersonaBase* curPersona;
};
