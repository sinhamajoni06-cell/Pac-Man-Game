#include "slot_machine.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>

namespace {
const sf::Color kYellow(255, 255, 0);
const sf::Color kRed(230, 20, 0);
const sf::Color kLavender(222, 222, 255);
const sf::Color kWhite(255, 255, 255);

// Slows down at the end and bounces a little, like a real reel
float easeOutBack(float u) {
    const float c1 = 1.2f, c3 = c1 + 1.f;
    float t = u - 1.f;
    return 1.f + c3 * t * t * t + c1 * t * t;
}
}

bool SlotMachine::load(const std::string& assetRoot, const std::string& wordFile) {
    std::string base = assetRoot;
    if (!base.empty() && (base.back() == '/' || base.back() == '\\')) base.pop_back();

    bool front = m_front.openFromFile(base + "/TxT/name/crackman front.ttf");
    m_hasBack  = m_back.openFromFile(base + "/TxT/name/crackman back.ttf");
    if (!front && !m_front.openFromFile(base + "/TxT/name/crackman.ttf")) {
        std::cerr << "[Slot] Cannot load the Crackman font\n";
        return false;
    }

    sf::Text ref(m_front, "H", 100);                 // measure a capital letter
    sf::FloatRect b = ref.getLocalBounds();
    m_capTop = b.position.y;
    m_capH   = b.size.y > 0.f ? b.size.y : 70.f;

    // Words: one per line, 5 to 10 letters, letters only
    std::ifstream in(wordFile);
    if (!in) std::cerr << "[Slot] Cannot open " << wordFile << " (using a few built-in words)\n";
    std::string line;
    while (std::getline(in, line)) {
        std::string w;
        for (unsigned char c : line)
            if (std::isalpha(c)) w += static_cast<char>(std::toupper(c));
        if (w.size() >= 5 && w.size() <= 10) m_words.push_back(w);
    }
    if (m_words.empty())
        m_words = {"APPLE", "CHERRY", "MELON", "GALAXIAN", "STRAWBERRY"};
    return true;
}

char SlotMachine::letterAt(int index, int reel) const {
    if (index == m_reels[static_cast<size_t>(reel)].landing)
        return m_word[static_cast<size_t>(reel)];            // the letter this reel lands on

    std::uint32_t h = static_cast<std::uint32_t>(index) * 73856093u
                    ^ static_cast<std::uint32_t>(reel + 1) * 19349663u ^ m_seed;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return static_cast<char>('A' + h % 26);
}

void SlotMachine::start(int letters) {
    if (m_words.empty()) return;
    m_word = pickWord(letters);
    m_seed = static_cast<unsigned>(m_rng());

    std::uniform_real_distribution<float> startPos(0.f, 100.f);
    std::uniform_real_distribution<float> extraSpeed(0.f, 4.f);
    std::uniform_real_distribution<float> jitter(0.f, 0.15f);

    m_reels.assign(m_word.size(), Reel{});
    for (size_t i = 0; i < m_reels.size(); ++i) {
        m_reels[i].pos    = startPos(m_rng);
        m_reels[i].speed  = 16.f + extraSpeed(m_rng);
        m_reels[i].stopAt = 1.4f + 0.45f * static_cast<float>(i) + jitter(m_rng);
    }
    m_time = 0.f;
    m_winTime = 0.f;
    m_started = true;
    m_finished = false;
}

int SlotMachine::stoppedReels() const {
    int n = 0;
    for (const Reel& r : m_reels) if (r.stopped) ++n;
    return n;
}

void SlotMachine::update(float dt) {
    if (!m_started) return;
    if (m_finished) { m_winTime += dt; return; }

    m_time += dt;
    bool allStopped = true;

    for (Reel& r : m_reels) {
        if (r.stopped) continue;
        allStopped = false;

        if (!r.stopping) {
            r.pos += r.speed * dt;
            if (m_time >= r.stopAt) {                        // start to brake
                r.stopping = true;
                r.landFrom = r.pos;
                r.landing  = static_cast<int>(std::ceil(r.pos)) + 6;   // not shown yet, so no letter pops
                r.landTime = 0.f;
            }
        } else {
            r.landTime += dt;
            float u = std::min(1.f, r.landTime / 0.8f);
            r.pos = r.landFrom + (static_cast<float>(r.landing) - r.landFrom) * easeOutBack(u);
            if (u >= 1.f) { r.pos = static_cast<float>(r.landing); r.stopped = true; }
        }
    }
    if (allStopped) m_finished = true;
}

// Draws text with the title fonts (back layer in red, letters in yellow).
// The text is shrunk to fit capHeightMap and maxWidthMap (map pixels).
void SlotMachine::drawGlyphs(sf::RenderTarget& t, const std::string& s, sf::Vector2f centerMap,
                             float capHeightMap, float maxWidthMap, sf::Color color) {
    sf::Text front(m_front, s, 100);
    sf::FloatRect b = front.getLocalBounds();
    if (b.size.x <= 0.f) return;

    float k = capHeightMap / m_capH;
    if (maxWidthMap > 0.f) k = std::min(k, maxWidthMap / b.size.x);
    k *= m_scale;

    sf::Vector2f origin(b.position.x + b.size.x / 2.f, m_capTop + m_capH / 2.f);
    sf::Vector2f pos = m_origin + centerMap * m_scale;

    sf::Text back(m_hasBack ? m_back : m_front, s, 100);
    back.setFillColor(kRed);
    back.setOrigin(origin);
    back.setScale({k, k});
    back.setPosition(m_hasBack ? pos : pos + sf::Vector2f(1.5f * m_scale, 1.5f * m_scale));
    t.draw(back);

    front.setFillColor(color);
    front.setOrigin(origin);
    front.setScale({k, k});
    front.setPosition(pos);
    t.draw(front);
}

