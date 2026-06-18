#pragma once
#include "core/geometry.h"
#include "core/structs.h"

struct CharacterFrameState
{
    Point2D<float> translate;
    float angle;
    float height;
};

class ICameraStrategy
{
  public:
    virtual ~ICameraStrategy() = default;
    virtual CameraPosition update(const CharacterFrameState& character) = 0;
    virtual bool allowsManualRotation() const
    {
        return true;
    }
    // normalized X/Z direction the camera is currently looking, used as the basis for movement input
    virtual Point2D<float> getForwardDirection() const = 0;
};
