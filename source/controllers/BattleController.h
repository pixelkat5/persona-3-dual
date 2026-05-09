#pragma once

#include <stdio.h>
#include <nds.h>
#include <stdio.h>
#include <vector>
#include <array>

#include "./battleActions/ActionBase.h"
#include "./battleActions/AttackAction.h"
#include "./battleActions/Guard.h"
#include "./battleActions/Persona.h"
#include "./battleActions/party/Player.h"
#include "./battleActions/enemies/Enemy.h"
#include "./battleActions/UpdateIndex.h"

class BattleController
{
private:     
    u32 index = 0;               
    u32 counter = 0;
    bool active = false;
    bool isEnemeyTurn = false;
    UpdateIndex updateIndex;              

    Player* player;
    std::vector<Enemy*> *enemies;

    AttackAction attack;
    Guard guard;
    Persona persona;

    std::array<ActionBase *, 3> actions = {nullptr};

    void enemyTurn();
public:
    bool isActive() {return active;};
    void execute();
    void update(u32 keys);
    void exit();
    BattleController(Player *iPlayer, std::vector<Enemy*> *iEnemies);
    ~BattleController() {}
};
