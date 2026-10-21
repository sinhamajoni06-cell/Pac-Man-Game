#pragma once
#include <algorithm>

// Difficulty of one level (speeds are fractions of the top speed, 75 map pixels per second)
struct LevelConfig {
    float pacSpeed = 0.8f, pacFrightSpeed = 0.9f;
    float ghostSpeed = 0.75f, ghostTunnelSpeed = 0.4f, ghostFrightSpeed = 0.5f;
    int   elroy1Dots = 20, elroy2Dots = 10;             // dots left when Blinky speeds up
    float elroy1Speed = 0.8f, elroy2Speed = 0.85f;
    float frightTime = 6.f;                             // seconds the ghosts stay blue (0 = never)
    int   flashes = 5;
    float schedule[7] = {7.f, 20.f, 7.f, 20.f, 5.f, 20.f, 5.f};   // scatter, chase, ... then chase forever
};

inline LevelConfig levelConfig(int level) {
    static const float fright[21] = {6, 5, 4, 3, 2, 5, 2, 2, 1, 5, 2, 1, 1, 3, 1, 1, 0, 1, 0, 0, 0};
    static const int   elroy[21]  = {20, 30, 40, 40, 40, 50, 50, 50, 60, 60, 60, 80, 80, 80,
                                     100, 100, 100, 100, 120, 120, 120};
    const int L = std::max(1, level);
    const int i = std::min(L, 21) - 1;

    LevelConfig c;

    // Pac-Man
    if (L == 1)       { c.pacSpeed = 0.80f; c.pacFrightSpeed = 0.90f; }
    else if (L <= 4)  { c.pacSpeed = 0.90f; c.pacFrightSpeed = 0.95f; }
    else if (L <= 20) { c.pacSpeed = 1.00f; c.pacFrightSpeed = 1.00f; }
    else              { c.pacSpeed = 0.90f; c.pacFrightSpeed = 0.90f; }

    // Ghosts and Blinky's speed-up
    if (L == 1) {
        c.ghostSpeed = 0.75f; c.ghostTunnelSpeed = 0.40f; c.ghostFrightSpeed = 0.50f;
        c.elroy1Speed = 0.80f; c.elroy2Speed = 0.85f;
    } else if (L <= 4) {
        c.ghostSpeed = 0.85f; c.ghostTunnelSpeed = 0.45f; c.ghostFrightSpeed = 0.55f;
        c.elroy1Speed = 0.90f; c.elroy2Speed = 0.95f;
    } else {
        c.ghostSpeed = 0.95f; c.ghostTunnelSpeed = 0.50f; c.ghostFrightSpeed = 0.60f;
        c.elroy1Speed = 1.00f; c.elroy2Speed = 1.05f;
    }
    c.elroy1Dots = elroy[i];
    c.elroy2Dots = elroy[i] / 2;

    // Scared time
    c.frightTime = fright[i];
    c.flashes = c.frightTime >= 2.f ? 5 : 3;

    // Scatter / chase timing
    if (L >= 2) { c.schedule[5] = 1033.f; c.schedule[6] = 0.02f; }
    if (L >= 5) { c.schedule[0] = 5.f; c.schedule[2] = 5.f; c.schedule[5] = 1037.f; }
    return c;
}
