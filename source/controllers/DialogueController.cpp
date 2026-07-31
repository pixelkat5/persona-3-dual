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
    doRenderOptions = false;
    optionCount = 0;
    selectedOption = 0;
    textCtrl->clearScreen(textVideoBufferSub);
    renderAnimFrame();
}

// print the text up to animIndex using a precision field
void DialogueController::renderAnimFrame()
{
    textCtrl->drawText(current->characterName, font, textVideoBufferSub, 32, 115, TextColor::White);
    textCtrl->appearText(current->text, font, textVideoBufferSub, 0, 130, TextColor::White);
}

void DialogueController::renderOptions()
{
    // reprint the complete line then list choices below it
    textCtrl->clearScreen(textVideoBufferSub);
    textCtrl->drawText(current->characterName, font, textVideoBufferSub, 32, 115, TextColor::White);
    textCtrl->drawText(current->text, font, textVideoBufferSub, 0, 130, TextColor::White);
    //TODO: The options are currently drawn outside the textbox, do we want to add an extra overlay for them similar to the actual game?
    for (int i = 0; i < optionCount; i++)
    {
        if (i == selectedOption)
            textCtrl->drawText(current->selections[i].text, font, textVideoBufferSub, 128, 50 + i * 8, TextColor::Blue);
        else
            textCtrl->drawText(
                current->selections[i].text, font, textVideoBufferSub, 128, 50 + i * 8, TextColor::White);
    }
}

void DialogueController::start(Dialogue* firstLine, Font* font, uint16_t* textVideoBufferSub)
{
    this->textVideoBufferSub = textVideoBufferSub;
    this->font = font;
    loadedImageId = -1; // force a bg load for the very first line
    prevKeys = 0;
    advanceTo(firstLine);
    active = true;
}

void DialogueController::exit()
{
    textCtrl->clearScreen(textVideoBufferSub);
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
    if (!textCtrl->appearTextDone())
    {
        textCtrl->update();
        // swap the background exactly once per dialogue line, on the first
        // frame, and only if the image has actually changed
        if (animIndex == 0 && bgLoader && current->imageId != loadedImageId)
        {
            bgLoader(current->imageId);
            loadedImageId = current->imageId;
        }
        else
        {
            optionCount = (int)current->selections.size();

            if (optionCount > 0)
            {
                // render the full text + option list now that animation ended
                doRenderOptions = true;
            }
        }
    }
    else if (doRenderOptions)
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
        if (textCtrl->appearTextDone())
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
        }
        if (pressed & KEY_A)
        {
            if (!textCtrl->appearTextDone())
            {
                textCtrl->appearTextSkip();
                return;
            }
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
            if (!textCtrl->appearTextDone())
            {
                textCtrl->appearTextSkip();
                return;
            }
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
            if (!textCtrl->appearTextDone())
            {
                textCtrl->appearTextSkip();
                return;
            }
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
