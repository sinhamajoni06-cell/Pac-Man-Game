#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>
#include "map_box.h"

enum class Direction { None, Up, Down, Left, Right };

class Player {
public:
    Player() = default;

    bool load(const std::string& dir);   // loads the 4 GIFs from this folder
    void handleInput();                  // Arrow keys / WASD

    void setDirection(Direction d);
    Direction getDirection() const { return m_dir; }
    void setMoving(bool moving) { m_moving = moving; }  // false = freeze animation

    void setPosition(sf::Vector2f p) { m_pos = p; }
    void setScale(float s)           { m_scale = s; }
    void setSpeed(float pxPerSec)    { m_speed = pxPerSec; }
    void setMapBox(const MapBox* box) { m_box = box; }
    sf::Vector2f getPosition() const { return m_pos; }

    void update(float dt);
    void draw(sf::RenderTarget& target);

private:
    struct Animation {
        std::vector<sf::Texture> frames;
        std::vector<float>       delays;   // seconds per frame
    };

    bool loadGif(const std::string& path, Animation& out);
    Animation& current();

    Animation m_up, m_down, m_left, m_right;
    Direction m_dir     = Direction::Right;
    Direction m_lastDir = Direction::None;
    Direction m_wanted  = Direction::None;    // direction of the key held right now
    std::vector<Direction> m_held;            // held keys, newest last
    bool m_keyDown[4]   = {false, false, false, false};   // Up, Down, Left, Right last frame
    sf::Vector2f m_snap{0.f, 0.f};                         // remaining glide into a corridor
    const MapBox* m_box = nullptr;
    bool      m_moving  = false;
    float     m_speed   = 240.f;
    float     m_scale   = 1.f;
    float     m_timer   = 0.f;
    size_t    m_frame   = 0;
    sf::Vector2f m_pos{400.f, 300.f};
    std::optional<sf::Sprite> m_sprite;   // SFML 3 sprites need a texture at creation
};
