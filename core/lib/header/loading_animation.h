#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

#include "enemy/gif_animation.h"

// The classic chase scene:
//  1. Pac-Man runs from right to left with Blinky right behind him
//  2. Pac-Man comes back from the left, grows, and chases a blue Blinky to the right
class LoadingAnimation {
public:
    LoadingAnimation() = default;
    LoadingAnimation(const LoadingAnimation&) = delete;
    LoadingAnimation& operator=(const LoadingAnimation&) = delete;

    // gameDir = "main/assets/graphic/game" (needs Pac-man/ and Blinky/ inside)
    bool load(const std::string& gameDir);
    void setTransform(sf::Vector2f origin, float scale) { m_origin = origin; m_scale = scale; }

    void start();                 // restarts the animation and builds a new fake loading curve
    void update(float dt) { m_time += dt; }
    void draw(sf::RenderTarget& target);

    bool finished() const;
    void setShowLoadingUi(bool show) { m_showUi = show; }   // false = plain cutscene

private:
    GifAnimation m_pacLeft, m_pacRight, m_blinkyLeft, m_blinkyRight;

    // LOADING text, bar and percentage
    sf::Font m_font;
    bool  m_fontLoaded = false;
    float m_fontTop = 0.f, m_fontH = 8.f;

    struct Step { float t0, burstStart, t1, p0, p1; };   // wait from t0 to burstStart, then jump to p1
    std::vector<Step> m_steps;
    void  buildProgress();
    float progress() const;                               // 0..100, uneven on purpose
    void  drawLoadingUi(sf::RenderTarget& target);
    static float totalTime();

    sf::Vector2f m_origin{0.f, 0.f};
    float m_scale = 1.f;
    float m_time = 0.f;
    bool  m_showUi = true;

    // Timing and movement, in map pixels (the maze is 224 wide) and seconds
    static constexpr float kSpeed    = 75.f;   // everyone runs at this speed
    static constexpr float kY        = 124.f;  // height of the chase on the screen
    static constexpr float kStartGap = 0.6f;   // empty screen at the start
    static constexpr float kMidGap   = 0.6f;   // empty screen between the two chases
    static constexpr float kEndGap   = 0.4f;   // empty screen at the end
    static constexpr float kBlinkyBehind = 34.f;  // Blinky chases this far behind Pac-Man
    static constexpr float kPacBehind    = 48.f;  // big Pac-Man chases this far behind Blinky
    static constexpr float kGrowDistance = 70.f;  // Pac-Man doubles in size over this many pixels

    static constexpr float kChase1 = (252.f + kBlinkyBehind + 24.f) / kSpeed;
    static constexpr float kChase2 = (224.f + 24.f + 72.f) / kSpeed;
};
