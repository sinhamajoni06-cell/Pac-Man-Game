#include "enemy/enemy_manager.h"

#include <algorithm>

EnemyManager::EnemyManager()
    : m_ghosts{&m_blinky, &m_pinky, &m_inky, &m_clyde} {}

bool EnemyManager::load(const std::string& dir) {
    for (Ghost* g : m_ghosts)
        if (!g->load(dir)) return false;
    return true;
}

void EnemyManager::setMap(const MapBox* box) {
    m_box = box;
    for (Ghost* g : m_ghosts) g->setMap(box);
}

void EnemyManager::reset() {
    m_phase = 0;
    m_phaseTimer = 0.f;
    m_frightTimer = 0.f;
    m_chain = 0;
    m_idleTimer = 0.f;
    for (Ghost* g : m_ghosts) { g->calm(); g->reset(); }
}

void EnemyManager::frighten(float seconds) {
    m_frightTimer = seconds;
    m_chain = 0;
    for (Ghost* g : m_ghosts) g->frighten();
}

EnemyResult EnemyManager::update(float dt, sf::Vector2f playerWindowPos, int playerDir, int dotsEaten) {
    EnemyResult result;
    sf::Vector2f playerMap = (playerWindowPos - m_origin) / m_scale;

    // 1. Scared mode, or the scatter/chase schedule (which pauses while scared)
    if (m_frightTimer > 0.f) {
        m_frightTimer -= dt;
        bool flash = m_frightTimer > 0.f && m_frightTimer < 2.f &&
                     (static_cast<int>(m_frightTimer / 0.2f) % 2 == 0);
        for (Ghost* g : m_ghosts) g->setFlash(flash);
        if (m_frightTimer <= 0.f)
            for (Ghost* g : m_ghosts) g->calm();
    } else if (m_phase < kPhaseCount) {
        m_phaseTimer += dt;
        if (m_phaseTimer >= kSchedule[m_phase]) {
            m_phaseTimer = 0.f;
            ++m_phase;
            for (Ghost* g : m_ghosts) g->reverse();       // ghosts turn around on every switch
        }
    }
    bool chase = (m_phase >= kPhaseCount) || (m_phase % 2 == 1);

    // 2. Leaving the house: Pinky at once, Inky after 30 dots, Clyde after 60
    Ghost* house[3] = {&m_pinky, &m_inky, &m_clyde};
    const int need[3] = {0, 30, 60};
    if (dotsEaten != m_lastDots) { m_lastDots = dotsEaten; m_idleTimer = 0.f; }
    else                         { m_idleTimer += dt; }

    bool someoneLeaving = false;
    for (Ghost* g : m_ghosts)
        if (g->state() == Ghost::State::Leaving) someoneLeaving = true;

    for (int i = 0; i < 3 && !someoneLeaving; ++i) {
        if (house[i]->state() == Ghost::State::InHouse && dotsEaten >= need[i]) {
            house[i]->release();
            someoneLeaving = true;
        }
    }

    if (m_idleTimer > 4.f && !someoneLeaving) {                              // no dots for a while: push one out
        for (int i = 0; i < 3; ++i) {
            if (house[i]->state() == Ghost::State::InHouse) { house[i]->release(); break; }
        }
        m_idleTimer = 0.f;
    }

    // 2b. Hitbox: two ghosts can't travel stacked in the same direction.
    //     The one behind slows down until there is a gap. Passing in opposite
    //     directions, or crossing at a junction, is still allowed (like the arcade).
    for (Ghost* g : m_ghosts) g->setSpeedLimit(0.f);
    for (int i = 0; i < 4; ++i) {
        for (int j = i + 1; j < 4; ++j) {
            Ghost* a = m_ghosts[i];
            Ghost* b = m_ghosts[j];
            if (a->state() != Ghost::State::Active || b->state() != Ghost::State::Active) continue;
            if (a->direction() != b->direction()) continue;              // passing or crossing: allowed

            sf::Vector2f d = b->position() - a->position();
            if (d.x * d.x + d.y * d.y >= kHitbox * kHitbox) continue;    // far enough apart

            // Who is behind, measured along the travel direction?
            static const float dirX[5] = {0.f, 0.f, 0.f, -1.f, 1.f};
            static const float dirY[5] = {0.f, -1.f, 1.f, 0.f, 0.f};
            int dir = a->direction();
            float along = d.x * dirX[dir] + d.y * dirY[dir];             // > 0: b is ahead of a

            Ghost* behind = (along > 0.f) ? a : b;                       // exact tie: b waits
            Ghost* front  = (along > 0.f) ? b : a;
            behind->setSpeedLimit(front->speed() * 0.8f);
        }
    }

    // 3. Move everyone
    GhostContext ctx;
    ctx.player.pos = playerMap;
    ctx.player.dir = playerDir;
    ctx.blinkyPos  = m_blinky.position();
    ctx.chase      = chase;
    for (Ghost* g : m_ghosts) g->update(dt, ctx);

    // 4. Collisions with Pac-Man
    for (Ghost* g : m_ghosts) {
        if (!g->canBeHit()) continue;
        sf::Vector2f d = g->position() - playerMap;
        if (d.x * d.x + d.y * d.y > 36.f) continue;       // closer than 6 map pixels

        if (g->frightened()) {
            result.eatenAt = g->position();
            g->getEaten();
            result.score += 200 << std::min(m_chain, 3);  // 200, 400, 800, 1600
            ++m_chain;
            ++result.ghostsEaten;
        } else {
            result.playerHit = true;
        }
    }
    return result;
}

void EnemyManager::draw(sf::RenderTarget& target) {
    if (!m_box) {
        for (Ghost* g : m_ghosts) g->draw(target, m_origin, m_scale);
        return;
    }

    // Clip to the maze so ghosts vanish into the tunnels (same as Pac-Man)
    sf::Vector2f ts(target.getSize());
    sf::FloatRect r = m_box->getMapRect();
    sf::View old = target.getView();

    sf::Vector2f p0(target.mapCoordsToPixel(r.position, old));
    sf::Vector2f p1(target.mapCoordsToPixel(r.position + r.size, old));

    sf::View clip(r);
    clip.setViewport(sf::FloatRect({p0.x / ts.x, p0.y / ts.y},
                                   {(p1.x - p0.x) / ts.x, (p1.y - p0.y) / ts.y}));
    target.setView(clip);
    for (Ghost* g : m_ghosts) g->draw(target, m_origin, m_scale);
    target.setView(old);
}