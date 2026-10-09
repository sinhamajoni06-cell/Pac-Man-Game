#include <SFML/Graphics.hpp>
#include <optional>
#include "player.h"

int main()
{
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Pacman");
    window.setFramerateLimit(60);

    Player player;
    // change this path to where your GIF folder really is
    if (!player.load("main/assets/graphic/game/Pac-man"))
        return 1;
    player.setPosition({400.f, 300.f});
    player.setScale(2.f);

    sf::Clock clock;

    while (window.isOpen())
    {
        float dt = clock.restart().asSeconds();

        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        player.handleInput();
        player.update(dt);

        window.clear(sf::Color::Black);
        player.draw(window);
        window.display();
    }
    return 0;
}
