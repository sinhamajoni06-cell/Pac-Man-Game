#pragma once
#include <SFML/Graphics.hpp>
#include <random>
#include <string>
#include <vector>

// Slot machine that picks a random word: one reel per letter, reels stop left to right.
// Letters use the same Crackman fonts as the game title.
class SlotMachine {
public:
    SlotMachine() = default;
    SlotMachine(const SlotMachine&) = delete;
    SlotMachine& operator=(const SlotMachine&) = delete;

    // assetRoot = "main/assets" (needs TxT/name/crackman *.ttf), wordFile = "main/Data/word.txt"
    bool load(const std::string& assetRoot, const std::string& wordFile);
    void setTransform(sf::Vector2f origin, float scale) { m_origin = origin; m_scale = scale; }

    void start();                         // picks a random word and starts spinning
    void update(float dt);
    void draw(sf::RenderTarget& target);

    bool spinning() const { return m_started && !m_finished; }
    bool finished() const { return m_started && m_finished; }
    int  stoppedReels() const;
    const std::string& word() const { return m_word; }
    std::string randomWord();                           // a random word from word.txt (for the next rounds)   // the chosen word, uppercase

private:
    struct Reel {
        float pos = 0.f;        // which letter index is in the middle (float = scrolling)
        float speed = 0.f;      // letters per second while spinning
        float stopAt = 0.f;     // time when this reel starts to stop
        float landFrom = 0.f;
        float landTime = 0.f;
        int   landing = -1;     // letter index where it lands
        bool  stopping = false;
        bool  stopped = false;
    };

    char letterAt(int index, int reel) const;
    void drawGlyphs(sf::RenderTarget& t, const std::string& s, sf::Vector2f centerMap,
                    float capHeightMap, float maxWidthMap, sf::Color color);
    void drawBox(sf::RenderTarget& t, sf::Vector2f pos, sf::Vector2f size,
                 sf::Color fill, sf::Color outline, float thickness);

    sf::Font m_front, m_back;
    bool  m_hasBack = false;
    float m_capTop = 0.f, m_capH = 70.f;      // size of a capital letter at character size 100

    std::vector<std::string> m_words;
    std::string m_word;
    std::vector<Reel> m_reels;

    sf::Vector2f m_origin{0.f, 0.f};
    float m_scale = 1.f;

    bool  m_started = false;
    bool  m_finished = false;
    float m_time = 0.f;
    float m_winTime = 0.f;
    unsigned m_seed = 0;
    std::mt19937 m_rng{std::random_device{}()};
};
