#include <nds.h>
#include "core/globals.h"
#include "PauseMenuComponent.h"

// sfx
#include "soundbank.h"
// backgrounds
#include "bgYukiClose.h"
#include "bgGuard.h"
#include "bgAkihiko.h"
#include "bgYuki.h"
// fonts
#include "rodin_sheet.h"
#include "rodin_sheet_map.h"
// dialogue
#include "dialogue/demo_dialogue.h"

DialogueController dialogueCtrl;

void PauseMenuComponent::setBgLoaders()
{
    bgLoaders[0] = [](int slot)
    {
        dmaCopy(bgGuardTiles, bgGetGfxPtr(slot), bgGuardTilesLen);
        dmaCopy(bgGuardMap, bgGetMapPtr(slot), bgGuardMapLen);
        vramSetBankH(VRAM_H_LCD);
        dmaCopy(bgGuardPal, &VRAM_H_EXT_PALETTE[0][0], bgGuardPalLen);
        vramSetBankH(VRAM_H_SUB_BG_EXT_PALETTE);
        bgShow(slot);
    };
    bgLoaders[1] = [](int slot)
    {
        dmaCopy(bgYukiTiles, bgGetGfxPtr(slot), bgYukiTilesLen);
        dmaCopy(bgYukiMap, bgGetMapPtr(slot), bgYukiMapLen);
        vramSetBankH(VRAM_H_LCD);
        dmaCopy(bgYukiPal, &VRAM_H_EXT_PALETTE[0][0], bgYukiPalLen);
        vramSetBankH(VRAM_H_SUB_BG_EXT_PALETTE);
        bgShow(slot);
    };
    bgLoaders[2] = [](int slot)
    {
        dmaCopy(bgAkihikoTiles, bgGetGfxPtr(slot), bgAkihikoTilesLen);
        dmaCopy(bgAkihikoMap, bgGetMapPtr(slot), bgAkihikoMapLen);
        vramSetBankH(VRAM_H_LCD);
        dmaCopy(bgAkihikoPal, &VRAM_H_EXT_PALETTE[0][0], bgAkihikoPalLen);
        vramSetBankH(VRAM_H_SUB_BG_EXT_PALETTE);
        bgShow(slot);
    };
    bgLoaders[3] = [](int slot)
    {
        dmaCopy(bgYukiCloseTiles, bgGetGfxPtr(slot), bgYukiCloseTilesLen);
        dmaCopy(bgYukiCloseMap, bgGetMapPtr(slot), bgYukiCloseMapLen);
        vramSetBankH(VRAM_H_LCD);
        dmaCopy(bgYukiClosePal, &VRAM_H_EXT_PALETTE[0][0], bgYukiClosePalLen);
        vramSetBankH(VRAM_H_SUB_BG_EXT_PALETTE);
        bgShow(slot);
    };
}

void PauseMenuComponent::loadBg(int bgIndex)
{
    if (bgIndex >= 0 && bgIndex < 4 && bgLoaders[bgIndex])
        bgLoaders[bgIndex](bgSlot);
}

void PauseMenuComponent::cancelSFX()
{
    musicCtrl.stopSFX(sfxMenuHandle);
    musicCtrl.stopSFX(sfxSelectHandle);
    musicCtrl.stopSFX(sfxCancelHandle);
}

void PauseMenuComponent::init(int iBgSlot, int iFontBgSlot, bool* isActive)
{
    musicCtrl.loadSFX(SFX_MENU);
    musicCtrl.loadSFX(SFX_SELECT);
    musicCtrl.loadSFX(SFX_CANCEL);

    options = menuOptions;
    optionCount = MENU_OPTIONS;
    selectedOption = 0;

    bgSlot = iBgSlot;
    fontBgSlot = iFontBgSlot;
    setBgLoaders();

    isActivePtr = isActive;
    nextViewState = ViewState::KEEP_CURRENT;

    font.init(
        fontBgSlot,
        rodin_sheetTiles,   rodin_sheetTilesLen,
        rodin_sheetPal,     rodin_sheetPalLen,
        rodin_sheet_CHAR_MAP, RODIN_SHEET_CHAR_COUNT,
        32,
        RODIN_SHEET_CELL_W / 8,   // cellTilesW = 3
        RODIN_SHEET_CELL_H / 8,   // cellTilesH = 3
        (384 / 8)                 // sheetCols in tiles = 48
    );
}

