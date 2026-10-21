#include "loading_animation.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

namespace {
// The GIF frame that is showing at time t (loops)
const sf::Texture* frameAt(const std::vector<sf::Texture>& frames,
                           const std::vector<float>& delays, float t) {
    if (frames.empty() || delays.size() != frames.size()) return nullptr;
    float total = 0.f;
    for (float d : delays) total += d;
    if (total <= 0.f) return &frames[0];

    float m = std::fmod(t, total);
    for (size_t i = 0; i < frames.size(); ++i) {
        if (m < delays[i]) return &frames[i];
        m -= delays[i];
    }
    return &frames.back();
}
}

bool LoadingAnimation::load(const std::string& gameDir) {
    std::string base = gameDir;
    if (!base.empty() && (base.back() == '/' || base.back() == '\\')) base.pop_back();

    // Font for the LOADING text (main/assets/TxT)
    if (m_font.openFromFile(base + "/../../TxT/PressStart2P-Regular.ttf")) {
        m_font.setSmooth(false);
        sf::Text ref(m_font, "H", 8);
        sf::FloatRect b = ref.getLocalBounds();
        m_fontTop = b.position.y;
        m_fontH   = b.size.y;
        m_fontLoaded = true;
    } else {
        std::cerr << "[Loading] Cannot load the font for the LOADING text\n";
    }

    return m_pacLeft.load(base + "/Pac-man/Pac-Man (Left).gif")
        && m_pacRight.load(base + "/Pac-man/Pac-Man (Intermission #1).gif")
        && m_blinkyLeft.load(base + "/Blinky/Blinky (Left).gif")
        && m_blinkyRight.load(base + "/Blinky/Blinky (Right).gif");
}

bool LoadingAnimation::finished() const {
    return m_time >= kStartGap + kChase1 + kMidGap + kChase2 + kEndGap;
}

void LoadingAnimation::draw(sf::RenderTarget& target) {
    // Clip to the game area so everything comes in and goes out at the screen edges
    sf::Vector2f ts(target.getSize());
    sf::FloatRect r(m_origin + sf::Vector2f(0.f, -24.f) * m_scale, sf::Vector2f(224.f, 288.f) * m_scale);
    sf::View old = target.getView();

    sf::Vector2f p0(target.mapCoordsToPixel(r.position, old));
    sf::Vector2f p1(target.mapCoordsToPixel(r.position + r.size, old));
    sf::View clip(r);
    clip.setViewport(sf::FloatRect({p0.x / ts.x, p0.y / ts.y},
                                   {(p1.x - p0.x) / ts.x, (p1.y - p0.y) / ts.y}));
    target.setView(clip);

    auto drawSprite = [&](const sf::Texture* tex, float mapX, float size) {
        if (!tex) return;
        sf::Sprite s(*tex);
        sf::Vector2u sz = tex->getSize();
        s.setOrigin({sz.x / 2.f, sz.y / 2.f});
        s.setScale({m_scale * size, m_scale * size});
        s.setPosition(m_origin + sf::Vector2f(mapX, kY) * m_scale);
        target.draw(s);
    };

    float t = m_time - kStartGap;

    if (t >= 0.f && t < kChase1) {
        // 1. Pac-Man runs left, Blinky chases him
        float pacX = 252.f - kSpeed * t;
        drawSprite(frameAt(m_pacLeft.frames,    m_pacLeft.delays,    m_time), pacX, 1.f);
        drawSprite(frameAt(m_blinkyLeft.frames, m_blinkyLeft.delays, m_time), pacX + kBlinkyBehind, 1.f);
    }
    else if (t >= kChase1 + kMidGap && t < kChase1 + kMidGap + kChase2) {
        // 2. A blue Blinky runs right, Pac-Man comes in behind him and grows
        float u = t - kChase1 - kMidGap;
        float blinkyX = -24.f + kSpeed * u;
        float pacX    = blinkyX - kPacBehind;
        float grow    = 1.f;                                                   // the GIF is already big

        drawSprite(frameAt(m_blinkyRight.frightened, m_blinkyRight.delays, m_time), blinkyX, 1.f);
        drawSprite(frameAt(m_pacRight.frames, m_pacRight.delays, m_time), pacX, grow);
    }

    drawLoadingUi(target);                 // text, bar and percentage (clipped like the scene)
    target.setView(old);
}

void LoadingAnimation::start() {
    m_time = 0.f;
    buildProgress();
}

