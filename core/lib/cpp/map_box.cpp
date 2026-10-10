#include "map_box.h"

#include <algorithm>
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

float MapBox::lane(float v) const {
    return m_laneOrigin + m_tile * std::round((v - m_laneOrigin) / m_tile);
}

// Can the hitbox travel one full tile from (cx, cy) in direction (dx, dy)?
bool MapBox::tileOpen(float cx, float cy, int dx, int dy) const {
    for (int i = 0; i <= 16; ++i) {
        if (blockedMap(cx + dx * i * 0.5f, cy + dy * i * 0.5f)) return false;
    }
    return true;
}

bool MapBox::advance(sf::Vector2f& pos, int& dir, int wanted, float distance) const {
    static const int dx[5]       = {0,  0, 0, -1, 1};
    static const int dy[5]       = {0, -1, 1,  0, 0};
    static const int opposite[5] = {0,  2, 1,  4, 3};

    if (dir < 1 || dir > 4) return false;
    float units = distance / m_scale;                     // map pixels this frame
    if (units <= 0.f) return false;

    int   n   = static_cast<int>(std::ceil(units / 0.25f));
    float sub = units / static_cast<float>(n);            // small steps: never skip a wall

    sf::Vector2f m = toMap(pos);
    bool advanced = false;

    for (int i = 0; i < n; ++i) {
        // 1. Direction change
        if (wanted > 0 && wanted != dir) {
            if (wanted == opposite[dir]) {
                // reversing is always instant (if there is room)
                if (!blockedMap(m.x + dx[wanted] * sub, m.y + dy[wanted] * sub)) dir = wanted;
            } else {
                // turning: only near a lane crossing, and only if that way is open
                bool  horiz = dir >= 3;
                float a = horiz ? m.x : m.y;
                float c = lane(a);
                if (std::fabs(a - c) <= m_corner) {
                    float cx = horiz ? c : lane(m.x);
                    float cy = horiz ? lane(m.y) : c;
                    if (tileOpen(cx, cy, dx[wanted], dy[wanted])) dir = wanted;
                }
            }
        }

        // 2. Slide toward the center of the lane (cornering / always aligned)
        bool horiz = dir >= 3;
        float off = horiz ? (lane(m.y) - m.y) : (lane(m.x) - m.x);
        float g   = std::clamp(off, -sub, sub);
        if (g != 0.f) {
            float gx = horiz ? m.x : m.x + g;
            float gy = horiz ? m.y + g : m.y;
            if (!blockedMap(gx, gy)) { m.x = gx; m.y = gy; }
        }

        // 3. Move forward, but never pass a lane center when the next tile is a wall
        float a  = horiz ? m.x : m.y;                  // position along the motion axis
        float sg = static_cast<float>(horiz ? dx[dir] : dy[dir]);
        float c  = lane(a);
        float cx = horiz ? c : lane(m.x);
        float cy = horiz ? lane(m.y) : c;
        float na = a + sg * sub;

        if (!tileOpen(cx, cy, dx[dir], dy[dir])) {
            if ((a - c) * sg >= -1e-3f) continue;      // at the center, wall ahead: stop here
            if ((na - c) * sg > 0.f) na = c;           // would overshoot: stop exactly on the center
        }

        float fx = horiz ? na : m.x;
        float fy = horiz ? m.y : na;
        if (!blockedMap(fx, fy)) {
            if (fx != m.x || fy != m.y) advanced = true;
            m.x = fx;
            m.y = fy;
        }
    }

    pos = fromMap(m);
    return advanced;
}
