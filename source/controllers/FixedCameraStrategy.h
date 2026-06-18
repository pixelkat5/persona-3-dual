#pragma once
#include "controllers/ICameraStrategy.h"

class FixedCameraStrategy : public ICameraStrategy
{
  public:
    FixedCameraStrategy(Point2D<float> iOrigin,
                        float iHeight,
                        float iSmoothing,
                        Point2D<float> iInitialTarget,
                        float iMinHorizontalDistance = 0.45f)
        : origin(iOrigin), height(iHeight), smoothing(iSmoothing), target(iInitialTarget),
          minHorizontalDistance(iMinHorizontalDistance)
    {
    }

    CameraPosition update(const CharacterFrameState& character) override;
    bool allowsManualRotation() const override
    {
        return false;
    }
    Point2D<float> getForwardDirection() const override;

  private:
    const Point2D<float> origin;
    const float height;
    const float smoothing;
    Point2D<float> target;
    // the look target is never allowed to get closer than this to the camera's
    // horizontal position, otherwise the look angle gets steep enough to flip
    const float minHorizontalDistance;
};
