#pragma once
#include "../Element.h"
#include "SkillRace.h"
#include "SkillTarget.h"
#include "SkillType.h"
#include <nds.h>
#include <string>

struct Skill
{
    std::string name;
    s32 cost;
    Element element;
    u32 hitRate;
    u32 movePower;
    SkillType skillType;
    SkillRace skillRace;
    SkillTarget skillTarget;
};
