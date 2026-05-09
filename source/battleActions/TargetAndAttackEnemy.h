#pragma once 
#include <nds.h>
#include <stdio.h> 
#include <vector>

#include "skills/AttackSkill.h"
#include "enemies/Enemy.h"
#include "party/Player.h"

struct TargetAndAttackActionEnemy
{
    Player* player;
    AttackSkill *attack;
    std::vector<Enemy*>* enemies;
    u32 *targetIndex; 

    TargetAndAttackActionEnemy(AttackSkill* iAttackAction, std::vector<Enemy*>* iEnemies, Player *iPlayer, u32 *iTargetIndex) :
    attack(iAttackAction), enemies(iEnemies), targetIndex(iTargetIndex), player(iPlayer) {}

    bool update(u32* keys);
};
