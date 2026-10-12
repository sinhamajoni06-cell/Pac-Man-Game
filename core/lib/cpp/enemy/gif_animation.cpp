#include "enemy/gif_animation.h"

#include <fstream>
#include <iostream>
#include <iterator>

#include "stb_image.h"   // declarations only: the implementation is compiled in player.cpp

bool GifAnimation::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "[Ghost] Cannot open: " << path << "\n";
        return false;
    }
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)),
                                      std::istreambuf_iterator<char>());

    int* delaysMs = nullptr;
    int w = 0, h = 0, count = 0, comp = 0;
    unsigned char* data = stbi_load_gif_from_memory(
        bytes.data(), static_cast<int>(bytes.size()),
        &delaysMs, &w, &h, &count, &comp, 4);
    if (!data) {
        std::cerr << "[Ghost] Failed to decode GIF: " << path << "\n";
        return false;
    }

    frames.clear(); frightened.clear(); flashing.clear(); delays.clear();
    frames.reserve(count); frightened.reserve(count); flashing.reserve(count);

    auto addTexture = [](const sf::Image& img, std::vector<sf::Texture>& out) {
        sf::Texture t;
        if (!t.loadFromImage(img)) return false;
        out.push_back(t);
        return true;
    };

    const size_t frameSize = static_cast<size_t>(w) * h * 4;
    bool ok = true;
    for (int i = 0; i < count && ok; ++i) {
        sf::Image img(sf::Vector2u(static_cast<unsigned>(w), static_cast<unsigned>(h)),
                      data + i * frameSize);
        sf::Image blue = img, white = img;

        // Scared look: body becomes blue (or white when flashing), white eyes stay light
        for (unsigned y = 0; y < static_cast<unsigned>(h); ++y) {
            for (unsigned x = 0; x < static_cast<unsigned>(w); ++x) {
                sf::Color c = img.getPixel({x, y});
                if (c.a == 0) continue;
                bool eye = c.r > 200 && c.g > 200 && c.b > 200;
                blue.setPixel({x, y},  eye ? sf::Color(255, 184, 174) : sf::Color(33, 33, 255));
                white.setPixel({x, y}, eye ? sf::Color(255, 0, 0)     : sf::Color(255, 255, 255));
            }
        }

        ok = addTexture(img, frames) && addTexture(blue, frightened) && addTexture(white, flashing);

        float d = delaysMs ? delaysMs[i] / 1000.f : 0.1f;
        if (d < 0.02f) d = 0.1f;               // many GIFs store 0 delay
        delays.push_back(d);
    }

    stbi_image_free(data);
    if (delaysMs) stbi_image_free(delaysMs);
    if (!ok) std::cerr << "[Ghost] Texture failed: " << path << "\n";
    return ok && !frames.empty();
}