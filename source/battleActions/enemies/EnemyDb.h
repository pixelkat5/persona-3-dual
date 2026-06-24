#pragma once
#include "Enemy.h"

struct EnemyDb
{
    static Enemy mercilessMaya;
    static Enemy cowardlyMaya;

    static void Initialize();

  private:
    static Skill* mercilessMayaSkills[1];
    static Skill* cowardlyMayaSkills[1];
};