float LoadingAnimation::totalTime() {
    return kStartGap + kChase1 + kMidGap + kChase2 + kEndGap;
}

// A made-up loading curve: waits, quick jumps, a long stall near the end, then 100%.
// A new random curve is made every time the intro starts.
void LoadingAnimation::buildProgress() {
    std::mt19937 rng{std::random_device{}()};
    auto uni = [&](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

    const int n = 9 + static_cast<int>(rng() % 4);                 // 9 to 12 steps
    std::vector<float> wait(n), burst(n), gain(n);
    float sumT = 0.f, sumG = 0.f;
    for (int i = 0; i < n; ++i) {
        wait[i]  = uni(0.05f, 0.8f);                               // how long it seems stuck
        burst[i] = uni(0.12f, 0.5f);                               // how long the jump takes
        gain[i]  = (rng() % 4 == 0) ? uni(15.f, 28.f)              // sometimes a big jump
                                    : uni(2.f, 12.f);              // mostly small ones
        if (i == n - 1) { wait[i] *= 2.5f; gain[i] = uni(1.f, 3.f); }   // stuck near the end
        sumT += wait[i] + burst[i];
        sumG += gain[i];
    }

    // Fit everything into the intro: it must reach 100% just before the end
    const float timeBudget = totalTime() - kEndGap - 0.1f;
    const float k = timeBudget / sumT;

    m_steps.clear();
    float t = 0.f, p = 0.f;
    for (int i = 0; i < n; ++i) {
        Step s;
        s.t0 = t;
        s.burstStart = t + wait[i] * k;
        s.t1 = s.burstStart + burst[i] * k;
        s.p0 = p;
        p += gain[i] / sumG * 100.f;
        s.p1 = p;
        m_steps.push_back(s);
        t = s.t1;
    }
    m_steps.back().p1 = 100.f;                                     // exactly 100 at the end
}

float LoadingAnimation::progress() const {
    if (m_steps.empty()) return std::min(100.f, 100.f * m_time / totalTime());
    if (m_time <= 0.f) return 0.f;

    for (const Step& s : m_steps) {
        if (m_time >= s.t1) continue;
        if (m_time <= s.burstStart) return s.p0;                   // waiting
        float u = (m_time - s.burstStart) / (s.t1 - s.burstStart);
        u = u * u * (3.f - 2.f * u);                               // smooth jump
        return s.p0 + (s.p1 - s.p0) * u;
    }
    return 100.f;
}

void LoadingAnimation::drawLoadingUi(sf::RenderTarget& target) {
    if (!m_fontLoaded || !m_showUi) return;

    const sf::Color lavender(222, 222, 255), yellow(255, 255, 0);

    auto drawText = [&](const std::string& s, sf::Vector2f mapPos, sf::Color color, bool centered) {
        sf::Text text(m_font, s, 8);
        text.setFillColor(color);
        float w = 8.f * static_cast<float>(s.size());
        text.setOrigin({centered ? w / 2.f : 0.f, m_fontTop + m_fontH / 2.f});
        text.setScale({m_scale, m_scale});
        text.setPosition(m_origin + mapPos * m_scale);
        target.draw(text);
    };

    // LOADING with moving dots (left-aligned so the text does not shift)
    int dots = static_cast<int>(m_time / 0.35f) % 4;               // "", ".", "..", "..."
    drawText("LOADING" + std::string(static_cast<size_t>(dots), '.'), {72.f, 170.f}, lavender, false);

    // Bar and percentage use the same number, so they always match
    int pct = std::min(100, static_cast<int>(progress()));

    const float barX = 32.f, barY = 184.f, barW = 160.f, barH = 10.f;
    sf::RectangleShape frame({barW * m_scale, barH * m_scale});
    frame.setPosition(m_origin + sf::Vector2f(barX, barY) * m_scale);
    frame.setFillColor(sf::Color(20, 20, 60));
    frame.setOutlineColor(lavender);
    frame.setOutlineThickness(-1.f * m_scale);
    target.draw(frame);

    float fillW = (barW - 4.f) * static_cast<float>(pct) / 100.f;
    if (fillW > 0.f) {
        sf::RectangleShape fill({fillW * m_scale, (barH - 4.f) * m_scale});
        fill.setPosition(m_origin + sf::Vector2f(barX + 2.f, barY + 2.f) * m_scale);
        fill.setFillColor(yellow);
        target.draw(fill);
    }

    drawText(std::to_string(pct) + "%", {112.f, 208.f}, yellow, true);
}
