#pragma once

#include "core/geometry.h"
#include "core/structs.h"
#include <nds.h>

// !Todo replace floats with fixed point math for camera position and target position.

/**
 * @brief Controls how the camera behaves each frame.
 *
 * - Free   : first-person fly cam, d-pad moves, L/R rotates.
 * - Static : fixed eye and target, ignores all input.
 * - CCTV   : fixed eye position, target tracks the character.
 * - Follow : orbits behind the character, L/R adjusts orbit angle.
 * - Path   : plays back a @ref CameraPath keyframe sequence, then
 *            automatically returns to Follow when complete.
 */
enum class CameraMode
{
    Free,
    Static,
    CCTV,
    Follow,
    Path
};

/**
 * @brief All parameters needed to configure a @ref CameraController in one call.
 *
 * Set the relevant fields for the chosen mode and pass to
 * CameraController::configure(). Fields irrelevant to the chosen mode are
 * ignored.
 */
struct CameraConfig
{
    CameraMode mode = CameraMode::Follow;

    // Static / CCTV — fixed eye position
    Vec3<float> eye = {};
    Vec3<float> target = {}; ///< Look-at point. Used by Static only.

    // Follow / Free
    float initialAngle = 0.0f;    ///< Starting orbit angle in radians.
    float distance = 1.5f;        ///< Distance from character to camera eye.
    float height = 0.6f;          ///< Eye height above the character origin.
    float lookAhead = 0.5f;       ///< Distance ahead of the character for the look-at point.
    float angleIncrement = 0.05f; ///< Radians rotated per frame on L/R input.
};

/**
 * @brief A single keyframe in a camera path.
 *
 * @see CameraPath
 */
struct CameraKeyframe
{
    int time;           ///< Frame index at which this keyframe is reached.
    Vec3<float> eye;    ///< Camera eye position.
    Vec3<float> target; ///< Look-at position.
};

/**
 * @brief An ordered list of keyframes defining a camera animation.
 *
 * The camera interpolates linearly between consecutive keyframes.
 * On completion the @ref CameraController switches to Follow mode.
 */
struct CameraPath
{
    std::vector<CameraKeyframe> keyframes;
};

/**
 * @brief Output of @ref CameraController::update(), consumed by gluLookAt().
 */
struct CameraPosition
{
    Vec3<float> eye;    ///< Camera eye position.
    Vec3<float> target; ///< Look-at point.
    Vec3<float> up;     ///< Up vector (default 0,1,0).
};

/**
 * @brief Manages the camera for a 3D environment view.
 *
 * Owns all camera state (position, angle, path playback). Call configure()
 * once on room load, then update() every frame to get the gluLookAt arguments.
 *
 * @todo Replace float arithmetic with fixed-point (f32) for NDS performance.
 */
class CameraController
{
  public:
    /**
     * @brief Applies a full camera configuration in one call.
     *
     * Resets all tuning parameters and mode from @p config. Call this after
     * configureCameraController() sets up the room-specific @ref CameraConfig.
     *
     * @param config Camera configuration to apply.
     */
    void configure(const CameraConfig& config);

    /**
     * @brief Switches the active camera mode at runtime.
     * @param mode New camera mode.
     */
    void setMode(CameraMode mode);

    /**
     * @brief Sets the path used by @ref CameraMode::Path and rewinds playback.
     *
     * Call this before switching mode to Path. The pointer must remain valid
     * for the lifetime of the playback.
     *
     * @param path Pointer to the CameraPath to play. Must not be null.
     */
    void setPath(const CameraPath* path);

    /**
     * @brief Advances camera state and returns the resulting gluLookAt arguments.
     *
     * @param keys    NDS key bitmask from keysHeld().
     * @param charPos Current character world position.
     * @return Camera eye, look-at, and up vectors for the current frame.
     */
    CameraPosition update(u32 keys, const CharacterPosition& charPos);

    /** @brief Returns the current camera mode. */
    CameraMode getMode() const
    {
        return mode;
    }

    /** @brief Returns the current orbit angle in radians. */
    float getAngle() const
    {
        return angle;
    }

    /** @brief Returns true once the Path playback has reached its last keyframe. */
    bool isPathComplete() const
    {
        return pathDone;
    }

    /**
     * @brief Returns the angle that maps the UP key to "move away from camera".
     *
     * For Follow/Free returns the current orbit angle. For CCTV/Static
     * computes atan2 from the fixed eye position toward the character, so
     * movement direction stays correct regardless of where the camera is mounted.
     *
     * @param charPos Current character world position.
     * @return Angle in radians to pass to CharacterController::update().
     */
    float getMovementAngle(const CharacterPosition& charPos) const;

  private:
    CameraMode mode = CameraMode::Follow;

    Vec3<float> currentPos = {};
    Vec3<float> targetPos = {};

    float angle = 0.0f;
    float distance = 1.5f;
    float height = 0.6f;
    float lookAhead = 0.5f;
    float angleIncrement = 0.05f;
    float freeCameraSpeed = 0.02f;

    // Path playback state
    const CameraPath* path = nullptr;
    int pathFrame = 0;
    int pathKeyIndex = 0;
    bool pathDone = false;

    // Free mode state
    bool freeInitialised = false;
};
