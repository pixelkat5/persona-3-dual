#include "Persona.h"
#include <stdio.h>

void Persona::execute()
{
    inProgress = true;
    menuState = SelectSkill;

    while (inProgress)
    {
        scanKeys();

        u32 keys = keysDown();

        if (menuState == SelectSkill)
        {
            u32 skillCount = player->attackCount;

            updateIndex.update(keys, targetIndex, skillCount);

            if (keys & KEY_LEFT)
            {
                iprintf("Cur: ");
                iprintf(player->attackSkill[targetIndex]->name.c_str());
                iprintf("\n");
            }
            else if (keys & KEY_RIGHT)
            {
                iprintf("Cur: ");
                iprintf(player->attackSkill[targetIndex]->name.c_str());
                iprintf("\n");
            }

            if (keys & KEY_A)
            {
                iprintf("Sel: ");
                selectedSkill = player->attackSkill[targetIndex];
                iprintf(selectedSkill->name.c_str());
                iprintf("\n");

                menuState = SelectTarget;
            }

            if (keys & KEY_B)
            {
                inProgress = false;
            }
        } 
        else
        {
            
            menuState = SelectSkill;
        } 
    }
}

bool Persona::update(){return true;}