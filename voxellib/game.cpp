#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include "game.h"
#include "error.h"
#include "util.h"

int Game::run() {
    // create window
    sf::ContextSettings settings;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    sf::RenderWindow window(sf::VideoMode(800, 600), "OpenGL", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    target_ = &window;

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

void Game::tick(float dt) {
    Log("ticking with dt %f\n", dt);
}

void Game::render(double alpha) {
    target_->clear();

    Log("rendering with alpha %f\n", alpha);
}
