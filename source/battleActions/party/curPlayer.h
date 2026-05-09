#pragma once
#include <nds.h>
#include <string>
#include "../skills/AttackSkill.h"
#include "../skills/Agi.h"
#include "../skills/Bufu.h"
#include "../skills/Slash_Attack.h"
#include "Player.h"

/*
St	Represents strength and physical damage.
Ma	Represents magic and magical damage.
En	Represents endurance, which determines your defense and how much damage you can take.
Ag	Represents agility, which determines your place in the turn order.
Lu	Represents luck, which is taken into account when using certain skills involving status afflictions or insta-death abilities.*/

// TODO: since these will be scalled with level etc this abosultey needs to be in some gloabl state
//Stats are p3f level 2 makoto + orpehus
struct curPlayer : Player
{
    Slash_Attack slash_Attack;
    Agi agi;
    Bufu bufu;
    AttackSkill *myAttcking[2];

    curPlayer()
    {
        myAttcking[0] = &agi;
        myAttcking[1] = &bufu;
        attackCount = 2;

        name = "Makoto";
        hp = 720;
        sp = 49;
        lv = 2;
        st = 3;
        ma = 3;
        en = 2;
        ag = 3;
        lu = 2;
        baseAttackAction = &slash_Attack;
        attackSkill = myAttcking;
        // add resistances and arcana in the future
        // ...
    }
    
};