#include "map_box.h"

#include <cmath>
#include <iostream>

bool MapBox::load(const std::string& dir) {
    std::string base = dir;
    if (!base.empty() && base.back() != '/' && base.back() != '\\') base += '/';

    sf::Image img;
    if (!img.loadFromFile(base + "map_full_.png")) {
        std::cerr << "[MapBox] Cannot load: " << base << "map_full_.png\n";
        return false;
    }

    m_w = img.getSize().x;
    m_h = img.getSize().y;
    m_sum.assign(static_cast<size_t>(m_w + 1) * (m_h + 1), 0);

    for (unsigned y = 0; y < m_h; ++y) {
        for (unsigned x = 0; x < m_w; ++x) {
            sf::Color c = img.getPixel({x, y});
            bool nonBlack = c.a > 0 && (c.r + c.g + c.b) > 0;
            bool isDot    = c.r > 200 && c.b < 200;      // pellets are (255,183,174)
            int wall = (nonBlack && !isDot) ? 1 : 0;     // walls blue, door pink-white

            size_t i = static_cast<size_t>(y + 1) * (m_w + 1) + (x + 1);
            m_sum[i] = wall
                     + m_sum[static_cast<size_t>(y)     * (m_w + 1) + (x + 1)]
                     + m_sum[static_cast<size_t>(y + 1) * (m_w + 1) + x]
                     - m_sum[static_cast<size_t>(y)     * (m_w + 1) + x];
        }
    }
    return true;
}

bool MapBox::blockedMap(float mx, float my) const {
    if (m_sum.empty()) return false;

    float half = m_size / 2.f;
    int x0 = static_cast<int>(std::floor(mx - half + 0.001f));
    int y0 = static_cast<int>(std::floor(my - half + 0.001f));
    int x1 = static_cast<int>(std::ceil (mx + half - 0.001f));
    int y1 = static_cast<int>(std::ceil (my + half - 0.001f));

    if (x0 < 0 || y0 < 0 || x1 > static_cast<int>(m_w) || y1 > static_cast<int>(m_h))
        return true;                                      // outside the map

    auto S = [&](int x, int y) { return m_sum[static_cast<size_t>(y) * (m_w + 1) + x]; };
    return (S(x1, y1) - S(x0, y1) - S(x1, y0) + S(x0, y0)) > 0;
}

bool MapBox::isBlocked(sf::Vector2f pos) const {
    sf::Vector2f m = toMap(pos);
    return blockedMap(m.x, m.y);
}

bool MapBox::move(sf::Vector2f& pos, sf::Vector2f delta) const {
    sf::Vector2f d = delta / m_scale;                     // to map pixels
    float len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len <= 0.f) return true;

    int steps = static_cast<int>(std::ceil(len / 0.25f)); // never skip over thin walls
    sf::Vector2f step = d / static_cast<float>(steps);

    sf::Vector2f m = toMap(pos);
    for (int i = 0; i < steps; ++i) {
        sf::Vector2f n = m + step;
        if (blockedMap(n.x, n.y)) {
            pos = fromMap(m);
            return false;
        }
        m = n;
    }
    pos = fromMap(m);
    return true;
}

bool MapBox::tryTurn(sf::Vector2f& pos, sf::Vector2f dir) const {
    sf::Vector2f m = toMap(pos);
    sf::Vector2f probe = dir;                             // look 1 map pixel ahead

    if (!blockedMap(m.x + probe.x, m.y + probe.y))
        return true;                                      // already lined up

    // Slide along the axis perpendicular to the wanted direction
    sf::Vector2f perp = (dir.y != 0.f) ? sf::Vector2f(1.f, 0.f) : sf::Vector2f(0.f, 1.f);
    bool open[2] = {true, true};                          // [0] = negative side, [1] = positive side

    for (int k = 1; k <= static_cast<int>(m_tolerance); ++k) {
        for (int s = 0; s < 2; ++s) {
            if (!open[s]) continue;
            float off = (s == 0 ? -1.f : 1.f) * static_cast<float>(k);
            sf::Vector2f c = m + perp * off;
            if (blockedMap(c.x, c.y)) { open[s] = false; continue; }   // wall in the way
            if (!blockedMap(c.x + probe.x, c.y + probe.y)) {
                pos = fromMap(c);
                return true;
            }
        }
    }
    return false;
}
