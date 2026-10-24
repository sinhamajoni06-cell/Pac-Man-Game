#include "audio_system.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

Audio& Audio::get() {
    static Audio instance;
    return instance;
}

bool Audio::loadTrack(const std::string& name, const std::string& path, bool loop) {
    auto t = std::make_unique<Track>();
    if (!t->buffer.loadFromFile(path)) {
        std::cerr << "[Audio] Cannot load: " << path << "\n";
        return false;
    }
    t->sound.emplace(t->buffer);
    t->sound->setLooping(loop);
    m_tracks[name] = std::move(t);
    return true;
}

bool Audio::load(const std::string& dir) {
    std::string base = dir;
    if (!base.empty() && base.back() != '/' && base.back() != '\\') base += '/';

    struct Item { const char* name; bool loop; };
    const Item items[] = {
        {"dot_1", false},   {"dot_2", false},     {"eat_ghost", false}, {"extra_life", false},
        {"eyes", true},     {"fruit", false},     {"game_start", false}, {"death", false},
        {"pause", false},   {"pause_beat", true}, {"power_up", true},
        {"siren_1", true},  {"siren_2", true},    {"siren_3", true},
        {"siren_4", true},  {"siren_5", true}
    };

    for (const Item& it : items) {
        std::string path = base + it.name + ".mp3";
        std::string n = it.name;
        // siren_4 / siren_5 are optional: skip quietly when the files are not there
        if ((n == "siren_4" || n == "siren_5") && !std::ifstream(path)) continue;
        loadTrack(n, path, it.loop);
    }
    applyVolume();
    return !m_tracks.empty();
}

Audio::Track* Audio::find(const std::string& name) const {
    auto it = m_tracks.find(name);
    return it == m_tracks.end() ? nullptr : it->second.get();
}

static const char* sfxName(Sfx s, bool dotFirst) {
    switch (s) {
        case Sfx::Dot:       return dotFirst ? "dot_1" : "dot_2";
        case Sfx::EatGhost:  return "eat_ghost";
        case Sfx::Fruit:     return "fruit";
        case Sfx::Death:     return "death";
        case Sfx::ExtraLife: return "extra_life";
        case Sfx::GameStart: return "game_start";
        case Sfx::Pause:     return "pause";
    }
    return "";
}

void Audio::play(Sfx sfx) {
    if (sfx == Sfx::Dot) m_dotToggle = !m_dotToggle;           // waka, waka, waka...
    Track* t = find(sfxName(sfx, m_dotToggle));
    if (t && t->sound) {
        t->sound->stop();
        t->sound->play();
    }
}

float Audio::length(Sfx sfx) const {
    Track* t = find(sfxName(sfx, true));
    return t ? t->buffer.getDuration().asSeconds() : 0.f;
}

// ---------------------------------------------------------------- background loops

void Audio::stopBackground() {
    if (m_bg.empty()) return;
    if (Track* t = find(m_bg)) t->sound->stop();
    m_bg.clear();
}

void Audio::startBackground(const std::string& name) {
    if (m_bg == name) return;                                   // already playing
    stopBackground();
    if (Track* t = find(name)) t->sound->play();
    m_bg = name;
}

int Audio::sirenCount() const {
    int n = 0;
    while (n < 5 && find("siren_" + std::to_string(n + 1))) ++n;
    return n;
}

void Audio::updateBackground(bool enabled, bool eyes, bool frightened, int dotsLeft, int dotsTotal) {
    if (!enabled) { stopBackground(); return; }
    if (eyes)       { startBackground("eyes");     return; }    // eaten ghosts running home
    if (frightened) { startBackground("power_up"); return; }    // ghosts are blue

    int n = sirenCount();
    if (n == 0) { stopBackground(); return; }

    // The siren gets faster as the dots run out (5 steps, spread over the files you have)
    int level = 4;
    if      (dotsLeft > dotsTotal * 2 / 3) level = 0;
    else if (dotsLeft > dotsTotal / 2)     level = 1;
    else if (dotsLeft > dotsTotal / 4)     level = 2;
    else if (dotsLeft > dotsTotal / 12)    level = 3;

    int index = std::min(n - 1, level * n / 5);
    startBackground("siren_" + std::to_string(index + 1));
}

// ---------------------------------------------------------------- pause, stop, volume

void Audio::update() {
    if (!m_pauseMode) return;

    // After the pause jingle has finished, loop the pause beat
    Track* jingle = find("pause");
    Track* beat   = find("pause_beat");
    bool jingleDone = !jingle || jingle->sound->getStatus() != sf::SoundSource::Status::Playing;
    if (beat && jingleDone && beat->sound->getStatus() != sf::SoundSource::Status::Playing)
        beat->sound->play();
}

void Audio::setPaused(bool paused) {
    if (paused) {
        m_pausedTracks.clear();
        for (auto& entry : m_tracks) {
            if (entry.first == "pause" || entry.first == "pause_beat") continue;
            Track& t = *entry.second;
            if (t.sound && t.sound->getStatus() == sf::SoundSource::Status::Playing) {
                t.sound->pause();
                m_pausedTracks.push_back(entry.first);
            }
        }
        m_pauseMode = true;
        play(Sfx::Pause);
    } else {
        m_pauseMode = false;
        if (Track* t = find("pause"))      t->sound->stop();
        if (Track* t = find("pause_beat")) t->sound->stop();
        for (const std::string& name : m_pausedTracks)
            if (Track* t = find(name)) t->sound->play();           // continue where it stopped
        m_pausedTracks.clear();
    }
}

void Audio::stopAll() {
    for (auto& entry : m_tracks)
        if (entry.second->sound) entry.second->sound->stop();
    m_bg.clear();
    m_pauseMode = false;
    m_pausedTracks.clear();
}

void Audio::applyVolume() {
    sf::Listener::setGlobalVolume(m_muted ? 0.f : m_volume);
}

void Audio::setVolume(float percent) {
    m_volume = std::clamp(percent, 0.f, 100.f);
    saveSettings();
    applyVolume();
}

void Audio::toggleMute() {
    m_muted = !m_muted;
    saveSettings();
    applyVolume();
}

bool Audio::loadSettings() {
    if (m_settingsPath.empty()) return false;
    std::ifstream f(m_settingsPath);
    float v = 0.f;
    if (!(f >> v)) return false;                  // no saved file yet: keep the defaults

    m_volume = std::clamp(v, 0.f, 100.f);
    int muted = 0;
    if (f >> muted) m_muted = (muted != 0);
    applyVolume();                                // applies without writing the file again
    return true;
}

void Audio::saveSettings() const {
    if (m_settingsPath.empty()) return;
    std::error_code ec;
    std::filesystem::path p(m_settingsPath);
    if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path(), ec);

    std::ofstream f(m_settingsPath);
    if (f) f << m_volume << "\n" << (m_muted ? 1 : 0) << "\n";
}
