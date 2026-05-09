#include "TargetAndAttackEnemy.h"

bool TargetAndAttackActionEnemy::update(u32 *keys)
{
    if (*keys & KEY_LEFT)
    {
        iprintf("CurTar: ");
        iprintf(enemies->at(*targetIndex)->name.c_str());
        iprintf("\n");
    }
    else if (*keys & KEY_RIGHT)
    {
        iprintf("CurTar: ");
        iprintf(enemies->at(*targetIndex)->name.c_str());
        iprintf("\n");
    }

    if (*keys & KEY_A)
    {
        iprintf("AttackSkill: ");
        iprintf(enemies->at(*targetIndex)->name.c_str());
        enemies->at(*targetIndex)->hp -= attack->calculateDamage(&player->ma, &player->st, &enemies->at(*targetIndex)->en, &player->lv, &enemies->at(*targetIndex)->lv);
        iprintf("\n");

        char str[50];
        std::sprintf(str, "remaing Enemy hp: %d \n", enemies->at(*targetIndex)->hp);
        iprintf(str);
        iprintf("\n");

        if (enemies->at(*targetIndex)->hp <= 0)
        {
            enemies->erase(enemies->begin() + *targetIndex);
        }

        return true;
    }

    return false;
}