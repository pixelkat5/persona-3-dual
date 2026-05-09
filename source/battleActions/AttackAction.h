#pragma once
#include "ActionBase.h"
#include "enemies/Enemy.h"
#include "party/Player.h"
#include <stdio.h>
#include <vector>
#include "UpdateIndex.h"
#include "TargetAndAttackEnemy.h"

struct AttackAction : ActionBase
{
    UpdateIndex updateIndex;
    std::vector<Enemy *> *enemies;
    Player *player;
    TargetAndAttackActionEnemy *targetAndAttackActionEnemy;

    AttackAction(std::vector<Enemy *> *iEnemies, Player *iPlayer) : enemies(iEnemies), player(iPlayer)
    {
        name = "AttackAction";
        // TODO: dont forget to clear in the future
        targetAndAttackActionEnemy = new TargetAndAttackActionEnemy(player->baseAttackAction, enemies, player, &targetIndex);
    }

    void execute() override;
    bool update() override;
};
