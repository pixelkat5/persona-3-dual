#pragma once
#include <stdint.h>
#include "core/geometry.h"

// TODO: move this to globals?
enum class TileType {
    NO_COLLISION = 0,   // walkable area
    COLLISION = 1,      // non-walkable area. Legacy = 5
    SAVE = 2,           // save interaction point
    PREV_SCENE = 3,     // load last location
    NEXT_SCENE = 4,     // load next location
    CHARACTER_Akihiko = 100
};

struct cameraPosition {
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

struct characterPosition {
    float x;
    float z;
    float angle;
    float facingAngle;
};

class CharacterController {
    public:
        // 3D environment
        const int mapWidth;
        const int mapHeight;
        const uint8_t* collisionMap;

        // world
        const float tileSize;
        const float worldOffsetX;
        const float worldOffsetZ;
        const Point2D<float> characterSize;

        // movement and viewpoint
        const float speed;
        const float angleIncrement;
        const float distance; 
        const float lookAhead;

        // translation (mutable)
        float angle = 0.0;
        Point2D<float> characterTranslate = Point2D<float>(0.0, 0.0);
        // float translateX = 0.0;
       // float translateZ = 0.0;
        float characterFacingAngle = 0.0f;

        CharacterController(
            int iMapWidth, int iMapHeight, const uint8_t* iCollisionMap, 
            float iTileSize, float iWorldOffsetX, float iWorldOffsetZ, Point2D<float> iCharacterSize,
            float iSpeed, float iAngleIncrement, float iDistance, float iLookAhead,
            float iAngle, Point2D<float> iCharacterTranslate, float iCharacterFacingAngle
        ) : 
            mapWidth(iMapWidth),
            mapHeight(iMapHeight),
            collisionMap(iCollisionMap),
            tileSize(iTileSize),
            worldOffsetX(iWorldOffsetX),
            worldOffsetZ(iWorldOffsetZ),
            characterSize(iCharacterSize),
            speed(iSpeed),
            angleIncrement(iAngleIncrement),
            distance(iDistance),
            lookAhead(iLookAhead)
        {
            // set inital position
            angle = iAngle;
            characterTranslate = iCharacterTranslate;
            characterFacingAngle = iCharacterFacingAngle;
        };

        cameraPosition update(u32 keys);
        characterPosition isCharacterAt();
        TileType isTileAt();
    
    private:
        TileType isTileAt(int tileX, int TileY);
        bool isTileWalkable(float worldX, float worldZ);
};