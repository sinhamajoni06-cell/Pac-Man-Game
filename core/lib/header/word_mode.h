#pragma once
#include <SFML/Graphics.hpp>
#include <random>
#include <string>
#include <vector>

#include "eat_system.h"

// Word Finding mode: the letters of the chosen word replace some food dots.
// Pac-Man collects them (any order) to win the round.
class WordMode {
public:
    WordMode() = default;
    WordMode(const WordMode&) = delete;
    WordMode& operator=(const WordMode&) = delete;

    bool load(const std::string& fontPath);                 // PressStart2P-Regular.ttf
    void setTransform(sf::Vector2f origin, float scale) { m_origin = origin; m_scale = scale; }

    // Starts a round. Call right after eatSystem.reset(): takes one food dot per letter.
    void start(const std::string& word, EatSystem& eat, sf::Vector2f avoidMapPos);

    // Returns the letter collected this frame (0 = none)
    char update(float dt, sf::Vector2f playerWindowPos, bool canCollect);

    void drawLetters(sf::RenderTarget& target);             // the letters in the maze
    void drawWordBar(sf::RenderTarget& target);             // the word at the top of the window

    bool complete() const { return !m_letters.empty() && m_collected == static_cast<int>(m_letters.size()); }
    const std::string& word() const { return m_word; }
    int  collected() const { return m_collected; }
    int  total() const { return static_cast<int>(m_letters.size()); }

private:
    struct Letter { char c; sf::Vector2f pos; bool taken = false; };

    void drawChar(sf::RenderTarget& target, char c, sf::Vector2f mapPos, sf::Color color);

    sf::Font m_font;
    bool  m_loaded = false;
    float m_top = 0.f, m_h = 8.f;

    std::string m_word;
    std::vector<char> m_slots;
    std::vector<bool> m_filled;
    std::vector<Letter> m_letters;
    int   m_collected = 0;

    sf::Vector2f m_origin{0.f, 0.f};
    float m_scale = 1.f;
    float m_time = 0.f;
    float m_doneTime = 0.f;
    std::mt19937 m_rng{std::random_device{}()};
};
