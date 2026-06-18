#pragma once
#include "controllers/ICameraStrategy.h"

class OrbitCameraStrategy : public ICameraStrategy
{
  public:
    OrbitCameraStrategy(float iDistance, float iLookAhead) : distance(iDistance), lookAhead(iLookAhead)
    {
    }

    CameraPosition update(const CharacterFrameState& character) override;
    Point2D<float> getForwardDirection() const override;

  private:
    const float distance;
    const float lookAhead;
    float lastAngle = 0.0f;
};
