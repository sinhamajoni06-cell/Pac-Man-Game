#include "player.h"

#include <fstream>
#include <iostream>
#include <iterator>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_GIF
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif
#include "stb_image.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

bool Player::loadGif(const std::string& path, Animation& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "[Player] Cannot open: " << path << "\n";
        return false;
    }
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)),
                                      std::istreambuf_iterator<char>());

    int* delays = nullptr;
    int w = 0, h = 0, frameCount = 0, comp = 0;
    unsigned char* data = stbi_load_gif_from_memory(
        bytes.data(), static_cast<int>(bytes.size()),
        &delays, &w, &h, &frameCount, &comp, 4);   // force RGBA
    if (!data) {
        std::cerr << "[Player] Failed to decode GIF: " << path << "\n";
        return false;
    }

    out.frames.clear();
    out.delays.clear();
    out.frames.reserve(frameCount);

    const size_t frameSize = static_cast<size_t>(w) * h * 4;
    for (int i = 0; i < frameCount; ++i) {
        sf::Image img(sf::Vector2u(static_cast<unsigned>(w), static_cast<unsigned>(h)),
                      data + i * frameSize);
        sf::Texture tex;
        if (!tex.loadFromImage(img)) {
            std::cerr << "[Player] Texture failed: " << path << "\n";
            stbi_image_free(data);
            if (delays) stbi_image_free(delays);
            return false;
        }
        out.frames.push_back(tex);

        float d = delays ? delays[i] / 1000.f : 0.1f;
        if (d < 0.02f) d = 0.1f;                   // many GIFs store 0 delay
        out.delays.push_back(d);
    }

    stbi_image_free(data);
    if (delays) stbi_image_free(delays);
    return !out.frames.empty();
}

bool Player::load(const std::string& dir) {
    std::string base = dir;
    if (!base.empty() && base.back() != '/' && base.back() != '\\') base += '/';

    bool ok = loadGif(base + "Pac-Man (Up).gif",    m_up)
           && loadGif(base + "Pac-Man (Down).gif",  m_down)
           && loadGif(base + "Pac-Man (Left).gif",  m_left)
           && loadGif(base + "Pac-Man (Right).gif", m_right);
    if (!ok) return false;

    m_sprite.emplace(m_right.frames[0]);
    sf::Vector2u size = m_right.frames[0].getSize();
    m_sprite->setOrigin({size.x / 2.f, size.y / 2.f});   // position = center

    m_lastDir = Direction::None;
    m_frame = 0;
    m_timer = 0.f;
    return true;
}

Player::Animation& Player::current() {
    switch (m_dir) {
        case Direction::Up:   return m_up;
        case Direction::Down: return m_down;
        case Direction::Left: return m_left;
        default:              return m_right;
    }
}

void Player::setDirection(Direction d) {
    if (d == Direction::None) return;
    m_wanted = d;
    m_moving = true;
    if (!m_box) m_dir = d;      // with a map, the turn happens in update() when the way is free
}

void Player::handleInput() {
    namespace K = sf::Keyboard;
    if      (K::isKeyPressed(K::Key::Up)    || K::isKeyPressed(K::Key::W)) setDirection(Direction::Up);
    else if (K::isKeyPressed(K::Key::Down)  || K::isKeyPressed(K::Key::S)) setDirection(Direction::Down);
    else if (K::isKeyPressed(K::Key::Left)  || K::isKeyPressed(K::Key::A)) setDirection(Direction::Left);
    else if (K::isKeyPressed(K::Key::Right) || K::isKeyPressed(K::Key::D)) setDirection(Direction::Right);
}

namespace {
sf::Vector2f dirVec(Direction d) {
    switch (d) {
        case Direction::Up:    return {0.f, -1.f};
        case Direction::Down:  return {0.f,  1.f};
        case Direction::Left:  return {-1.f, 0.f};
        case Direction::Right: return {1.f,  0.f};
        default:               return {0.f,  0.f};
    }
}
}

void Player::update(float dt) {
    if (!m_sprite) return;

    bool advancing = false;

    if (m_moving) {
        if (m_box) {
            // 1. Turn as soon as the wanted direction is free (key press is remembered)
            if (m_wanted != m_dir) {
                sf::Vector2f p = m_pos;
                if (m_box->tryTurn(p, dirVec(m_wanted))) {
                    m_pos = p;
                    m_dir = m_wanted;
                }
            }
            // 2. Move forward until a wall stops us
            advancing = m_box->move(m_pos, dirVec(m_dir) * (m_speed * dt));
        } else {
            m_pos += dirVec(m_dir) * (m_speed * dt);
            advancing = true;
        }
    }

    Animation& anim = current();
    if (anim.frames.empty()) return;

    if (m_dir != m_lastDir) {          // direction changed -> switch GIF
        m_lastDir = m_dir;
        if (m_frame >= anim.frames.size()) m_frame = 0;
        m_timer = 0.f;
    }

    if (advancing) {                   // animation freezes while blocked by a wall
        m_timer += dt;
        while (m_timer >= anim.delays[m_frame]) {
            m_timer -= anim.delays[m_frame];
            m_frame = (m_frame + 1) % anim.frames.size();
        }
    }

    m_sprite->setTexture(anim.frames[m_frame], true);
    m_sprite->setPosition(m_pos);
    m_sprite->setScale({m_scale, m_scale});
}

void Player::draw(sf::RenderTarget& target) {
    if (m_sprite) target.draw(*m_sprite);
}
