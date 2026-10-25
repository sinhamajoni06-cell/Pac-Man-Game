#pragma once
#include <SFML/Graphics.hpp>
#include <random>
#include <string>
#include <vector>

#include "eat_system.h"

// What the player just picked up
struct WordPickup {
    char letter = 0;                // 0 = nothing collected
    bool decoy  = false;            // a wrong letter (costs points)
    sf::Vector2f pos{0.f, 0.f};     // where it was (map pixels)
};

// Word Finding mode: the letters of the chosen word replace some food dots,
// plus a few wrong letters. Pac-Man collects the real ones (any order) to win.
class WordMode {
public:
    WordMode() = default;
    WordMode(const WordMode&) = delete;
    WordMode& operator=(const WordMode&) = delete;

    bool load(const std::string& fontPath);                 // PressStart2P-Regular.ttf
    void setTransform(sf::Vector2f origin, float scale) { m_origin = origin; m_scale = scale; }

    // Starts a round. Call right after eatSystem.reset(): takes one food dot per letter.
    // decoys = number of wrong letters mixed in.
    void start(const std::string& word, EatSystem& eat, sf::Vector2f avoidMapPos, int decoys = 0);

    // Returns what was collected this frame (letter == 0 means nothing)
    WordPickup update(float dt, sf::Vector2f playerWindowPos, bool canCollect);

    void drawLetters(sf::RenderTarget& target);             // the letters in the maze
    void drawWordBar(sf::RenderTarget& target);             // the word at the top of the window

    bool complete() const { return m_realTotal > 0 && m_collected == m_realTotal; }
    const std::string& word() const { return m_word; }
    int   collected() const { return m_collected; }
    int   total() const { return m_realTotal; }
    float playTime() const { return m_playTime; }           // seconds spent collecting
    int   timeBonus() const;                                // points for finishing fast

private:
    struct Letter { char c = 0; bool decoy = false; sf::Vector2f pos{0.f, 0.f}; bool taken = false; };

    void drawChar(sf::RenderTarget& target, char c, sf::Vector2f mapPos, sf::Color color);

    sf::Font m_font;
    bool  m_loaded = false;
    float m_top = 0.f, m_h = 8.f;

    std::string m_word;
    std::vector<char> m_slots;
    std::vector<bool> m_filled;
    std::vector<Letter> m_letters;
    int   m_collected = 0;
    int   m_realTotal = 0;

    sf::Vector2f m_origin{0.f, 0.f};
    float m_scale = 1.f;
    float m_time = 0.f;
    float m_playTime = 0.f;
    float m_doneTime = 0.f;
    std::mt19937 m_rng{std::random_device{}()};
};