ViewState PauseMenuComponent::update(int keys)
{
    if (dialogueCtrl.isActive()) {
        dialogueCtrl.update(keys);
        return ViewState::KEEP_CURRENT;
    }

    if (keys & KEY_DOWN)
    {
        sfxMenuHandle = musicCtrl.playSFX(SFX_MENU, 255, 128);
        selectedOption = (selectedOption + 1) % optionCount;
    }
    else if (keys & KEY_UP)
    {
        sfxMenuHandle = musicCtrl.playSFX(SFX_MENU, 255, 128);
        selectedOption = (selectedOption + optionCount - 1) % optionCount;
    }
    else if (keys & KEY_A)
    {
        cancelSFX();
        sfxSelectHandle = musicCtrl.playSFX(SFX_SELECT, 255, 128);

        PauseState currentState = {options, optionCount, selectedOption};

        if (options[selectedOption].onSelect != nullptr)
        {
            ViewState result = (this->*(options[selectedOption].onSelect))();
            if (result != ViewState::KEEP_CURRENT) {
                nextViewState = result;
                *isActivePtr = false;
            } else if (dialogueCtrl.isActive()) {
                return ViewState::KEEP_CURRENT;
            }
            if (options != currentState.options)
            {
                prevOptions.push(currentState);
            }
        }
    }

    if (keys & KEY_B)
    {
        cancelSFX();
        musicCtrl.playSFX(SFX_CANCEL, 255, 128);
        selectedOption = 0;

        if (!prevOptions.empty())
        {
            PauseState prevState = prevOptions.top();
            prevOptions.pop();
            options = prevState.options;
            optionCount = prevState.optionCount;
            selectedOption = prevState.selectedOption;
        } else {
            *isActivePtr = false;
        }
    }

    bgShow(fontBgSlot);
    font.clear();
    for (int option = 0; option < optionCount; option++)
    {
        int tileY = option * (RODIN_SHEET_CELL_H / 8);
        font.draw(0, tileY, option == selectedOption ? "> " : "  ");
        font.draw(2, tileY, options[option].name);
    }

    int bgIndex = options[selectedOption].bgIndex;
    if (bgIndex != -1)
    {
        loadBg(bgIndex);
        bgShow(bgSlot);
    }
    else
    {
        bgHide(bgSlot);
    }

    ViewState viewState = nextViewState;
    nextViewState = ViewState::KEEP_CURRENT;
    return viewState;
}

// OPTION HANDLERS
ViewState PauseMenuComponent::openDebugMenu()
{
    selectedOption = 0; options = debugOptions; optionCount = DEBUG_OPTIONS;
    return ViewState::KEEP_CURRENT;
}
ViewState PauseMenuComponent::openSkillMenu()
{
    selectedOption = 0; options = skillOptions; optionCount = SKILL_OPTIONS;
    return ViewState::KEEP_CURRENT;
}
ViewState PauseMenuComponent::openItemMenu()
{
    selectedOption = 0; options = itemOptions; optionCount = ITEM_OPTIONS;
    return ViewState::KEEP_CURRENT;
}
ViewState PauseMenuComponent::openPersonaMenu()
{
    selectedOption = 0; options = personaOptions; optionCount = PERSONA_OPTIONS;
    return ViewState::KEEP_CURRENT;
}
ViewState PauseMenuComponent::openEquipMenu()
{
    selectedOption = 0; options = equipOptions; optionCount = EQUIP_OPTIONS;
    return ViewState::KEEP_CURRENT;
}
ViewState PauseMenuComponent::openStatusMenu()
{
    selectedOption = 0; options = statsOptions; optionCount = STATS_OPTIONS;
    return ViewState::KEEP_CURRENT;
}
ViewState PauseMenuComponent::openSLinkMenu()
{
    selectedOption = 0; options = sLinkOptions; optionCount = S_LINK_OPTIONS;
    return ViewState::KEEP_CURRENT;
}
ViewState PauseMenuComponent::openSystemMenu()
{
    selectedOption = 0; options = systemOptions; optionCount = SYSTEM_OPTIONS;
    return ViewState::KEEP_CURRENT;
}

ViewState PauseMenuComponent::debugOptionSelected()
{
    ViewState selectedView;
    switch (selectedOption)
    {
        case DISCLAIMER_VIEW:      selectedView = ViewState::DISCLAIMER;        break;
        case INTRO_VIDEO_VIEW:     selectedView = ViewState::INTRO_VIDEO;       break;
        case INTRO_VIEW:           selectedView = ViewState::INTRO;             break;
        case MAIN_MENU_VIEW:       selectedView = ViewState::MAIN_MENU;         break;
        case IWATODAI_DORM_VIEW:   selectedView = ViewState::IWATODAI_DORM;     break;
        case IWATODAI_STREETS_VIEW:selectedView = ViewState::IWATODAI_STREETS;  break;
        case DEBUG_DIALOGUE:
            consoleClear();
            demo_yuki_guard_argument_load();
            dialogueCtrl.setLoader(demo_yuki_guard_argument_load_bg);
            dialogueCtrl.start(demo_yuki_guard_argument_first());
            selectedView = ViewState::KEEP_CURRENT;
            break;
        case TOGGLE_BILLBOARDS:
            enableBillboards = !enableBillboards;
            *isActivePtr = false;
            selectedView = ViewState::KEEP_CURRENT;
            break;
        default: selectedView = ViewState::KEEP_CURRENT;
    }
    return selectedView;
}

ViewState PauseMenuComponent::skillOptionSelected()
{
    selectedOption = 0; options = skills; optionCount = SKILLS;
    return ViewState::KEEP_CURRENT;
}
ViewState PauseMenuComponent::itemOptionSelected()    { return ViewState::KEEP_CURRENT; }
ViewState PauseMenuComponent::equipOptionSelected()   { return ViewState::KEEP_CURRENT; }
ViewState PauseMenuComponent::personaOptionSelected() { return ViewState::KEEP_CURRENT; }
ViewState PauseMenuComponent::statsOptionSelected()   { return ViewState::KEEP_CURRENT; }
ViewState PauseMenuComponent::sLinkOptionSelected()   { return ViewState::KEEP_CURRENT; }
ViewState PauseMenuComponent::systemOptionSelected()  { return ViewState::KEEP_CURRENT; }

ViewState PauseMenuComponent::openCharacterAnimMenu() {
    selectedOption = 0;
    options = characterAnimOptions;
    optionCount = CHARACTER_ANIM_OPTIONS;
    return ViewState::KEEP_CURRENT;
}

ViewState PauseMenuComponent::characterAnimOptionSelected() {
    return ViewState::KEEP_CURRENT;
}
