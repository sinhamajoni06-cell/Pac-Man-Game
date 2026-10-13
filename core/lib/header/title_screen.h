#pragma once
#include <SFML/Graphics.hpp>
#include <string>

#include "enemy/gif_animation.h"

// The home page: logo, CHARACTER / NICKNAME list, point values, PRESS ENTER.
// Everything is placed in MAP pixels (same layout space as the HUD).
class TitleScreen {
public:
    TitleScreen() = default;
    TitleScreen(const TitleScreen&) = delete;
    TitleScreen& operator=(const TitleScreen&) = delete;

    // assetRoot = "main/assets" (needs TxT/ and graphic/game/ inside)
    bool load(const std::string& assetRoot);
    void setTransform(sf::Vector2f origin, float scale) { m_origin = origin; m_scale = scale; }
    void setTitle(const std::string& title);

    void reset();                       // restart the reveal from the beginning
    void update(float dt);
    void draw(sf::RenderTarget& target);

    bool startRequested() const;        // Enter or Space is held

private:
    enum class Align { Left, Center };

    void drawText(sf::RenderTarget& target, const std::string& s, sf::Vector2f mapPos,
                  sf::Color color, Align align);
    void drawLogo(sf::RenderTarget& target);

    sf::Font m_font;                    // Press Start 2P
    sf::Font m_logoFront, m_logoBack;   // Crackman
    bool  m_hasBack = false;
    float m_glyphTop = 0.f, m_glyphH = 8.f;

    GifAnimation m_ghost[4];            // Blinky, Pinky, Inky, Clyde (facing right)
    size_t m_frame[4] = {0, 0, 0, 0};
    float  m_frameTimer[4] = {0.f, 0.f, 0.f, 0.f};

    std::string  m_title = "WORD-MAN";
    sf::Vector2f m_origin{0.f, 0.f};
    float m_scale = 1.f;
    float m_time = 0.f;
};
