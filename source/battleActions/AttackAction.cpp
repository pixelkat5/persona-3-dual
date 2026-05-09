#include "AttackAction.h"
#include <stdio.h>
#include <string.h>

void AttackAction::execute()
{
    inProgress = true;
}

bool AttackAction::update()
{
    u32 keys = keysDown();
    u32 enemyCount = enemies->size();

    updateIndex.update(keys, targetIndex, enemyCount);

    bool madeAction = targetAndAttackActionEnemy->update(&keys);
    if (madeAction)
    {
        targetIndex = 0;
        inProgress = false;
    }

    if (keys & KEY_B)
    {
        inProgress = false;
    }

    return madeAction;
}