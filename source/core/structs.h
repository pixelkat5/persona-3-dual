#pragma once
#include "core/enums.h"
#include <nds.h>
#include <string>
#include <vector>
class BaseMenu;

struct SpriteRegister
{
    int id;
    void* tiles;
    u32 tilesLen;
    void* pal;
    u32 palLen;
};

struct SpriteDBEntry
{
    SpriteType type;
    int id;
    const char* filename;
};

//a simple sprite structure
struct Sprite
{
    u16* gfx;
    SpriteSize size;
    SpriteColorFormat format;
    int rotationIndex;
    int paletteAlpha;
    int x;
    int y;
};

struct MenuOption
{
    const char* name;
    int bgIndex;
    ViewState (BaseMenu::*onSelect)();
};

struct MenuState
{
    MenuOption* options;
    int optionCount;
    int selectedOption;
    int startIndex;
};

// From AnimationController.h
struct Keyframe
{
    int time;
    s16 rotX, rotY, rotZ;
    s16 posX, posY, posZ;
};

struct AnimTrack
{
    std::vector<Keyframe> frames;
};

struct Animation
{
    int duration;
    std::vector<AnimTrack> nodeTracks;
};

struct SubList
{
    int texSlot;
    std::vector<u32> displayList;
    u32 displayListSize;
};

struct AnimNode
{
    int id;
    int parentId;
    std::vector<SubList> subLists;
    std::vector<int> children;
    v16 pivotX, pivotY, pivotZ;
};

// From CharacterController.h
struct CameraPosition
{
    float cameraX;
    float cameraY;
    float cameraZ;
    float targetX;
    float targetY;
    float targetZ;
    float upX;
    float upY;
    float upZ;
};

struct CharacterPosition
{
    float x;
    float z;
    float y; // height
    float angle;
    float facingAngle;
};

// From DialogueController.h
struct Dialogue;
struct DialogueSelection
{
    std::string text;
    bool isSelected;
    Dialogue* next;
};
struct Dialogue
{
    std::string characterName;
    std::string text;
    int imageId;
    Dialogue* prev;
    Dialogue* next;
    std::vector<DialogueSelection> selections;
};

struct Save
{
    char introVideoPath[128];
    char lastName[32];
    char firstName[32];
    bool femcMode;
} __attribute__((packed));

struct GraphicAsset
{
    void* tiles;
    u32 tilesLen;
    void* pal;
    u32 palLen;
    void* map;
    u32 mapLen;
};
