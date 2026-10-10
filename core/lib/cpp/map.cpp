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
    if (!m_fullTex.loadFromFile(base + "map_full_.png")) {
        std::cerr << "[Map] Cannot load: " << base << "map_full_.png\n";
        return false;
    }

    m_walls.emplace(m_wallsTex);
    m_full.emplace(m_fullTex);
    return true;
}

void Map::fitToWindow(sf::Vector2u windowSize) {
    if (!m_walls || !m_full) return;

    sf::Vector2u tex = m_wallsTex.getSize();
    m_scale = std::min(static_cast<float>(windowSize.x) / tex.x,
                       static_cast<float>(windowSize.y) / tex.y);

    float w = tex.x * m_scale;
    float h = tex.y * m_scale;
    m_pos = {(windowSize.x - w) / 2.f, (windowSize.y - h) / 2.f};

    for (auto* s : {&*m_walls, &*m_full}) {
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
}
