#pragma once
#include <array>
#include <nds.h>
#include <string>

struct TurnResult
{
    bool hit = false;
    s32 hpDelta = 0;
    bool oneMore = false;
    std::string log;
};
