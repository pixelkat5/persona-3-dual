#include "Player.h"

bool Player::actorCanUse(ActionBase* action)
{
    return true;
}

void Player::onDead(BattleResult& battleResult)
{
    if (hp > 0)
        return;

    battleResult.playerDied = true;
}
