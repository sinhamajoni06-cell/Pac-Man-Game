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

void WordMode::start(const std::string& word, EatSystem& eat, sf::Vector2f avoidMapPos, int decoys) {
    m_word.clear();
    for (unsigned char c : word)
        if (std::isalpha(c)) m_word += static_cast<char>(std::toupper(c));

    m_letters.clear();
    m_collected = 0;
    m_realTotal = 0;
    m_playTime = 0.f;
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

    // One spot per letter plus one per wrong letter, spread out:
    // start with a big minimum distance and relax it if there is not enough room
    const size_t wanted = m_word.size() + static_cast<size_t>(std::max(0, decoys));
    std::vector<int> picked;
    for (float sep = 56.f; sep >= 0.f && picked.size() < wanted; sep -= 8.f) {
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
                if (picked.size() == wanted) break;
            }
        }
    }

    // Wrong letters: letters that are NOT in the word
    std::vector<char> pool;
    for (char c = 'A'; c <= 'Z'; ++c)
        if (m_word.find(c) == std::string::npos) pool.push_back(c);
    std::shuffle(pool.begin(), pool.end(), m_rng);
    size_t decoyIndex = 0;

    for (size_t k = 0; k < picked.size(); ++k) {
        eat.reserveDot(picked[k]);                         // the dot is gone, a letter takes its place
        Letter l;
        l.pos = eat.dotCenterMap(picked[k]);
        if (k < m_word.size()) {
            l.c = m_word[k];
            ++m_realTotal;
        } else {
            l.decoy = true;
            l.c = pool.empty() ? 'X' : pool[decoyIndex++ % pool.size()];
        }
        m_letters.push_back(l);
    }
}

WordPickup WordMode::update(float dt, sf::Vector2f playerWindowPos, bool canCollect) {
    WordPickup r;
    m_time += dt;
    if (complete()) m_doneTime += dt;
    if (!canCollect || m_letters.empty()) return r;
    if (!complete()) m_playTime += dt;                             // the clock runs while you play

    sf::Vector2f p = (playerWindowPos - m_origin) / m_scale;       // window -> map pixels
    for (Letter& l : m_letters) {
        if (l.taken) continue;
        if (std::fabs(l.pos.x - p.x) < 4.f && std::fabs(l.pos.y - p.y) < 4.f) {
            l.taken = true;
            r.letter = l.c;
            r.decoy  = l.decoy;
            r.pos    = l.pos;
            if (!l.decoy) {
                ++m_collected;
                for (size_t i = 0; i < m_slots.size(); ++i) {       // fill the first empty slot of this letter
                    if (!m_filled[i] && m_slots[i] == l.c) { m_filled[i] = true; break; }
                }
            }
            return r;
        }
    }
    return r;
}

// 10 points for every second left under a limit of 12 seconds per letter
int WordMode::timeBonus() const {
    float par = 12.f * static_cast<float>(m_realTotal);
    float left = par - m_playTime;
    if (left <= 0.f) return 0;
    return static_cast<int>(left) * 10;
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
        if (!l.taken) drawChar(target, l.c, l.pos, on ? kCyan : kWhite);   // wrong letters look the same
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
