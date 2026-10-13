#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <random>
#include <string>

struct PickupResult {
    int          points = 0;          // > 0 when Pac-Man ate the fruit this frame
    sf::Vector2f where{0.f, 0.f};     // map pixels, for the score popup
};

// Bonus fruit: appears after 70 and 170 dots, changes with the level,
// plus the row of recent fruits at the bottom right.
class PickupSystem {
public:
    PickupSystem() = default;
    PickupSystem(const PickupSystem&) = delete;
    PickupSystem& operator=(const PickupSystem&) = delete;

    // `dir` contains cherry.svg, strawberry.svg, orange.svg, apple.svg, melon.svg,
    // galaxian.svg, key.svg (and optionally bell.svg)
    bool load(const std::string& dir);

    void setTransform(sf::Vector2f origin, float scale) { m_origin = origin; m_scale = scale; }

    void setLevel(int level);          // new level: resets the fruit timers
    void clearFruit() { m_active = false; }

    // dotsEaten: dots eaten so far in this level
    PickupResult update(float dt, sf::Vector2f playerWindowPos, int dotsEaten);

    void draw(sf::RenderTarget& target);

private:
    struct FruitInfo { const char* name; int points; };
    static FruitInfo fruitForLevel(int level);

    bool loadSvg(const std::string& path, sf::Texture& tex);
    const sf::Texture* textureFor(const std::string& name) const;
    void spawn();

    std::map<std::string, sf::Texture> m_tex;

    sf::Vector2f m_origin{0.f, 0.f};
    float m_scale = 1.f;

    int   m_level = 1;
    bool  m_spawned[2] = {false, false};

    bool        m_active = false;
    float       m_timeLeft = 0.f;
    int         m_points = 0;
    std::string m_name;

    std::mt19937 m_rng{std::random_device{}()};
};