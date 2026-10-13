#include "title_screen.h"

#include <algorithm>
#include <cctype>
#include <iostream>

namespace {
const sf::Color kWhite (222, 222, 255);
const sf::Color kCyan  (0, 255, 255);
const sf::Color kOrange(255, 184, 81);
const sf::Color kDot   (255, 184, 174);

const sf::Color kLogoFront(255, 255, 0);      // yellow letters
const sf::Color kLogoBack (230, 20, 0);       // red back layer

const char* kGhost[4] = {"Blinky", "Pinky", "Inky", "Clyde"};
const char* kRole[4]  = {"-SHADOW", "-SPEEDY", "-BASHFUL", "-POKEY"};
const char* kNick[4]  = {"\"BLINKY\"", "\"PINKY\"", "\"INKY\"", "\"CLYDE\""};
const sf::Color kGhostColor[4] = {
    sf::Color(255, 0, 0),        // Blinky red
    sf::Color(255, 184, 255),    // Pinky pink
    sf::Color(0, 255, 255),      // Inky cyan
    sf::Color(255, 184, 81)      // Clyde orange
};
}

void TitleScreen::setTitle(const std::string& title) {
    m_title = title;
    std::transform(m_title.begin(), m_title.end(), m_title.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
}

bool TitleScreen::load(const std::string& assetRoot) {
    std::string base = assetRoot;
    if (!base.empty() && (base.back() == '/' || base.back() == '\\')) base.pop_back();

    if (!m_font.openFromFile(base + "/TxT/PressStart2P-Regular.ttf")) {
        std::cerr << "[Title] Cannot load Press Start 2P font\n";
        return false;
    }
    m_font.setSmooth(false);
    sf::Text ref(m_font, "H", 8);
    sf::FloatRect b = ref.getLocalBounds();
    m_glyphTop = b.position.y;
    m_glyphH   = b.size.y;

    // Logo fonts: front = the letters, back = the layer behind them
    bool front = m_logoFront.openFromFile(base + "/TxT/name/crackman front.ttf");
    m_hasBack  = m_logoBack.openFromFile(base + "/TxT/name/crackman back.ttf");
    if (!front && !m_logoFront.openFromFile(base + "/TxT/name/crackman.ttf")) {
        std::cerr << "[Title] Cannot load the Crackman font\n";
        return false;
    }

    for (int i = 0; i < 4; ++i) {
        std::string path = base + "/graphic/game/" + kGhost[i] + "/" + kGhost[i] + " (Right).gif";
        if (!m_ghost[i].load(path)) return false;
    }

    setTitle(m_title);
    reset();
    return true;
}

void TitleScreen::reset() {
    m_time = 0.f;
    for (int i = 0; i < 4; ++i) { m_frame[i] = 0; m_frameTimer[i] = 0.f; }
}

void TitleScreen::update(float dt) {
    m_time += dt;
    for (int i = 0; i < 4; ++i) {
        const GifAnimation& a = m_ghost[i];
        if (a.frames.empty()) continue;
        m_frameTimer[i] += dt;
        while (m_frameTimer[i] >= a.delays[m_frame[i]]) {
            m_frameTimer[i] -= a.delays[m_frame[i]];
            m_frame[i] = (m_frame[i] + 1) % a.frames.size();
        }
    }
}

bool TitleScreen::startRequested() const {
    return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter) ||
           sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
}

// mapPos.y is the vertical center of the text; the font advances 8 pixels per letter
void TitleScreen::drawText(sf::RenderTarget& target, const std::string& s, sf::Vector2f mapPos,
                           sf::Color color, Align align) {
    sf::Text text(m_font, s, 8);
    text.setFillColor(color);
    float w = 8.f * static_cast<float>(s.size());
    text.setOrigin({align == Align::Center ? w / 2.f : 0.f, m_glyphTop + m_glyphH / 2.f});
    text.setPosition(m_origin + mapPos * m_scale);
    text.setScale({m_scale, m_scale});
    target.draw(text);
}

void TitleScreen::drawLogo(sf::RenderTarget& target) {
    const sf::Vector2f center{112.f, 36.f};     // map pixels
    const float targetWidth = 200.f;            // the logo is stretched to this width

    sf::Text front(m_logoFront, m_title, 100);
    sf::FloatRect b = front.getLocalBounds();
    if (b.size.x <= 0.f) return;
    float k = targetWidth / b.size.x * m_scale;
    sf::Vector2f origin(b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f);
    sf::Vector2f pos = m_origin + center * m_scale;

    // 1. back layer (or a small red shadow when there is no back font)
    sf::Text back(m_hasBack ? m_logoBack : m_logoFront, m_title, 100);
    back.setFillColor(kLogoBack);
    back.setOrigin(origin);
    back.setScale({k, k});
    back.setPosition(m_hasBack ? pos : pos + sf::Vector2f(1.5f * m_scale, 1.5f * m_scale));
    target.draw(back);

    // 2. yellow letters on top
    front.setFillColor(kLogoFront);
    front.setOrigin(origin);
    front.setScale({k, k});
    front.setPosition(pos);
    target.draw(front);
}

void TitleScreen::draw(sf::RenderTarget& target) {
    drawLogo(target);

    if (m_time >= 0.5f)
        drawText(target, "CHARACTER / NICKNAME", {112.f, 72.f}, kWhite, Align::Center);

    // One ghost every 1.8 s: picture, then its role, then its nickname
    for (int i = 0; i < 4; ++i) {
        float t0 = 1.0f + 1.8f * static_cast<float>(i);
        float y  = 92.f + 24.f * static_cast<float>(i);

        if (m_time >= t0 && !m_ghost[i].frames.empty()) {
            const sf::Texture& tex = m_ghost[i].frames[m_frame[i]];
            sf::Sprite s(tex);
            sf::Vector2u sz = tex.getSize();
            s.setOrigin({sz.x / 2.f, sz.y / 2.f});
            s.setScale({m_scale, m_scale});
            s.setPosition(m_origin + sf::Vector2f(44.f, y) * m_scale);
            target.draw(s);
        }
        if (m_time >= t0 + 0.6f) drawText(target, kRole[i], {64.f, y},  kGhostColor[i], Align::Left);
        if (m_time >= t0 + 1.2f) drawText(target, kNick[i], {136.f, y}, kGhostColor[i], Align::Left);
    }

    // Point values
    if (m_time >= 8.2f) {
        sf::RectangleShape dot({2.f * m_scale, 2.f * m_scale});
        dot.setFillColor(kDot);
        dot.setOrigin({m_scale, m_scale});
        dot.setPosition(m_origin + sf::Vector2f(84.f, 190.f) * m_scale);
        target.draw(dot);
        drawText(target, "10 PTS", {96.f, 190.f}, kWhite, Align::Left);

        if (static_cast<int>(m_time / 0.25f) % 2 == 0) {        // the big pellet blinks
            sf::CircleShape pellet(4.f * m_scale);
            pellet.setFillColor(kDot);
            pellet.setOrigin({4.f * m_scale, 4.f * m_scale});
            pellet.setPosition(m_origin + sf::Vector2f(84.f, 204.f) * m_scale);
            target.draw(pellet);
        }
        drawText(target, "50 PTS", {96.f, 204.f}, kWhite, Align::Left);
    }

    // Prompt
    if (static_cast<int>(m_time / 0.5f) % 2 == 0)
        drawText(target, "PRESS ENTER", {112.f, 226.f}, kOrange, Align::Center);
    drawText(target, "1 PLAYER ONLY", {112.f, 242.f}, kCyan, Align::Center);
}
