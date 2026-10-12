#pragma once
#include <SFML/Graphics.hpp>
#include <string>

#include "enemy/blinky.h"
#include "enemy/pinky.h"
#include "enemy/inky.h"
#include "enemy/clyde.h"

struct EnemyResult {
    bool playerHit    = false;   // a ghost caught Pac-Man
    int  ghostsEaten  = 0;       // ghosts Pac-Man ate this frame
    int  score        = 0;       // points for those ghosts (200, 400, 800, 1600)
};

// Owns the four ghosts: scatter/chase timing, scared mode, house release, collisions.
class EnemyManager {
public:
    EnemyManager();
    EnemyManager(const EnemyManager&) = delete;
    EnemyManager& operator=(const EnemyManager&) = delete;

    // `dir` is the game asset folder that contains Blinky/, Pinky/, Inky/, Clyde/
    bool load(const std::string& dir);
    void setMap(const MapBox* box);
    void setTransform(sf::Vector2f origin, float scale) { m_origin = origin; m_scale = scale; }

    void reset();                              // everyone back to the start
    void frighten(float seconds = 6.f);        // call when a power pellet is eaten

    // playerWindowPos: Pac-Man's center in window coordinates
    // playerDir: 1 up, 2 down, 3 left, 4 right
    // dotsEaten: how many dots have been eaten so far (releases Inky and Clyde)
    EnemyResult update(float dt, sf::Vector2f playerWindowPos, int playerDir, int dotsEaten);

    void draw(sf::RenderTarget& target);

private:
    static constexpr int   kPhaseCount = 7;
    static constexpr float kSchedule[kPhaseCount] = {7.f, 20.f, 7.f, 20.f, 5.f, 20.f, 5.f};

    Blinky m_blinky;
    Pinky  m_pinky;
    Inky   m_inky;
    Clyde  m_clyde;
    Ghost* m_ghosts[4];

    const MapBox* m_box = nullptr;
    sf::Vector2f  m_origin{0.f, 0.f};
    float         m_scale = 1.f;

    int   m_phase = 0;          // index into the schedule (even = scatter, odd = chase)
    float m_phaseTimer = 0.f;
    float m_frightTimer = 0.f;
    int   m_chain = 0;          // ghosts eaten in this scared period
    int   m_lastDots = 0;
    float m_idleTimer = 0.f;    // time since the last dot was eaten
};