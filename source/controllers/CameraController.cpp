#include "CameraController.h"
#include <math.h>
#include <nds/arm9/trig_lut.h>

static const float RAD_TO_LIBNDS = 32768.0f / (2.0f * 3.14159265f);

static inline float hw_sinf(float r)
{
    return sinLerp((s16)(r * RAD_TO_LIBNDS)) / 4096.0f;
}
static inline float hw_cosf(float r)
{
    return cosLerp((s16)(r * RAD_TO_LIBNDS)) / 4096.0f;
}

void CameraController::configure(const CameraConfig& config)
{
    mode = config.mode;
    currentPos = config.eye;
    targetPos = config.target;
    angle = config.initialAngle;
    distance = config.distance;
    height = config.height;
    lookAhead = config.lookAhead;
    angleIncrement = config.angleIncrement;
}

void CameraController::setMode(CameraMode newMode)
{
    mode = newMode;
    if (newMode == CameraMode::Path)
    {
        pathFrame = 0;
        pathKeyIndex = 0;
        pathDone = false;
    }
    if (newMode == CameraMode::Free)
    {
        freeInitialised = false;
    }
}

void CameraController::setPath(const CameraPath* p)
{
    path = p;
    pathFrame = 0;
    pathKeyIndex = 0;
    pathDone = false;
}

float CameraController::getMovementAngle(const CharacterPosition& charPos) const
{
    switch (mode)
    {
    case CameraMode::CCTV:
    case CameraMode::Static:
        return atan2f(currentPos.x - charPos.x, charPos.z - currentPos.z);
    default:
        return angle;
    }
}

CameraPosition CameraController::update(u32 keys, const CharacterPosition& charPos)
{
    CameraPosition cam = {};
    cam.up.y = 1.0f;

    switch (mode)
    {
    case CameraMode::Static:
    {
        cam.eye.x = currentPos.x;
        cam.eye.y = currentPos.y;
        cam.eye.z = currentPos.z;
        cam.target.x = targetPos.x;
        cam.target.y = targetPos.y;
        cam.target.z = targetPos.z;
        break;
    }

    case CameraMode::CCTV:
    {
        cam.eye.x = currentPos.x;
        cam.eye.y = currentPos.y;
        cam.eye.z = currentPos.z;
        cam.target.x = charPos.x;
        cam.target.y = charPos.y;
        cam.target.z = charPos.z;
        break;
    }

    case CameraMode::Follow:
    {
        if (keys & KEY_L)
            angle -= angleIncrement;
        if (keys & KEY_R)
            angle += angleIncrement;

        cam.eye.x = charPos.x + hw_sinf(angle) * distance;
        cam.eye.y = charPos.y + height;
        cam.eye.z = charPos.z - hw_cosf(angle) * distance;

        cam.target.x = charPos.x - hw_sinf(angle) * lookAhead;
        cam.target.y = charPos.y + 0.1f;
        cam.target.z = charPos.z + hw_cosf(angle) * lookAhead;
        break;
    }

    case CameraMode::Free:
    {
        if (!freeInitialised)
        {
            currentPos.x = charPos.x;
            currentPos.y = charPos.y + height;
            currentPos.z = charPos.z;
            freeInitialised = true;
        }

        if (keys & KEY_L)
            angle -= angleIncrement;
        if (keys & KEY_R)
            angle += angleIncrement;

        const float fwdX = -hw_sinf(angle) * freeCameraSpeed;
        const float fwdZ = hw_cosf(angle) * freeCameraSpeed;

        if (keys & KEY_UP)
        {
            currentPos.x += fwdX;
            currentPos.z += fwdZ;
        }
        if (keys & KEY_DOWN)
        {
            currentPos.x -= fwdX;
            currentPos.z -= fwdZ;
        }
        if (keys & KEY_RIGHT)
        {
            currentPos.x -= fwdZ;
            currentPos.z += fwdX;
        }
        if (keys & KEY_LEFT)
        {
            currentPos.x += fwdZ;
            currentPos.z -= fwdX;
        }

        cam.eye.x = currentPos.x;
        cam.eye.y = currentPos.y;
        cam.eye.z = currentPos.z;
        cam.target.x = currentPos.x - hw_sinf(angle);
        cam.target.y = currentPos.y;
        cam.target.z = currentPos.z + hw_cosf(angle);
        break;
    }

    case CameraMode::Path:
    {
        if (!path || path->keyframes.size() < 2)
            break;

        pathFrame++;

        while (pathKeyIndex + 2 < static_cast<int>(path->keyframes.size()) &&
               pathFrame >= path->keyframes[pathKeyIndex + 1].time)
        {
            pathKeyIndex++;
        }

        const CameraKeyframe& kf0 = path->keyframes[pathKeyIndex];
        const CameraKeyframe& kf1 = path->keyframes[pathKeyIndex + 1];

        if (pathFrame >= kf1.time && pathKeyIndex + 2 >= static_cast<int>(path->keyframes.size()))
        {
            pathDone = true;
            mode = CameraMode::Follow;
            cam.eye.x = kf1.eye.x;
            cam.eye.y = kf1.eye.y;
            cam.eye.z = kf1.eye.z;
            cam.target.x = kf1.target.x;
            cam.target.y = kf1.target.y;
            cam.target.z = kf1.target.z;
            break;
        }

        int span = kf1.time - kf0.time;
        float t = (span > 0) ? static_cast<float>(pathFrame - kf0.time) / static_cast<float>(span) : 1.0f;

        cam.eye.x = kf0.eye.x + (kf1.eye.x - kf0.eye.x) * t;
        cam.eye.y = kf0.eye.y + (kf1.eye.y - kf0.eye.y) * t;
        cam.eye.z = kf0.eye.z + (kf1.eye.z - kf0.eye.z) * t;
        cam.target.x = kf0.target.x + (kf1.target.x - kf0.target.x) * t;
        cam.target.y = kf0.target.y + (kf1.target.y - kf0.target.y) * t;
        cam.target.z = kf0.target.z + (kf1.target.z - kf0.target.z) * t;
        break;
    }

    default:
        break;
    }

    return cam;
}
