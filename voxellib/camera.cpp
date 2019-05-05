#include <SFML/Window/Keyboard.hpp>
#include "glm/simd/matrix.h"
#include "glm/gtx/compatibility.hpp"
#include "camera.h"
#include "util.h"

Camera::Camera(glm::vec3 start_pos, glm::vec3 start_dir) : state_() {
    state_.position_ = start_pos;
    state_.transform_ = glm::mat4(1.0); // TODO should be identity

    // yaw and pitch from vector
    glm::vec3 dir = glm::normalize(start_dir);
    yaw_ = atan2f(dir.z, dir.x);
    pitch_ = asinf(-dir.y);

    update_orientation();
}

void Camera::rotate(int screen_x, int screen_y) {
    yaw_ += screen_x * kCameraTurnSpeed;
    pitch_ = glm::clamp(pitch_ - (screen_y * kCameraTurnSpeed),
                        -glm::two_pi<double>(), glm::two_pi<double>());

    update_orientation();
}

CameraState Camera::tick(double dt) {
    int backwards = 0, right = 0;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        backwards -= 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        backwards += 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        right -= 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        right += 1;

    CameraState last_state = state_;
    if (backwards != 0 || right != 0) {
        state_.update_directions();

        glm::vec3 movement = (state_.forward_ * (backwards * kCameraMoveSpeed * (float) dt)) +
                             (state_.right_ * (right * kCameraMoveSpeed * (float) dt));

        state_.position_ += movement;
    }

    state_.update_transform();
    return last_state;
}

CameraState Camera::interpolate_from(const CameraState &from, double alpha) const {
    float lerp = alpha;
    auto copy = CameraState(state_);
    copy.position_ = glm::lerp(from.position_, state_.position_, lerp);
    copy.rotation_ = glm::lerp(from.rotation_, state_.rotation_, lerp);
    copy.update_transform();
    return copy;
}

void Camera::update_orientation() {
    glm::quat pitch = glm::quat(pitch_, glm::vec3(1.f, 0.f, 0.f)); // x axis up
    glm::quat yaw = glm::quat(yaw_, glm::vec3(0.f, 1.f, 0.f)); // y axis up

    glm::quat orientation = pitch * yaw;
    state_.rotation_ = glm::normalize(orientation);
}

void CameraState::update_transform() {
    glm::vec3 pos_inv = position_ * glm::vec3(-1, -1, -1);
    glm::mat4 translation = glm::translate(glm::mat4(1.0), pos_inv); // TODO identity?
    glm::mat4 rotation = glm::mat4_cast(rotation_);

    transform_ = rotation * translation;
}


void CameraState::update_directions() {
    glm::mat4 rotation = glm::inverse(glm::mat4_cast(rotation_));

    forward_ = rotation * glm::vec4(0.f, 0.f, 1.f, 0.f); // z axis up
    right_ = rotation * glm::vec4(1.f, 0.f, 0.f, 0.f); // x axis up
}

