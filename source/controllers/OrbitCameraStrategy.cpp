#include "OrbitCameraStrategy.h"
#include "math.h"

CameraPosition OrbitCameraStrategy::update(const CharacterFrameState& character)
{
    lastAngle = character.angle;

    CameraPosition camPos;

    camPos.cameraX = character.translate.x + (sin(character.angle) * distance);
    camPos.cameraY = 0.6f + character.height;
    camPos.cameraZ = character.translate.z - (cos(character.angle) * distance);

    camPos.targetX = character.translate.x - (sin(character.angle) * lookAhead);
    camPos.targetY = 0.1f + character.height;
    camPos.targetZ = character.translate.z + (cos(character.angle) * lookAhead);

    camPos.upX = 0.0f;
    camPos.upY = 1.0f + character.height;
    camPos.upZ = 0.0f;

    return camPos;
}

Point2D<float> OrbitCameraStrategy::getForwardDirection() const
{
    return Point2D<float>(-sin(lastAngle), cos(lastAngle));
}
