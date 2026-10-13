#include "enemy/ghost.h"

#include <algorithm>
#include <random>

namespace {
std::mt19937& rng() {
    static std::mt19937 gen{std::random_device{}()};
    return gen;
}
const int kOpposite[5] = {0, 2, 1, 4, 3};
}

Ghost::Ghost(const std::string& name, sf::Vector2f houseSpot)
    : m_name(name), m_house(houseSpot) {}

bool Ghost::load(const std::string& dir) {
    std::string base = dir;
    if (!base.empty() && base.back() != '/' && base.back() != '\\') base += '/';

    const char* names[5] = {"", "Up", "Down", "Left", "Right"};
    for (int d = 1; d <= 4; ++d) {
        std::string path = base + m_name + "/" + m_name + " (" + names[d] + ").gif";
        if (!m_anims[d].load(path)) return false;
    }
    m_sprite.emplace(m_anims[3].frames[0]);
    reset();
    return true;
}

// ---------------------------------------------------------------- state changes

void Ghost::startActive() {
    m_state  = State::Active;
    m_col    = 14;
    m_row    = 11;
    m_dir    = 3;                 // leaves heading left, 4 pixels past the door
    m_offset = 4.f;
    m_pos    = {112.f, 92.f};
}

void Ghost::reset() {
    m_frightened = false;
    m_flash = false;
    m_animFrame = 0;
    m_animTimer = 0.f;
    m_path.clear();
    m_pathIdx = 0;

    if (startsOutside()) {
        startActive();
    } else {
        m_state = State::InHouse;
        m_pos = m_house;
        m_dir = 1;
        m_bobDir = -1.f;
    }
}

void Ghost::release() {
    if (m_state != State::InHouse) return;
    m_state = State::Leaving;
    m_path = {{112.f, 116.f}, {112.f, 92.f}};   // to the middle, then up through the door
    m_pathIdx = 0;
}

void Ghost::reverse() {
    if (m_state != State::Active) return;
    m_col += kDX[m_dir];
    m_row += kDY[m_dir];
    m_dir = kOpposite[m_dir];
    m_offset = 8.f - m_offset;
}

void Ghost::frighten() {
    if (m_state == State::Eaten || m_state == State::Entering) return;
    m_frightened = true;
    m_flash = false;
    reverse();
}

void Ghost::getEaten() {
    m_frightened = false;
    m_flash = false;
    if (m_state == State::Active) {
        m_state = State::Eaten;                 // eyes run home along the maze
    } else {
        m_state = State::Entering;              // eaten inside the house: go straight back
        m_path = {{112.f, 116.f}};
        m_pathIdx = 0;
    }
}

// ---------------------------------------------------------------- movement

float Ghost::currentSpeed() const {
    if (m_state == State::Eaten) return kSpeedEyes;
    bool tunnel = (m_row == 14) && (m_col <= 4 || m_col >= 23);
    if (tunnel) return kSpeedTunnel;
    return m_frightened ? kSpeedFright : kSpeedNormal;
}

bool Ghost::followPath(float dist) {
    while (dist > 1e-5f && m_pathIdx < m_path.size()) {
        sf::Vector2f d = m_path[m_pathIdx] - m_pos;
        float len = std::hypot(d.x, d.y);
        if (len < 1e-4f) { ++m_pathIdx; continue; }

        if (std::fabs(d.x) > std::fabs(d.y)) m_dir = d.x < 0.f ? 3 : 4;
        else                                  m_dir = d.y < 0.f ? 1 : 2;

        float s = std::min(dist, len);
        m_pos += d / len * s;
        dist -= s;
        if (s >= len - 1e-4f) ++m_pathIdx;
    }
    return m_pathIdx >= m_path.size();
}

bool Ghost::stepOpen(int col, int row, int dir) const {
    return m_box && m_box->tileOpenMap(4.f + 8.f * col, 4.f + 8.f * row, kDX[dir], kDY[dir]);
}

// Called when the ghost arrives at a tile center: pick the direction to leave by
void Ghost::chooseDirection(const GhostContext& ctx) {
    const int back = kOpposite[m_dir];
    const bool scared = m_frightened && m_state == State::Active;

    sf::Vector2f target{0.f, 0.f};
    if (m_state == State::Eaten)  target = {13.5f, 11.f};              // above the door
    else if (!scared)             target = ctx.chase
                                         ? chaseTarget(ctx, {static_cast<float>(m_col), static_cast<float>(m_row)})
                                         : scatterTarget();

    // The arcade checks up, left, down, right: that order breaks ties
    static const int order[4] = {1, 3, 2, 4};
    // Four tiles where ghosts may not turn upward (above the house and in the lower middle)
    const bool noUpTile = (m_col == 12 || m_col == 15) && (m_row == 11 || m_row == 23);

    int allowed[4];
    int n = 0, best = 0;
    float bestDist = 1e9f;
    for (int d : order) {
        if (d == back || !stepOpen(m_col, m_row, d)) continue;
        if (d == 1 && noUpTile && m_state == State::Active && !scared) continue;
        allowed[n++] = d;

        float dx = static_cast<float>(m_col + kDX[d]) - target.x;
        float dy = static_cast<float>(m_row + kDY[d]) - target.y;
        float dist = dx * dx + dy * dy;
        if (dist < bestDist) { bestDist = dist; best = d; }
    }

    if (n == 0) {                                   // dead end: turn around
        m_dir = stepOpen(m_col, m_row, back) ? back : m_dir;
        return;
    }
    if (scared) {
        std::uniform_int_distribution<int> pick(0, n - 1);
        best = allowed[pick(rng())];
    }
    m_dir = best;
}

