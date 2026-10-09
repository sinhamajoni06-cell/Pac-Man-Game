#include <SFML/Graphics.hpp>
#include <optional>

int main()
{
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Pacman");
    window.setFramerateLimit(60);

    sf::CircleShape pacman(30.f);
    pacman.setFillColor(sf::Color::Yellow);
    pacman.setOrigin({30.f, 30.f});
    pacman.setPosition({400.f, 300.f});

    const float speed = 4.f;

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))  pacman.move({-speed, 0.f});
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) pacman.move({speed, 0.f});
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))    pacman.move({0.f, -speed});
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))  pacman.move({0.f, speed});

        window.clear(sf::Color::Black);
        window.draw(pacman);
        window.display();
    }
    return 0;
}