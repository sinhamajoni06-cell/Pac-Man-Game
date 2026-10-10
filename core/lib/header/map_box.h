#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// Pixel-accurate wall hitbox built from the map image.
// All public functions take WINDOW coordinates; the map transform
// (origin + scale) converts them to image pixels internally.
class MapBox {
public:
    // Loads "map_full_.png" from `dir`. Blue walls and the pink ghost door
    // are solid; black and the dots are walkable.
    bool load(const std::string& dir);

    // Same position/scale the map is drawn with (Map::getPosition / getScale)
    void setTransform(sf::Vector2f origin, float scale) { m_origin = origin; m_scale = scale; }

    // Hitbox size in MAP pixels (square, centered on the player)
    void setHitboxSize(float mapPixels)     { m_size = mapPixels; }
    // How far (map pixels) a turn may be nudged to line up with a corridor
    void setTurnTolerance(float mapPixels)  { m_tolerance = mapPixels; }

    // True if a hitbox centered at `pos` touches a wall or leaves the map
    bool isBlocked(sf::Vector2f pos) const;

    // Moves `pos` by `delta`, stopping at walls. Returns true if the full
    // distance was covered, false if a wall stopped it.
    bool move(sf::Vector2f& pos, sf::Vector2f delta) const;

    // Can we start moving along `dir` (unit vector)? If it is blocked only by
    // a small misalignment, `pos` is nudged into the corridor. Returns true on success.
    bool tryTurn(sf::Vector2f& pos, sf::Vector2f dir) const;

    // Arcade-style movement along the corridor lanes.
    // dir / wanted: 0 = none, 1 = up, 2 = down, 3 = left, 4 = right
    // Returns true if Pac-Man moved forward (false = blocked by a wall).
    bool advance(sf::Vector2f& pos, int& dir, int wanted, float distance) const;
    void setCornerWindow(float mapPixels) { m_corner = mapPixels; }

private:
    bool blockedMap(float mx, float my) const;
    bool tileOpen(float cx, float cy, int dx, int dy) const;
    float lane(float v) const;
    sf::Vector2f toMap(sf::Vector2f p) const   { return (p - m_origin) / m_scale; }
    sf::Vector2f fromMap(sf::Vector2f m) const { return m_origin + m * m_scale; }

    unsigned m_w = 0, m_h = 0;
    std::vector<int> m_sum;          // integral image of wall pixels, (w+1)*(h+1)
    sf::Vector2f m_origin{0.f, 0.f};
    float m_scale     = 1.f;
    float m_size      = 6.f;
    float m_tolerance = 3.f;
    float m_corner    = 3.f;    // how early/late a turn is still accepted (map pixels)
    float m_laneOrigin = 12.f;  // first lane center in the map image
    float m_tile       = 8.f;   // distance between lane centers
};
