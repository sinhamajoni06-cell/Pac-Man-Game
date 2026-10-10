#include <SFML/Graphics.hpp>
#include <optional>
#include "player.h"
#include "map.h"
#include "map_box.h"

int main()
{
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Pacman");
    window.setFramerateLimit(60);

    Map gameMap;
    if (!gameMap.load("main/assets/graphic/game"))
        return 1;
    gameMap.fitToWindow(window.getSize());

    MapBox mapBox;
    if (!mapBox.load("main/assets/graphic/game"))
        return 1;
    mapBox.setTransform(gameMap.getPosition(), gameMap.getScale());
    mapBox.setHitboxSize(6.f);        // Pac-Man's hitbox, in map pixels

    Player player;
    // change this path to where your GIF folder really is
    if (!player.load("main/assets/graphic/game/Pac-man"))
        return 1;
    // Start spot: center of the map, in the row below the ghost house (map pixel 113, 188)
    player.setPosition(gameMap.getPosition() + sf::Vector2f(113.f, 188.f) * gameMap.getScale());
    player.setMapBox(&mapBox);
    player.setDirection(Direction::Left);   // already moving left at launch
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
        gameMap.draw(window);
        player.draw(window);
        window.display();
    }
    return 0;
}
