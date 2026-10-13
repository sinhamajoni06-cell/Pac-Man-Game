#include "hud.h"

#include <algorithm>
#include <fstream>
#include <iostream>

namespace {
const sf::Color kWhite (222, 222, 255);
const sf::Color kYellow(255, 255, 0);
const sf::Color kCyan  (0, 255, 255);
const sf::Color kRed   (255, 0, 0);
}

bool Hud::load(const std::string& fontPath) {
    if (!m_font.openFromFile(fontPath)) {
        std::cerr << "[Hud] Cannot load font: " << fontPath << "\n";
        return false;
    }
    m_font.setSmooth(false);                       // keep the pixel look

    sf::Text ref(m_font, "H", 8);                  // measure where a capital letter sits
    sf::FloatRect b = ref.getLocalBounds();
    m_glyphTop = b.position.y;
    m_glyphH   = b.size.y;

    m_loaded = true;
    return true;
}

void Hud::setScore(int score) {
    m_score = score;
    if (m_score > m_high) m_high = m_score;
}

void Hud::setLifeTexture(const sf::Texture* tex) {
    if (tex) m_lifeSprite.emplace(*tex);
    else     m_lifeSprite.reset();
}

bool Hud::loadLifeIcon(const std::string& path) {
    if (!m_lifeTex.loadFromFile(path)) {
        std::cerr << "[Hud] Cannot load life icon: " << path << "\n";
        return false;
    }
    m_lifeSprite.emplace(m_lifeTex);
    return true;
}

void Hud::addPopup(const std::string& text, sf::Vector2f mapPos, float seconds) {
    m_popups.push_back({text, mapPos, seconds});
}

void Hud::loadHighScore(const std::string& path) {
    std::ifstream f(path);
    int v = 0;
    if (f >> v && v > 0) m_high = v;
}

void Hud::saveHighScore(const std::string& path) const {
    std::ofstream f(path);
    if (f) f << m_high << "\n";
}

std::string Hud::pad(int value) {
    std::string s = std::to_string(value);
    if (s.size() < 2) s = "0" + s;                 // the arcade shows at least "00"
    return s;
}

void Hud::update(float dt) {
    m_blinkTimer += dt;
    while (m_blinkTimer >= 0.28f) {                // "1UP" blinks
        m_blinkTimer -= 0.28f;
        m_blinkOn = !m_blinkOn;
    }
    for (Popup& p : m_popups) p.time -= dt;
    m_popups.erase(std::remove_if(m_popups.begin(), m_popups.end(),
                                  [](const Popup& p) { return p.time <= 0.f; }),
                   m_popups.end());
}

// mapPos.y is the vertical CENTER of the text; the font advances 8 pixels per letter
void Hud::drawText(sf::RenderTarget& target, const std::string& s, sf::Vector2f mapPos,
                   sf::Color color, Align align) {
    sf::Text text(m_font, s, 8);
    text.setFillColor(color);

    float w  = 8.f * static_cast<float>(s.size());
    float ox = (align == Align::Center) ? w / 2.f : (align == Align::Right ? w : 0.f);
    text.setOrigin({ox, m_glyphTop + m_glyphH / 2.f});
    text.setPosition(m_origin + mapPos * m_scale);
    text.setScale({m_scale, m_scale});
    target.draw(text);
}

void Hud::draw(sf::RenderTarget& target) {
    if (!m_loaded) return;

    // Top rows (the maze starts at y = 0, the HUD is above it)
    if (m_blinkOn) drawText(target, "1UP", {24.f, -20.f}, kWhite, Align::Left);
    drawText(target, pad(m_score), {56.f, -12.f}, kWhite, Align::Right);
    drawText(target, "HIGH SCORE", {72.f, -20.f}, kWhite, Align::Left);
    drawText(target, pad(m_high), {136.f, -12.f}, kWhite, Align::Right);

    // Messages in the middle of the maze (below / above the ghost house)
    if (m_playerOne) drawText(target, "PLAYER ONE", {112.f, 92.f},  kCyan,   Align::Center);
    if (m_ready)     drawText(target, "READY!",     {112.f, 140.f}, kYellow, Align::Center);
    if (m_gameOver)  drawText(target, "GAME  OVER", {112.f, 140.f}, kRed,    Align::Center);

    // Points where a ghost was eaten
    for (const Popup& p : m_popups) drawText(target, p.text, p.pos, kCyan, Align::Center);

    // Life icons under the maze
    if (m_lifeSprite) {
        sf::Vector2u sz = m_lifeSprite->getTexture().getSize();
        m_lifeSprite->setOrigin({sz.x / 2.f, sz.y / 2.f});
        float fit = m_scale * 14.f / static_cast<float>(sz.y);   // icon height = 14 map pixels
        m_lifeSprite->setScale({fit, fit});
        for (int i = 0; i < std::min(m_lives, 5); ++i) {
            m_lifeSprite->setPosition(m_origin + sf::Vector2f(24.f + 16.f * i, 256.f) * m_scale);
            target.draw(*m_lifeSprite);
        }
    }
}