void Ghost::stepActive(float dist, const GhostContext& ctx) {
    while (dist > 1e-5f) {
        float step = std::min(dist, 8.f - m_offset);
        m_offset += step;
        dist -= step;

        if (m_offset >= 7.9999f) {                  // arrived at the next tile center
            m_col += kDX[m_dir];
            m_row += kDY[m_dir];
            m_offset = 0.f;

            // Tunnel: leaving one side comes out the other
            if (m_dir == 3 && m_col <= -2)      m_col = 29;
            else if (m_dir == 4 && m_col >= 29) m_col = -2;

            // Eyes reached the spot above the door: enter the house
            if (m_state == State::Eaten && m_row == 11 && (m_col == 13 || m_col == 14)) {
                m_pos = {4.f + 8.f * m_col, 4.f + 8.f * m_row};
                m_state = State::Entering;
                m_path = {{112.f, 92.f}, {112.f, 116.f}};
                m_pathIdx = 0;
                return;
            }
            chooseDirection(ctx);
        }
    }
    m_pos = sf::Vector2f(4.f + 8.f * m_col + static_cast<float>(kDX[m_dir]) * m_offset,
                         4.f + 8.f * m_row + static_cast<float>(kDY[m_dir]) * m_offset);
}

// ---------------------------------------------------------------- update / draw

void Ghost::update(float dt, const GhostContext& ctx) {
    switch (m_state) {
        case State::InHouse:
            m_pos.y += m_bobDir * kSpeedBob * dt;
            if (m_pos.y < m_house.y - 4.f) { m_pos.y = m_house.y - 4.f; m_bobDir =  1.f; }
            if (m_pos.y > m_house.y + 4.f) { m_pos.y = m_house.y + 4.f; m_bobDir = -1.f; }
            m_dir = m_bobDir < 0.f ? 1 : 2;
            break;

        case State::Leaving:
            if (followPath(kSpeedHouse * dt)) startActive();
            break;

        case State::Entering:
            if (followPath(kSpeedEyes * dt)) {      // back inside: come out again
                m_state = State::Leaving;
                m_path = {{112.f, 116.f}, {112.f, 92.f}};
                m_pathIdx = 0;
            }
            break;

        case State::Active:
        case State::Eaten:
            {
                float v = currentSpeed();
                if (m_speedLimit > 0.f) v = std::min(v, m_speedLimit);   // keep a gap to the ghost ahead
                stepActive(v * dt, ctx);
            }
            break;
    }

    // GIF animation
    const GifAnimation& a = m_anims[m_dir];
    if (!a.frames.empty()) {
        if (m_animFrame >= a.frames.size()) m_animFrame = 0;
        m_animTimer += dt;
        while (m_animTimer >= a.delays[m_animFrame]) {
            m_animTimer -= a.delays[m_animFrame];
            m_animFrame = (m_animFrame + 1) % a.frames.size();
        }
    }
}

void Ghost::draw(sf::RenderTarget& target, sf::Vector2f origin, float scale) {
    sf::Vector2f p = origin + m_pos * scale;
    const bool eyesOnly = (m_state == State::Eaten || m_state == State::Entering);

    if (eyesOnly) {
        // Eaten ghost: only the eyes, looking where it travels
        float r = 2.4f * scale, rp = 1.2f * scale;
        sf::CircleShape white(r), pupil(rp);
        white.setFillColor(sf::Color::White);
        pupil.setFillColor(sf::Color(33, 33, 255));
        white.setOrigin({r, r});
        pupil.setOrigin({rp, rp});
        sf::Vector2f look(static_cast<float>(kDX[m_dir]) * 1.2f * scale,
                          static_cast<float>(kDY[m_dir]) * 1.2f * scale);
        for (float sx : {-3.f, 3.f}) {
            sf::Vector2f eye = p + sf::Vector2f(sx * scale, -1.f * scale);
            white.setPosition(eye);
            pupil.setPosition(eye + look);
            target.draw(white);
            target.draw(pupil);
        }
        return;
    }

    if (!m_sprite) return;
    const GifAnimation& a = m_anims[m_dir];
    if (a.frames.empty()) return;

    size_t i = m_animFrame % a.frames.size();
    const sf::Texture& tex = m_frightened ? (m_flash ? a.flashing[i] : a.frightened[i]) : a.frames[i];

    m_sprite->setTexture(tex, true);
    sf::Vector2u sz = tex.getSize();
    m_sprite->setOrigin({sz.x / 2.f, sz.y / 2.f});
    m_sprite->setPosition(p);
    m_sprite->setScale({scale, scale});
    target.draw(*m_sprite);
}