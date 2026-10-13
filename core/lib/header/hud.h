#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>

// Arcade text: 1UP, score, HIGH SCORE, PLAYER ONE, READY!, GAME OVER,
// ghost score popups and the life icons.
// All positions are in MAP pixels (the maze is 224x248; the HUD sits above and below it).
class Hud {
public:
    Hud() = default;
    Hud(const Hud&) = delete;
    Hud& operator=(const Hud&) = delete;

    bool load(const std::string& fontPath);             // PressStart2P-Regular.ttf
    void setTransform(sf::Vector2f origin, float scale) { m_origin = origin; m_scale = scale; }

    void setScore(int score);                           // also raises the high score
    void setLives(int extraLives) { m_lives = extraLives; }
    void setLifeTexture(const sf::Texture* tex);
    bool loadLifeIcon(const std::string& path);         // load the life icon from an image file        // Pac-Man icon

    void showPlayerOne(bool v) { m_playerOne = v; }
    void showReady(bool v)     { m_ready = v; }
    void showGameOver(bool v)  { m_gameOver = v; }
    void addPopup(const std::string& text, sf::Vector2f mapPos, float seconds);

    void loadHighScore(const std::string& path);
    void saveHighScore(const std::string& path) const;
    int  highScore() const { return m_high; }

    void update(float dt);
    void draw(sf::RenderTarget& target);

private:
    enum class Align { Left, Center, Right };
    struct Popup { std::string text; sf::Vector2f pos; float time; };

    void drawText(sf::RenderTarget& target, const std::string& s, sf::Vector2f mapPos,
                  sf::Color color, Align align);
    static std::string pad(int value);

    sf::Font m_font;
    bool  m_loaded = false;
    float m_glyphTop = 0.f, m_glyphH = 8.f;

    sf::Vector2f m_origin{0.f, 0.f};
    float m_scale = 1.f;

    int  m_score = 0, m_high = 0, m_lives = 0;
    bool m_playerOne = false, m_ready = false, m_gameOver = false;

    float m_blinkTimer = 0.f;
    bool  m_blinkOn = true;

    std::vector<Popup> m_popups;
    sf::Texture m_lifeTex;
    std::optional<sf::Sprite> m_lifeSprite;
};