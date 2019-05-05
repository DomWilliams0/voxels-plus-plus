#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include "game.h"
#include "error.h"
#include "util.h"
#include "camera.h"

int Game::run() {
    // create window
    sf::ContextSettings settings;
    settings.majorVersion = 3;
    settings.minorVersion = 3;

    const int width = 800;
    const int height = 600;
    register_window_resize(width, height);

    sf::Window window(sf::VideoMode(width, height), "OpenGL", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);

    window_ = &window;

    // init renderer
    int ret;
    if ((ret = renderer_.init(&world_)) != kErrorSuccess)
        return ret;

    // TODO start at a less hard-coded position
//    camera_ = Camera(glm::vec3(-4.f, 3.f, 0.75f),
//                     glm::vec3(-0.3f, -0.5f, 1.f));
    CameraState last_camera_state{};


    // timestep
    sf::Clock clock;
    double t = 0;
    const int tps = 20;
    const double dt = 1.0 / tps;
    double now_s = clock.getElapsedTime().asSeconds();
    double acc = 0;

    sf::Event event{};
    while (window.isOpen()) {
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;

                case sf::Event::KeyReleased:
                    if (event.key.code == sf::Keyboard::Key::Escape)
                        window.close();
                    break;

                case sf::Event::Resized:
                    register_window_resize(event.size.width, event.size.height);
                    break;

                case sf::Event::MouseMoved:
                    if (mouse_grabbed_)
                        on_mouse_grab(event.mouseMove.x, event.mouseMove.y);
                    break;

                case sf::Event::MouseButtonPressed:
                    if (event.mouseButton.button == sf::Mouse::Button::Left)
                        grab_cursor(true);
                    break;

                case sf::Event::MouseButtonReleased:
                    if (event.mouseButton.button == sf::Mouse::Button::Left)
                        grab_cursor(false);
                    break;

                default:
                    break;
            }
        }

        // independent frame rate
        double new_now_s = clock.getElapsedTime().asSeconds();
        double frame_time = new_now_s - now_s;
        if (frame_time > 0.25)
            frame_time = 0.25;
        now_s = new_now_s;

        acc += frame_time;

        while (acc >= dt) {
            tick(dt);

            t += dt;
            acc -= dt;
        }

        const double alpha = acc / dt;
        render(alpha);

        window.display();
    }

    return kErrorSuccess;
}

void Game::tick(double dt) {
//    log("ticking with dt %f", dt);
    last_camera_state_ = camera_.tick(dt);
}

void Game::render(double alpha) {
//    log("rendering with alpha %f", alpha);

    CameraState interpolated(camera_.interpolate_from(last_camera_state_, alpha));
    renderer_.render_world(interpolated.transform(), alpha);
}

void Game::grab_cursor(bool grab) {
    // TODO use raw/relative mouse events in v2.6
    window_->setMouseCursorVisible(!grab);
    mouse_grabbed_ = grab;

    if (grab)
        original_mouse_pos_ = last_mouse_pos_ = sf::Mouse::getPosition(*window_);
    else
        sf::Mouse::setPosition(original_mouse_pos_, *window_);

}

void Game::on_mouse_grab(int x, int y) {
    int dx = last_mouse_pos_.x - x;
    int dy = last_mouse_pos_.y - y;

    camera_.rotate(dx, dy);

    auto new_pos = sf::Mouse::getPosition(*window_);
    last_mouse_pos_.x = new_pos.x;
    last_mouse_pos_.y = new_pos.y;
}

