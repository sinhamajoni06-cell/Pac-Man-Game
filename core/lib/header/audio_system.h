#pragma once
#include <SFML/Audio.hpp>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// One-shot sounds
enum class Sfx { Dot, EatGhost, Fruit, Death, ExtraLife, GameStart, Pause };

// Global sound system. Use it from anywhere:  Audio::get().play(Sfx::Fruit);
class Audio {
public:
    static Audio& get();
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    // dir = folder with dot_1.mp3, siren_1.mp3 ... Missing files only print a warning.
    bool load(const std::string& dir);

    void  play(Sfx sfx);                     // Sfx::Dot alternates dot_1 / dot_2 (waka-waka)
    float length(Sfx sfx) const;             // seconds (0 if the file is missing)

    // Background loop with arcade priority: eyes > power-up > siren.
    // Call every frame while playing; enabled=false silences the background.
    void updateBackground(bool enabled, bool eyes, bool frightened, int dotsLeft, int dotsTotal);

    void update();                           // call every frame (starts the pause beat)
    void stopAll();
    void setPaused(bool paused);
    void toggleMute();
    bool  muted()  const { return m_muted; }
    float volume() const { return m_volume; }

    // Saved sound settings (volume and mute), restored on the next launch
    void setSettingsFile(const std::string& path) { m_settingsPath = path; }
    bool loadSettings();                         // true if a saved file was found
    void saveSettings() const;               // 0..100
    void  changeVolume(float delta) { setVolume(m_volume + delta); }
    void setVolume(float percent, bool save = true);     // save = false while a slider is being dragged           // 0..100

private:
    Audio() = default;

    struct Track {
        sf::SoundBuffer buffer;
        std::optional<sf::Sound> sound;      // SFML 3 sounds need their buffer at creation
    };

    bool   loadTrack(const std::string& name, const std::string& path, bool loop);
    Track* find(const std::string& name) const;
    void   startBackground(const std::string& name);
    void   stopBackground();
    int    sirenCount() const;
    void   applyVolume();

    std::map<std::string, std::unique_ptr<Track>> m_tracks;
    std::string m_bg;                        // background loop that is playing now
    bool  m_dotToggle = false;
    bool  m_pauseMode = false;
    bool  m_muted = false;
    std::string m_settingsPath;
    float m_volume = 100.f;
    std::vector<std::string> m_pausedTracks;
};
