#include "FixedCameraStrategy.h"
#include "math.h"

CameraPosition FixedCameraStrategy::update(const CharacterFrameState& character)
{
    target.x += (character.translate.x - target.x) * smoothing;
    target.z += (character.translate.z - target.z) * smoothing;

    float offsetX = target.x - origin.x;
    float offsetZ = target.z - origin.z;
    float horizontalDistance = sqrtf(offsetX * offsetX + offsetZ * offsetZ);

    if (horizontalDistance < minHorizontalDistance)
    {
        if (horizontalDistance < 0.0001f)
        {
            // no real direction to push toward, just nudge along Z so we don't divide by zero
            offsetX = 0.0f;
            offsetZ = 1.0f;
            horizontalDistance = 1.0f;
        }

        float scale = minHorizontalDistance / horizontalDistance;
        target.x = origin.x + offsetX * scale;
        target.z = origin.z + offsetZ * scale;
    }

    CameraPosition camPos;

    camPos.cameraX = origin.x;
    camPos.cameraY = height;
    camPos.cameraZ = origin.z;

    camPos.targetX = target.x;
    camPos.targetY = 0.1f + character.height;
    camPos.targetZ = target.z;

    camPos.upX = 0.0f;
    camPos.upY = 1.0f + character.height;
    camPos.upZ = 0.0f;

    return camPos;
}

Point2D<float> FixedCameraStrategy::getForwardDirection() const
{
    Point2D<float> direction = target - origin;
    float length = sqrtf(direction.x * direction.x + direction.z * direction.z);

    if (length < 0.0001f)
    {
        return Point2D<float>(0.0f, 1.0f);
    }

    return Point2D<float>(direction.x / length, direction.z / length);
}
