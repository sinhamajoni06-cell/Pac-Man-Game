#include <SFML/Graphics.hpp>
#include <optional>
#include "player.h"
#include "map.h"
#include "map_box.h"
#include "eat_system.h"

// Keeps the 800x600 game world in the right proportions when the window is
// resized or maximized (black bars appear if the window shape is different)
static void applyLetterbox(sf::RenderWindow& window, sf::Vector2u size)
{
    const float worldW = 800.f, worldH = 600.f;
    float windowRatio = static_cast<float>(size.x) / static_cast<float>(size.y);
    float worldRatio  = worldW / worldH;

    sf::FloatRect viewport({0.f, 0.f}, {1.f, 1.f});
    if (windowRatio > worldRatio) {            // window is wider: bars left and right
        viewport.size.x     = worldRatio / windowRatio;
        viewport.position.x = (1.f - viewport.size.x) / 2.f;
    } else {                                   // window is taller: bars top and bottom
        viewport.size.y     = windowRatio / worldRatio;
        viewport.position.y = (1.f - viewport.size.y) / 2.f;
    }

    sf::View view(sf::FloatRect({0.f, 0.f}, {worldW, worldH}));
    view.setViewport(viewport);
    window.setView(view);
}

int main()
{
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Pacman");
    window.setFramerateLimit(60);

    Map gameMap;
    if (!gameMap.load("main/assets/graphic/game"))
        return 1;
    gameMap.fitToWindow({800, 600});

    EatSystem eatSystem;
    if (!eatSystem.load("main/assets/graphic/game"))
        return 1;
    eatSystem.setTransform(gameMap.getPosition(), gameMap.getScale());

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
    player.setPosition(gameMap.getPosition() + sf::Vector2f(112.f, 188.f) * gameMap.getScale());
    player.setMapBox(&mapBox);
    mapBox.setWrapMargin(player.getHalfWidth() / gameMap.getScale());   // jump only when fully hidden
    player.setDirection(Direction::Left);   // already moving left at launch
    player.setScale(gameMap.getScale());   // sprite scaled together with the maze
    player.setSpeed(60.f * gameMap.getScale());   // arcade speed: ~60 map pixels per second

    sf::Clock clock;

    while (window.isOpen())
    {
        float dt = clock.restart().asSeconds();
        // (blinking now lives in EatSystem)

        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto* resized = event->getIf<sf::Event::Resized>())
                applyLetterbox(window, resized->size);
        }

        player.handleInput();
        player.update(dt);
        eatSystem.update(dt, player.getPosition());

        window.clear(sf::Color::Black);
        gameMap.draw(window);
        eatSystem.draw(window);
        player.draw(window);
        window.display();
    }
    return 0;
}
