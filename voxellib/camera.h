#ifndef VOXELS_CAMERA_H
#define VOXELS_CAMERA_H


#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/quaternion.hpp"

const double kCameraTurnSpeed = 0.0001;
const float kCameraMoveSpeed = 10.0f;

class CameraState {
public:
    CameraState() = default;

    inline const glm::mat4 &transform() const { return transform_; }

private:
    glm::vec3 position_;
    glm::quat rotation_;

    glm::mat4 transform_;

    glm::vec3 forward_, right_;

    /**
     * Updates state.transform from rotation and position
     */
    void update_transform();

    /**
     * Updates forward and right from state.rotation
     */
    void update_directions();

    friend class Camera;

};

class Camera {
public:

    Camera(glm::vec3 start_pos, glm::vec3 start_dir);

    Camera() : Camera(glm::vec3(0.f), glm::vec3(1.f, 0.f, 0.f)) {};

    void rotate(int screen_x, int screen_y);

    /**
     * @return Copy of state before tick
     */
    CameraState tick(double dt);

    CameraState interpolate_from(const CameraState &from, double alpha) const;

private:
    double yaw_, pitch_;
    CameraState state_;

    /**
     * Updates orientation from pitch and yaw
     */
    void update_orientation();
};


#endif
