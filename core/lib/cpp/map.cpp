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

void Map::update(float dt) {
    const float blinkTime = 0.23f;   // seconds per on/off phase (arcade-like)
    m_blinkTimer += dt;
    while (m_blinkTimer >= blinkTime) {
        m_blinkTimer -= blinkTime;
        m_powerVisible = !m_powerVisible;
    }
}
