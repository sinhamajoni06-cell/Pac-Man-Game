#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include "map_box.h"
#include "enemy/gif_animation.h"

struct PlayerInfo {
    sf::Vector2f pos{0.f, 0.f};   // map pixels
    int dir = 4;                  // 1 up, 2 down, 3 left, 4 right
};

struct GhostContext {
    PlayerInfo   player;
    sf::Vector2f blinkyPos{0.f, 0.f};   // map pixels (Inky needs it)
    bool         chase = false;         // false = scatter
};

// Base class: movement on the maze tiles, house logic, animation.
// Each ghost only decides WHERE it wants to go (chaseTarget / scatterTarget).
class Ghost {
public:
    enum class State { InHouse, Leaving, Active, Eaten, Entering };

    // Speeds in map pixels per second (Pac-Man runs at about 60)
    static constexpr float kSpeedNormal = 56.f;
    static constexpr float kSpeedFright = 34.f;
    static constexpr float kSpeedTunnel = 28.f;
    static constexpr float kSpeedEyes   = 130.f;
    static constexpr float kSpeedHouse  = 30.f;
    static constexpr float kSpeedBob    = 20.f;

    Ghost(const std::string& name, sf::Vector2f houseSpot);
    virtual ~Ghost() = default;
    Ghost(const Ghost&) = delete;
    Ghost& operator=(const Ghost&) = delete;

    // `dir` contains "<Name>/<Name> (Up).gif", (Down), (Left), (Right)
    bool load(const std::string& dir);
    void setMap(const MapBox* box) { m_box = box; }

    void reset();                       // back to the start position
    void release();                     // leave the ghost house
    void frighten();                    // turn blue, reverse
    void setFlash(bool flash) { m_flash = flash; }
    void calm()               { m_frightened = false; m_flash = false; }
    void getEaten();                    // Pac-Man ate this ghost
    void reverse();                     // forced turn-around (mode change)

    void update(float dt, const GhostContext& ctx);
    void draw(sf::RenderTarget& target, sf::Vector2f origin, float scale);

    State        state()      const { return m_state; }
    bool         frightened() const { return m_frightened; }
    sf::Vector2f position()   const { return m_pos; }      // map pixels
    bool canBeHit() const { return m_state == State::Active || m_state == State::Leaving; }

    int   direction() const { return m_dir; }            // 1 up, 2 down, 3 left, 4 right
    float speed()     const { return currentSpeed(); }   // map pixels per second

    // Used by EnemyManager to keep ghosts from stacking (0 = no limit)
    void  setSpeedLimit(float limit) { m_speedLimit = limit; }

protected:
    // Targets are in TILE coordinates (tile = 8x8 map pixels)
    virtual sf::Vector2f chaseTarget(const GhostContext& ctx, sf::Vector2f myTile) const = 0;
    virtual sf::Vector2f scatterTarget() const = 0;
    virtual bool startsOutside() const { return false; }   // only Blinky

    static sf::Vector2f tileOf(sf::Vector2f p) {
        return {std::floor(p.x / 8.f), std::floor(p.y / 8.f)};
    }
    // The tile `tiles` ahead of Pac-Man (with the arcade's "up" bug: also shifts left)
    static sf::Vector2f ahead(const PlayerInfo& p, int tiles) {
        sf::Vector2f t = tileOf(p.pos);
        float n = static_cast<float>(tiles);
        switch (p.dir) {
            case 1:  return {t.x - n, t.y - n};
            case 2:  return {t.x, t.y + n};
            case 3:  return {t.x - n, t.y};
            default: return {t.x + n, t.y};
        }
    }

private:
    static constexpr int kDX[5] = {0, 0, 0, -1, 1};
    static constexpr int kDY[5] = {0, -1, 1, 0, 0};

    void startActive();
    float currentSpeed() const;
    bool followPath(float dist);
    void stepActive(float dist, const GhostContext& ctx);
    void chooseDirection(const GhostContext& ctx);
    bool stepOpen(int col, int row, int dir) const;

    std::string  m_name;
    sf::Vector2f m_house;               // where it waits inside the house
    const MapBox* m_box = nullptr;

    GifAnimation m_anims[5];            // indexed by direction 1..4
    std::optional<sf::Sprite> m_sprite;
    size_t m_animFrame = 0;
    float  m_animTimer = 0.f;

    State  m_state = State::InHouse;
    bool   m_frightened = false;
    bool   m_flash = false;

    // Tile movement (Active / Eaten)
    int    m_col = 14, m_row = 11;      // current tile
    int    m_dir = 3;                   // 1 up, 2 down, 3 left, 4 right
    float  m_offset = 0.f;              // map pixels traveled toward the next tile (0..8)

    // Free movement (house)
    sf::Vector2f m_pos{112.f, 92.f};    // map pixels
    std::vector<sf::Vector2f> m_path;
    size_t m_pathIdx = 0;
    float  m_bobDir = -1.f;
    float  m_speedLimit = 0.f;   // 0 = no limit (set by EnemyManager)
};
