#include "loading_animation.h"

#include <algorithm>
#include <cmath>
#include <iostream>
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

    target.setView(old);
}
