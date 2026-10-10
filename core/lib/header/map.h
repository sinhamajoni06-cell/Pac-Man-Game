#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>

class Map {
public:
    Map() = default;
    Map(const Map&) = delete;             // sprites point at our textures
    Map& operator=(const Map&) = delete;

    // Loads "map.png" (walls only) and "map_full_.png" (walls + dots) from `dir`
    bool load(const std::string& dir);

    // Scales the map to fill the window height and centers it horizontally
    void fitToWindow(sf::Vector2u windowSize);

    // withDots = true draws map_full_.png, false draws map.png
    void draw(sf::RenderTarget& target, bool withDots = true);

    sf::Vector2f getPosition() const { return m_pos; }
    float        getScale()    const { return m_scale; }
    sf::Vector2u getTextureSize() const { return m_wallsTex.getSize(); }

private:
    sf::Texture m_wallsTex;
    sf::Texture m_fullTex;
    std::optional<sf::Sprite> m_walls;   // SFML 3 sprites need a texture at creation
    std::optional<sf::Sprite> m_full;
    sf::Vector2f m_pos{0.f, 0.f};
    float m_scale = 1.f;
};
