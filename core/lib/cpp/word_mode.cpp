#include "word_mode.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>

namespace {
const sf::Color kYellow(255, 255, 0);
const sf::Color kCyan(0, 255, 255);
const sf::Color kWhite(255, 255, 255);
const sf::Color kDim(100, 100, 170);
}

bool WordMode::load(const std::string& fontPath) {
    if (!m_font.openFromFile(fontPath)) {
        std::cerr << "[WordMode] Cannot load font: " << fontPath << "\n";
        return false;
    }
    m_font.setSmooth(false);
    sf::Text ref(m_font, "H", 8);
    sf::FloatRect b = ref.getLocalBounds();
    m_top = b.position.y;
    m_h   = b.size.y;
    m_loaded = true;
    return true;
}

void WordMode::start(const std::string& word, EatSystem& eat, sf::Vector2f avoidMapPos) {
    m_word.clear();
    for (unsigned char c : word)
        if (std::isalpha(c)) m_word += static_cast<char>(std::toupper(c));

    m_letters.clear();
    m_collected = 0;
    m_doneTime = 0.f;
    m_slots.assign(m_word.begin(), m_word.end());
    m_filled.assign(m_word.size(), false);
    if (m_word.empty()) return;

    // Food dots that may be replaced: not too close to Pac-Man's start
    std::vector<int> candidates;
    for (int i = 0; i < eat.dotCount(); ++i) {
        if (!eat.isFoodDot(i)) continue;
        sf::Vector2f c = eat.dotCenterMap(i);
        float dx = c.x - avoidMapPos.x, dy = c.y - avoidMapPos.y;
        if (dx * dx + dy * dy < 28.f * 28.f) continue;
        candidates.push_back(i);
    }
    std::shuffle(candidates.begin(), candidates.end(), m_rng);

    // Spread them out: start with a big minimum distance and relax it if needed
    const size_t n = m_word.size();
    std::vector<int> picked;
    for (float sep = 56.f; sep >= 0.f && picked.size() < n; sep -= 8.f) {
        picked.clear();
        for (int i : candidates) {
            sf::Vector2f c = eat.dotCenterMap(i);
            bool ok = true;
            for (int j : picked) {
                sf::Vector2f d = c - eat.dotCenterMap(j);
                if (d.x * d.x + d.y * d.y < sep * sep) { ok = false; break; }
            }
            if (ok) {
                picked.push_back(i);
                if (picked.size() == n) break;
            }
        }
    }

    for (size_t k = 0; k < picked.size() && k < n; ++k) {
        eat.reserveDot(picked[k]);                         // the dot is gone, a letter takes its place
        Letter l;
        l.c = m_word[k];
        l.pos = eat.dotCenterMap(picked[k]);
        m_letters.push_back(l);
    }
}

char WordMode::update(float dt, sf::Vector2f playerWindowPos, bool canCollect) {
    m_time += dt;
    if (complete()) m_doneTime += dt;
    if (!canCollect || m_letters.empty()) return 0;

    sf::Vector2f p = (playerWindowPos - m_origin) / m_scale;       // window -> map pixels
    for (Letter& l : m_letters) {
        if (l.taken) continue;
        if (std::fabs(l.pos.x - p.x) < 4.f && std::fabs(l.pos.y - p.y) < 4.f) {
            l.taken = true;
            ++m_collected;
            for (size_t i = 0; i < m_slots.size(); ++i) {           // fill the first empty slot of this letter
                if (!m_filled[i] && m_slots[i] == l.c) { m_filled[i] = true; break; }
            }
            return l.c;
        }
    }
    return 0;
}

void WordMode::drawChar(sf::RenderTarget& target, char c, sf::Vector2f mapPos, sf::Color color) {
    sf::Text t(m_font, std::string(1, c), 8);
    t.setFillColor(color);
    t.setOrigin({4.f, m_top + m_h / 2.f});
    t.setScale({m_scale, m_scale});
    t.setPosition(m_origin + mapPos * m_scale);
    target.draw(t);
}

void WordMode::drawLetters(sf::RenderTarget& target) {
    if (!m_loaded) return;
    const bool on = (static_cast<int>(m_time / 0.2f) % 2) == 0;     // blinking
    for (const Letter& l : m_letters)
        if (!l.taken) drawChar(target, l.c, l.pos, on ? kCyan : kWhite);
}

void WordMode::drawWordBar(sf::RenderTarget& target) {
    if (!m_loaded || m_slots.empty()) return;

    const float n = static_cast<float>(m_slots.size());
    const float spacing = 11.f;
    const float y = -5.f;                                           // the row just above the maze
    const bool flash = complete() && (static_cast<int>(m_doneTime / 0.15f) % 2) == 0;

    for (size_t i = 0; i < m_slots.size(); ++i) {
        float x = 112.f + (static_cast<float>(i) - (n - 1.f) / 2.f) * spacing;
        if (m_filled[i]) drawChar(target, m_slots[i], {x, y}, flash ? kWhite : kYellow);
        else             drawChar(target, '_',        {x, y}, kDim);
    }
}
