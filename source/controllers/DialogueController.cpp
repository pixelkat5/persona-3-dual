#include "DialogueController.h"
#include <nds.h>
#include <stdio.h>

DialogueController::DialogueController()
{
}

// transition to a new Dialogue node and reset animation state
void DialogueController::advanceTo(Dialogue* next)
{
    current = next;
    animIndex = 0;
    animDone = false;
    doRenderOptions = false;
    optionCount = 0;
    selectedOption = 0;
    consoleClear();
}

// print the text up to animIndex using a precision field
void DialogueController::renderAnimFrame()
{
    iprintf("\x1b[15;4H%s\n", current->characterName.c_str());
    iprintf("\x1b[17;0H%.*s\n", animIndex, current->text.c_str());
}

void DialogueController::renderOptions()
{
    // reprint the complete line then list choices below it
    consoleClear();
    iprintf("\x1b[15;4H%s\n", current->characterName.c_str());
    iprintf("\x1b[17;0H%s\n", current->text.c_str());
    for (int i = 0; i < optionCount; i++)
    {
        iprintf("%c %s\n", i == selectedOption ? '>' : ' ', current->selections[i].text.c_str());
    }
}

void DialogueController::start(Dialogue* firstLine)
{
    loadedImageId = -1; // force a bg load for the very first line
    prevKeys = 0;
    advanceTo(firstLine);
    active = true;
}

void DialogueController::exit()
{
    consoleClear();
    active = false;
}

void DialogueController::update(u32 keys)
{
    if (!active || current == nullptr)
    {
        active = false;
        return;
    }

    // animation
    if (!animDone)
    {
        // swap the background exactly once per dialogue line, on the first
        // frame, and only if the image has actually changed
        if (animIndex == 0 && bgLoader && current->imageId != loadedImageId)
        {
            bgLoader(current->imageId);
            loadedImageId = current->imageId;
        }

        if (animIndex <= (int)current->text.length())
        {
            renderAnimFrame();
            animIndex++;
        }
        else
        {
            animDone = true;
            optionCount = (int)current->selections.size();

            if (optionCount > 0)
            {
                // render the full text + option list now that animation ended
                doRenderOptions = true;
            }
        }
        return; // consume the frame (don't process input yet)
    }

    // deferred option render (first frame after animation)
    if (doRenderOptions)
    {
        renderOptions();
        doRenderOptions = false;
    }

    // input
    u32 pressed = keys & ~prevKeys;
    prevKeys = keys;

    if (pressed & KEY_START)
    {
        exit();
        return;
    }

    if (optionCount > 0)
    {
        // selection dialogue
        if (pressed & KEY_DOWN)
        {
            selectedOption = (selectedOption + 1) % optionCount;
            renderOptions();
        }
        else if (pressed & KEY_UP)
        {
            selectedOption = (selectedOption + optionCount - 1) % optionCount;
            renderOptions();
        }
        else if (pressed & KEY_A)
        {
            Dialogue* next = current->selections[selectedOption].next;
            if (next == nullptr)
            {
                exit();
                return;
            }
            advanceTo(next);
        }
    }
    else
    {
        // linear dialogue
        if (pressed & KEY_A)
        {
            Dialogue* next = current->next;
            if (next == nullptr)
            {
                exit();
                return;
            }
            advanceTo(next);
        }
        else if (pressed & KEY_B)
        {
            Dialogue* next = current->prev;
            if (next == nullptr)
            {
                exit();
                return;
            }
            advanceTo(next);
        }
    }
}
