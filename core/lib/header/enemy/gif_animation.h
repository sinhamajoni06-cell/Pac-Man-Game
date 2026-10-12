#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// One animated GIF, decoded into textures.
// Also builds a blue and a white copy of every frame for the scared look.
struct GifAnimation {
    std::vector<sf::Texture> frames;       // normal colors
    std::vector<sf::Texture> frightened;   // blue ghost
    std::vector<sf::Texture> flashing;     // white ghost (end of scared time)
    std::vector<float>       delays;       // seconds per frame

    bool load(const std::string& path);
    bool empty() const { return frames.empty(); }
};