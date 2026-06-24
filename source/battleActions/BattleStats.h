#pragma once
#include "Element.h"
#include "armours/Armour.h"
#include <nds.h>

struct BattleStats
{
    u32 st;
    u32 ma;
    u32 en;
    u32 ag;
    u32 lu;

    enum Affinity
    {
        Neutral,
        Weak,
        Resist,
        Null,
        Absorb,
        Repel
    };

    Affinity affinities[10] = {
        Neutral,
        Neutral,
        Neutral,
        Neutral,
        Neutral,
        Neutral,
        Neutral,
        Neutral,
        Neutral,
        Neutral // almighty
    };

    BattleStats* getBattleStats()
    {
        return this;
    };
    // add afinites
};