void SlotMachine::drawBox(sf::RenderTarget& t, sf::Vector2f pos, sf::Vector2f size,
                          sf::Color fill, sf::Color outline, float thickness) {
    sf::RectangleShape r(size * m_scale);
    r.setPosition(m_origin + pos * m_scale);
    r.setFillColor(fill);
    r.setOutlineColor(outline);
    r.setOutlineThickness(-thickness * m_scale);
    t.draw(r);
}

void SlotMachine::draw(sf::RenderTarget& target) {
    if (!m_started || m_reels.empty()) return;

    // Headline
    drawGlyphs(target, "WORD FINDING", {112.f, 52.f}, 22.f, 180.f, kYellow);

    // Layout: one reel per letter, centered
    const int n = static_cast<int>(m_word.size());
    const float gap = 2.f;
    const float rh = 34.f;                                      // reel height
    const float rw = std::min(22.f, (206.f - gap * static_cast<float>(n - 1)) / static_cast<float>(n));
    const float totalW = rw * static_cast<float>(n) + gap * static_cast<float>(n - 1);
    const float left = 112.f - totalW / 2.f;
    const float cy = 118.f;                                     // middle of the reels

    // Machine frame
    const float fx = left - 8.f, fy = cy - rh / 2.f - 12.f;
    const float fw = totalW + 16.f, fh = rh + 24.f;
    drawBox(target, {fx, fy}, {fw, fh}, sf::Color(10, 10, 40), kYellow, 2.f);

    // Marquee lights (blink faster when the word is found)
    const float blinkStep = m_finished ? 0.1f : 0.2f;
    const int phase = static_cast<int>((m_finished ? m_winTime : m_time) / blinkStep);
    int bulb = 0;
    auto drawBulb = [&](float x, float y) {
        bool lit = ((bulb + phase) % 2) == 0;
        drawBox(target, {x - 1.f, y - 1.f}, {2.f, 2.f}, lit ? kYellow : sf::Color(90, 10, 0), kYellow, 0.f);
        ++bulb;
    };
    for (float x = fx + 4.f; x <= fx + fw - 3.f; x += 8.f) { drawBulb(x, fy + 1.f); drawBulb(x, fy + fh - 1.f); }
    for (float y = fy + 8.f; y <= fy + fh - 7.f; y += 8.f) { drawBulb(fx + 1.f, y); drawBulb(fx + fw - 1.f, y); }

    // Pointers on both sides of the middle line
    drawBox(target, {fx - 6.f, cy - 1.5f}, {5.f, 3.f}, kRed, kRed, 0.f);
    drawBox(target, {fx + fw + 1.f, cy - 1.5f}, {5.f, 3.f}, kRed, kRed, 0.f);

    // Reels
    const bool flash = m_finished && m_winTime < 1.5f && (static_cast<int>(m_winTime / 0.15f) % 2 == 0);
    const sf::Color letterColor = flash ? kWhite : kYellow;

    sf::Vector2f ts(target.getSize());
    for (int i = 0; i < n; ++i) {
        const float rx = left + (rw + gap) * static_cast<float>(i);
        drawBox(target, {rx, cy - rh / 2.f}, {rw, rh}, sf::Color(15, 15, 60), kLavender, 1.f);

        // Clip the letters to this reel's window
        sf::FloatRect r(m_origin + sf::Vector2f(rx, cy - rh / 2.f) * m_scale, sf::Vector2f(rw, rh) * m_scale);
        sf::View old = target.getView();
        sf::Vector2f p0(target.mapCoordsToPixel(r.position, old));
        sf::Vector2f p1(target.mapCoordsToPixel(r.position + r.size, old));
        sf::View clip(r);
        clip.setViewport(sf::FloatRect({p0.x / ts.x, p0.y / ts.y},
                                       {(p1.x - p0.x) / ts.x, (p1.y - p0.y) / ts.y}));
        target.setView(clip);

        const Reel& reel = m_reels[static_cast<size_t>(i)];
        const int base = static_cast<int>(std::floor(reel.pos));
        for (int k = base - 1; k <= base + 2; ++k) {
            float y = cy + (reel.pos - static_cast<float>(k)) * rh;   // bigger index = higher up
            if (std::fabs(y - cy) > rh) continue;
            drawGlyphs(target, std::string(1, letterAt(k, i)), {rx + rw / 2.f, y}, 20.f, rw - 4.f, letterColor);
        }
        target.setView(old);
    }
}

std::string SlotMachine::randomWord(int letters) {
    return pickWord(letters);
}

std::string SlotMachine::pickWord(int letters) {
    if (m_words.empty()) return "APPLE";

    // words of the wanted length (if there are none, the closest length)
    std::vector<const std::string*> pool;
    int bestDiff = 1000;
    for (const std::string& w : m_words) {
        int d = 0;
        if (letters > 0) {
            d = static_cast<int>(w.size()) - letters;
            if (d < 0) d = -d;
        }
        if (d < bestDiff) { bestDiff = d; pool.clear(); }
        if (d == bestDiff) pool.push_back(&w);
    }

    // not the same word twice in a row (when there is a choice)
    if (pool.size() > 1)
        pool.erase(std::remove_if(pool.begin(), pool.end(),
                                  [&](const std::string* w) { return *w == m_word; }),
                   pool.end());

    std::uniform_int_distribution<size_t> pick(0, pool.size() - 1);
    return *pool[pick(m_rng)];
}
