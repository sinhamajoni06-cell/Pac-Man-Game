#include "eat_system.h"

#include <cmath>
#include <iostream>

bool EatSystem::findDots(const std::string& path, sf::Texture& tex, bool power) {
    sf::Image img;
    if (!img.loadFromFile(path)) {
        std::cerr << "[EatSystem] Cannot load: " << path << "\n";
        return false;
    }
    if (!tex.loadFromImage(img)) {
        std::cerr << "[EatSystem] Texture failed: " << path << "\n";
        return false;
    }

    const unsigned w = img.getSize().x, h = img.getSize().y;
    std::vector<char> seen(static_cast<size_t>(w) * h, 0);
    std::vector<unsigned> stack;

    // Every connected group of visible pixels is one dot
    for (unsigned y = 0; y < h; ++y) {
        for (unsigned x = 0; x < w; ++x) {
            size_t idx = static_cast<size_t>(y) * w + x;
            if (seen[idx] || img.getPixel({x, y}).a == 0) continue;

            unsigned minX = x, maxX = x, minY = y, maxY = y;
            stack.clear();
            stack.push_back(static_cast<unsigned>(idx));
            seen[idx] = 1;

            while (!stack.empty()) {
                unsigned cur = stack.back();
                stack.pop_back();
                unsigned cx = cur % w, cy = cur / w;
                minX = std::min(minX, cx); maxX = std::max(maxX, cx);
                minY = std::min(minY, cy); maxY = std::max(maxY, cy);

                const int nx[4] = {1, -1, 0, 0};
                const int ny[4] = {0, 0, 1, -1};
                for (int k = 0; k < 4; ++k) {
                    int px = static_cast<int>(cx) + nx[k];
                    int py = static_cast<int>(cy) + ny[k];
                    if (px < 0 || py < 0 || px >= static_cast<int>(w) || py >= static_cast<int>(h)) continue;
                    size_t ni = static_cast<size_t>(py) * w + px;
                    if (seen[ni] || img.getPixel({static_cast<unsigned>(px), static_cast<unsigned>(py)}).a == 0) continue;
                    seen[ni] = 1;
                    stack.push_back(static_cast<unsigned>(ni));
                }
            }

            Dot d;
            d.rect   = sf::IntRect({static_cast<int>(minX), static_cast<int>(minY)},
                                   {static_cast<int>(maxX - minX + 1), static_cast<int>(maxY - minY + 1)});
            d.center = {minX + (maxX - minX + 1) / 2.f, minY + (maxY - minY + 1) / 2.f};
            d.power  = power;
            m_dots.push_back(d);
        }
    }
    return true;
}

bool EatSystem::load(const std::string& dir) {
    std::string base = dir;
    if (!base.empty() && base.back() != '/' && base.back() != '\\') base += '/';

    m_dots.clear();
    if (!findDots(base + "map/food.png",     m_foodTex,  false)) return false;
    if (!findDots(base + "map/power_up.png", m_powerTex, true))  return false;

    m_foodSprite.emplace(m_foodTex);
    m_powerSprite.emplace(m_powerTex);
    reset();
    return true;
}

void EatSystem::reset() {
    m_foodLeft = m_powerLeft = 0;
    for (Dot& d : m_dots) {
        d.eaten = false;
        (d.power ? m_powerLeft : m_foodLeft)++;
    }
    m_score = 0;
    m_blinkTimer = 0.f;
    m_powerVisible = true;
}

EatResult EatSystem::update(float dt, sf::Vector2f playerPos) {
    EatResult result;

    // Power-up blinking
    const float blinkTime = 0.23f;
    m_blinkTimer += dt;
    while (m_blinkTimer >= blinkTime) {
        m_blinkTimer -= blinkTime;
        m_powerVisible = !m_powerVisible;
    }

    // Eating: Pac-Man's center touches a dot's center
    sf::Vector2f p = (playerPos - m_origin) / m_scale;   // window -> map pixels
    for (Dot& d : m_dots) {
        if (d.eaten) continue;
        if (std::fabs(d.center.x - p.x) < m_radius && std::fabs(d.center.y - p.y) < m_radius) {
            d.eaten = true;
            if (d.power) { ++result.power; --m_powerLeft; m_score += 50; }
            else         { ++result.food;  --m_foodLeft;  m_score += 10; }
        }
    }
    return result;
}

void EatSystem::draw(sf::RenderTarget& target) {
    if (!m_foodSprite || !m_powerSprite) return;

    for (const Dot& d : m_dots) {
        if (d.eaten) continue;
        if (d.power && !m_powerVisible) continue;       // blink

        sf::Sprite& s = d.power ? *m_powerSprite : *m_foodSprite;
        s.setTextureRect(d.rect);
        s.setPosition(m_origin + sf::Vector2f(d.rect.position) * m_scale);
        s.setScale({m_scale, m_scale});
        target.draw(s);
    }
}