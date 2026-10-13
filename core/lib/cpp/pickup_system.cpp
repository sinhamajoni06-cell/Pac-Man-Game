#include "pickup_system.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

// nanosvg: reads SVG files and draws them into pixels
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg.h"
#include "nanosvgrast.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace {
const sf::Vector2f kSpot{112.f, 140.f};      // below the ghost house (map pixels)
}

PickupSystem::FruitInfo PickupSystem::fruitForLevel(int level) {
    if (level <= 1)  return {"cherry",     100};
    if (level == 2)  return {"strawberry", 300};
    if (level <= 4)  return {"orange",     500};
    if (level <= 6)  return {"apple",      700};
    if (level <= 8)  return {"melon",     1000};
    if (level <= 10) return {"galaxian",  2000};
    if (level <= 12) return {"bell",      3000};
    return                  {"key",       5000};
}

bool PickupSystem::loadSvg(const std::string& path, sf::Texture& tex) {
    NSVGimage* svg = nsvgParseFromFile(path.c_str(), "px", 96.f);
    if (!svg) {
        std::cerr << "[Pickup] Cannot read SVG: " << path << "\n";
        return false;
    }
    int w = static_cast<int>(std::ceil(svg->width));
    int h = static_cast<int>(std::ceil(svg->height));
    if (w <= 0 || h <= 0) {
        std::cerr << "[Pickup] SVG has no size: " << path << "\n";
        nsvgDelete(svg);
        return false;
    }

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    std::vector<unsigned char> px(static_cast<size_t>(w) * h * 4, 0);
    nsvgRasterize(rast, svg, 0.f, 0.f, 1.f, px.data(), w, h, w * 4);
    nsvgDeleteRasterizer(rast);
    nsvgDelete(svg);

    sf::Image img(sf::Vector2u(static_cast<unsigned>(w), static_cast<unsigned>(h)), px.data());
    if (!tex.loadFromImage(img)) {
        std::cerr << "[Pickup] Texture failed: " << path << "\n";
        return false;
    }
    return true;
}

bool PickupSystem::load(const std::string& dir) {
    std::string base = dir;
    if (!base.empty() && base.back() != '/' && base.back() != '\\') base += '/';

    const char* names[] = {"cherry", "strawberry", "orange", "apple", "melon", "galaxian", "bell", "key"};
    for (const char* n : names) {
        std::string path = base + n + ".svg";
        bool optional = (std::string(n) == "bell");           // no bell file in the pack: fine

        if (optional && !std::ifstream(path)) continue;
        if (!loadSvg(path, m_tex[n])) return false;
    }
    setLevel(1);
    return true;
}

const sf::Texture* PickupSystem::textureFor(const std::string& name) const {
    auto it = m_tex.find(name);
    if (it == m_tex.end() && name == "bell") it = m_tex.find("galaxian");   // fallback
    return it == m_tex.end() ? nullptr : &it->second;
}

void PickupSystem::setLevel(int level) {
    m_level = std::max(1, level);
    m_spawned[0] = m_spawned[1] = false;
    m_active = false;
}

void PickupSystem::spawn() {
    FruitInfo f = fruitForLevel(m_level);
    m_name = f.name;
    m_points = f.points;
    std::uniform_real_distribution<float> extra(0.f, 0.67f);
    m_timeLeft = 9.33f + extra(m_rng);                         // 9 to 10 seconds, like the arcade
    m_active = true;
}

PickupResult PickupSystem::update(float dt, sf::Vector2f playerWindowPos, int dotsEaten) {
    PickupResult r;

    if (!m_spawned[0] && dotsEaten >= 70)  { m_spawned[0] = true; spawn(); }
    if (!m_spawned[1] && dotsEaten >= 170) { m_spawned[1] = true; spawn(); }

    if (m_active) {
        m_timeLeft -= dt;
        if (m_timeLeft <= 0.f) {
            m_active = false;                                  // nobody took it
        } else {
            sf::Vector2f p = (playerWindowPos - m_origin) / m_scale;   // window -> map pixels
            sf::Vector2f d = p - kSpot;
            if (d.x * d.x + d.y * d.y < 36.f) {                // within 6 map pixels
                m_active = false;
                r.points = m_points;
                r.where = kSpot;
            }
        }
    }
    return r;
}

void PickupSystem::draw(sf::RenderTarget& target) {
    // The fruit waiting in the maze
    if (m_active) {
        if (const sf::Texture* tex = textureFor(m_name)) {
            sf::Sprite s(*tex);
            sf::Vector2u sz = tex->getSize();
            s.setOrigin({sz.x / 2.f, sz.y / 2.f});
            s.setScale({m_scale, m_scale});
            s.setPosition(m_origin + kSpot * m_scale);
            target.draw(s);
        }
    }

    // Level indicator: the last 7 fruits at the bottom right, newest on the right
    int shown = std::min(m_level, 7);
    for (int k = 0; k < shown; ++k) {
        FruitInfo f = fruitForLevel(m_level - k);
        const sf::Texture* tex = textureFor(f.name);
        if (!tex) continue;

        sf::Sprite s(*tex);
        sf::Vector2u sz = tex->getSize();
        s.setOrigin({sz.x / 2.f, sz.y / 2.f});
        s.setScale({m_scale, m_scale});
        s.setPosition(m_origin + sf::Vector2f(200.f - 16.f * k, 256.f) * m_scale);
        target.draw(s);
    }
}