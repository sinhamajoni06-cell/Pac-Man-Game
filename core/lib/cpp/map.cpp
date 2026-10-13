#include "map.h"

#include <algorithm>
#include <iostream>

bool Map::load(const std::string& dir) {
    std::string base = dir;
    if (!base.empty() && base.back() != '/' && base.back() != '\\') base += '/';

    if (!m_wallsTex.loadFromFile(base + "map.png")) {
        std::cerr << "[Map] Cannot load: " << base << "map.png\n";
        return false;
    }
    if (!m_fullTex.loadFromFile(base + "map/map_full_.png")) {
        std::cerr << "[Map] Cannot load: " << base << "map/map_full_.png\n";
        return false;
    }

    m_walls.emplace(m_wallsTex);
    m_full.emplace(m_fullTex);

    // White version of the same maze: only the blue wall pixels change color
    {
        sf::Image img = m_fullTex.copyToImage();
        const sf::Vector2u size = img.getSize();
        for (unsigned y = 0; y < size.y; ++y) {
            for (unsigned x = 0; x < size.x; ++x) {
                sf::Color c = img.getPixel({x, y});
                bool blueWall = c.a > 0 && c.b > 200 && c.r < 80;   // the maze blue (33,33,255)
                if (blueWall) img.setPixel({x, y}, sf::Color(222, 222, 255));
            }
        }
        if (!m_whiteTex.loadFromImage(img)) {
            std::cerr << "[Map] Could not build the white maze\n";
            return false;
        }
    }

    if (!m_foodTex.loadFromFile(base + "map/food.png")) {
        std::cerr << "[Map] Cannot load: " << base << "map/food.png\n";
        return false;
    }
    if (!m_powerTex.loadFromFile(base + "map/power_up.png")) {
        std::cerr << "[Map] Cannot load: " << base << "map/power_up.png\n";
        return false;
    }
    m_food.emplace(m_foodTex);
    m_power.emplace(m_powerTex);
    return true;
}

void Map::fitToWindow(sf::Vector2u windowSize) {
    if (!m_walls || !m_full || !m_food || !m_power) return;

    sf::Vector2u tex = m_wallsTex.getSize();
    m_scale = std::min(static_cast<float>(windowSize.x) / tex.x,
                       static_cast<float>(windowSize.y) / tex.y);

    float w = tex.x * m_scale;
    float h = tex.y * m_scale;
    m_pos = {(windowSize.x - w) / 2.f, (windowSize.y - h) / 2.f};

    for (auto* s : {&*m_walls, &*m_full, &*m_food, &*m_power}) {
        s->setScale({m_scale, m_scale});
        s->setPosition(m_pos);
    }
}

void Map::draw(sf::RenderTarget& target, bool withDots) {
    if (withDots) {
        if (m_full) target.draw(*m_full);
    } else {
        if (m_walls) target.draw(*m_walls);
    }

    // Food and power-ups are drawn on top of the map
    // food is drawn by EatSystem now
    // power-ups are drawn (and blink) in EatSystem now
}

void Map::startFlash(float duration, float interval) {
    m_flashing      = true;
    m_flashWhite    = true;            // the first change is to white
    m_flashElapsed  = 0.f;
    m_flashTimer    = 0.f;
    m_flashDuration = duration;
    m_flashInterval = interval;
    if (m_full) m_full->setTexture(m_whiteTex);
}

void Map::stopFlash() {
    m_flashing   = false;
    m_flashWhite = false;
    if (m_full) m_full->setTexture(m_fullTex);
}

void Map::update(float dt) {
    if (m_flashing) {
        m_flashElapsed += dt;
        m_flashTimer   += dt;
        while (m_flashTimer >= m_flashInterval) {
            m_flashTimer -= m_flashInterval;
            m_flashWhite = !m_flashWhite;
        }
        if (m_flashElapsed >= m_flashDuration) {
            m_flashing   = false;
            m_flashWhite = false;
        }
        if (m_full) m_full->setTexture(m_flashWhite ? m_whiteTex : m_fullTex);
    }
    const float blinkTime = 0.23f;   // seconds per on/off phase (arcade-like)
    m_blinkTimer += dt;
    while (m_blinkTimer >= blinkTime) {
        m_blinkTimer -= blinkTime;
        m_powerVisible = !m_powerVisible;
    }
}

void Map::fitWithMargins(sf::Vector2u windowSize, float topMargin, float bottomMargin) {
    if (!m_walls || !m_full || !m_food || !m_power) return;

    sf::Vector2u tex = m_wallsTex.getSize();
    float totalH = tex.y + topMargin + bottomMargin;           // maze + HUD rows
    m_scale = std::min(static_cast<float>(windowSize.x) / tex.x,
                       static_cast<float>(windowSize.y) / totalH);

    float w = tex.x * m_scale;
    float h = totalH * m_scale;
    m_pos = {(windowSize.x - w) / 2.f, (windowSize.y - h) / 2.f + topMargin * m_scale};

    for (auto* s : {&*m_walls, &*m_full, &*m_food, &*m_power}) {
        s->setScale({m_scale, m_scale});
        s->setPosition(m_pos);
    }
}