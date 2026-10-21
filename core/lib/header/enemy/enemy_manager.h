#pragma once
#include <SFML/Graphics.hpp>
#include <string>

#include "enemy/blinky.h"
#include "enemy/pinky.h"
#include "enemy/inky.h"
#include "enemy/clyde.h"
#include "level_config.h"

struct EnemyResult {
    bool playerHit   = false;   // a ghost caught Pac-Man
    int  ghostsEaten = 0;       // ghosts Pac-Man ate this frame
    int  score       = 0;       // points for those ghosts (200, 400, 800, 1600)
    sf::Vector2f eatenAt{0.f, 0.f};   // where the last ghost was eaten (map pixels)
};

// Owns the four ghosts: scatter/chase timing, scared mode, house release, collisions.
class EnemyManager {
public:
    EnemyManager();
    EnemyManager(const EnemyManager&) = delete;
    EnemyManager& operator=(const EnemyManager&) = delete;

    bool load(const std::string& dir);
    void setMap(const MapBox* box);
    void setTransform(sf::Vector2f origin, float scale) { m_origin = origin; m_scale = scale; }

    void reset();                                          // everyone back to the start
    void markNewLevel() { m_afterDeath = false; m_counterBase = 0; }
    void markDeath(int dotsEaten) { m_afterDeath = true; m_counterBase = dotsEaten; }

    // Call every frame: speeds, scatter/chase timing and Blinky's speed-up for this level
    void setDifficulty(int level, const LevelConfig& cfg, float topSpeed, int dotsLeft);

    void frighten(float seconds, int flashes = 5);         // power pellet eaten
    void hideEaten(bool hide);                             // hides the eaten ghost during the freeze

    // playerWindowPos: Pac-Man's center in window coordinates
    // playerDir: 1 up, 2 down, 3 left, 4 right
    // dotsEaten: dots eaten so far this level
    EnemyResult update(float dt, sf::Vector2f playerWindowPos, int playerDir, int dotsEaten);

    void draw(sf::RenderTarget& target);

    // For the background sound
    bool anyFrightened() const { return m_frightTimer > 0.f; }
    bool anyEaten() const {                                // eyes running back to the house
        for (const Ghost* g : m_ghosts)
            if (g->state() == Ghost::State::Eaten || g->state() == Ghost::State::Entering) return true;
        return false;
    }

private:
    static constexpr int   kPhaseCount = 7;
    static constexpr float kHitbox = 8.f;   // ghosts closer than this (map pixels) can't travel stacked

    Blinky m_blinky;
    Pinky  m_pinky;
    Inky   m_inky;
    Clyde  m_clyde;
    Ghost* m_ghosts[4];

    const MapBox* m_box = nullptr;
    sf::Vector2f  m_origin{0.f, 0.f};
    float         m_scale = 1.f;

    LevelConfig m_cfg;
    int   m_level = 1;
    int   m_phase = 0;          // index into the schedule (even = scatter, odd = chase)
    float m_phaseTimer = 0.f;
    float m_frightTimer = 0.f;
    float m_flashTime = 2.f;    // how long the blue ghosts flash at the end
    int   m_chain = 0;          // ghosts eaten in this scared period
    int   m_lastDots = 0;
    float m_idleTimer = 0.f;    // time since the last dot was eaten
    bool  m_afterDeath = false; // after a death, ghosts use a separate dot counter
    int   m_counterBase = 0;
};
