#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>

// What was eaten during one update
struct EatResult {
    int food  = 0;
    int power = 0;
};

class EatSystem {
public:
    EatSystem() = default;
    EatSystem(const EatSystem&) = delete;             // sprites point at our textures
    EatSystem& operator=(const EatSystem&) = delete;

    // Loads "map/food.png" and "map/power_up.png" from `dir` and finds every dot
    bool load(const std::string& dir);

    // Same position/scale the map is drawn with (Map::getPosition / getScale)
    void setTransform(sf::Vector2f origin, float scale) { m_origin = origin; m_scale = scale; }

    // How close (map pixels) Pac-Man's center must get to a dot to eat it
    void setEatRadius(float mapPixels) { m_radius = mapPixels; }

    // Call every frame with Pac-Man's center (window coordinates).
    // Eats dots he is touching and blinks the power-ups.
    EatResult update(float dt, sf::Vector2f playerPos);

    void draw(sf::RenderTarget& target);

    void reset();                                      // put every dot back (new level)

    int  getScore()  const { return m_score; }
    void addScore(int points) { m_score += points; }   // e.g. points for eating ghosts
    int  foodLeft()  const { return m_foodLeft; }
    int  powerLeft() const { return m_powerLeft; }
    bool allEaten()  const { return m_foodLeft + m_powerLeft == 0; }

    // For Word Finding: letters replace some food dots
    int  dotCount() const { return static_cast<int>(m_dots.size()); }
    bool isFoodDot(int i) const { return !m_dots[static_cast<size_t>(i)].power && !m_dots[static_cast<size_t>(i)].eaten; }
    sf::Vector2f dotCenterMap(int i) const { return m_dots[static_cast<size_t>(i)].center; }
    void reserveDot(int i);                       // takes a food dot out of the game (no score)
    int  reservedCount() const { return m_reserved; }

private:
    struct Dot {
        sf::IntRect  rect;      // where the dot sits in its image (pixels)
        sf::Vector2f center;    // dot center in map pixels
        bool power  = false;
        bool eaten  = false;
    };

    bool findDots(const std::string& path, sf::Texture& tex, bool power);

    sf::Texture m_foodTex, m_powerTex;
    std::optional<sf::Sprite> m_foodSprite, m_powerSprite;
    std::vector<Dot> m_dots;

    sf::Vector2f m_origin{0.f, 0.f};
    float m_scale  = 1.f;
    float m_radius = 3.f;

    int m_score = 0, m_foodLeft = 0, m_powerLeft = 0;
    int m_reserved = 0;

    float m_blinkTimer   = 0.f;
    bool  m_powerVisible = true;
};
