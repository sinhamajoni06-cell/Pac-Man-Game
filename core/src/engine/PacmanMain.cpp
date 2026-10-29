#include <SFML/Graphics.hpp>
#include <optional>
#include "player.h"
#include "map.h"
#include "map_box.h"
#include "eat_system.h"
#include "enemy/enemy_manager.h"
#include "hud.h"
#include "pickup_system.h"
#include "title_screen.h"
#include "audio_system.h"
#include "level_config.h"
#include "loading_animation.h"
#include <algorithm>
#include <fstream>
#include <string>
#include "ui_panel.h"
#include "menus.h"
#include "slot_machine.h"
#include <filesystem>
#include "word_mode.h"
#include <iostream>
#include <sstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

// Keeps the 800x600 game world in the right proportions when the window is
// resized or maximized (black bars appear if the window shape is different)
static void applyLetterbox(sf::RenderWindow& window, sf::Vector2u size)
{
    const float worldW = 800.f, worldH = 600.f;
    float windowRatio = static_cast<float>(size.x) / static_cast<float>(size.y);
    float worldRatio  = worldW / worldH;

    sf::FloatRect viewport({0.f, 0.f}, {1.f, 1.f});
    if (windowRatio > worldRatio) {            // window is wider: bars left and right
        viewport.size.x     = worldRatio / windowRatio;
        viewport.position.x = (1.f - viewport.size.x) / 2.f;
    } else {                                   // window is taller: bars top and bottom
        viewport.size.y     = windowRatio / worldRatio;
        viewport.position.y = (1.f - viewport.size.y) / 2.f;
    }

    sf::View view(sf::FloatRect({0.f, 0.f}, {worldW, worldH}));
    view.setViewport(viewport);
    window.setView(view);
}

static void showFatalError(const std::string& text)
{
#ifdef _WIN32
    MessageBoxA(nullptr, text.c_str(), "Word-Man", MB_OK | MB_ICONERROR);
#else
    std::cerr << text << std::endl;
#endif
}

static int gameMain();

int main()
{
    // Everything the game prints to std::cerr goes into error.log (the console is hidden)
    std::ofstream errorLog("error.log");
    std::streambuf* oldBuf = std::cerr.rdbuf(errorLog.rdbuf());

    int rc = 1;
    try {
        rc = gameMain();
    } catch (const std::exception& e) {
        std::cerr << "[Fatal] " << e.what() << "\n";
    } catch (...) {
        std::cerr << "[Fatal] Unknown error\n";
    }

    std::cerr.flush();
    std::cerr.rdbuf(oldBuf);
    errorLog.close();

    if (rc != 0) {
        std::ifstream in("error.log");
        std::stringstream ss;
        ss << in.rdbuf();
        std::string msg = ss.str();
        if (msg.size() > 1200) msg.resize(1200);
        if (msg.empty()) msg = "The game could not start.";
        showFatalError("The game stopped because of an error:\n\n" + msg);
    }
    {   // nothing went wrong: do not leave an empty error.log behind
        std::error_code ec;
        if (std::filesystem::exists("error.log", ec) && std::filesystem::file_size("error.log", ec) == 0)
            std::filesystem::remove("error.log", ec);
    }
    return rc;
}

static int gameMain()
{
    {   // user data folder; copy saves from the old place on the first run
        namespace fs = std::filesystem;
        std::error_code ec;
        fs::create_directories("main/Data/User_data", ec);
        for (const char* name : {"highscore.txt", "settings.txt"}) {
            fs::path oldPath = fs::path("main/assets/TxT") / name;
            fs::path newPath = fs::path("main/Data/User_data") / name;
            if (fs::exists(oldPath, ec) && !fs::exists(newPath, ec)) fs::copy_file(oldPath, newPath, ec);
        }
    }

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Word-Man");
    window.setFramerateLimit(60);

    sf::Image icon;
    if (icon.loadFromFile("main/assets/graphic/game/logo.png"))
        window.setIcon(icon.getSize(), icon.getPixelsPtr());

    bool fullscreen = false;
    bool autoFullscreen = true;       // saved: stays windowed if you chose that last time
    {
        std::ifstream f("main/assets/TxT/settings.txt");
        int v = 1;
        if (f >> v) autoFullscreen = (v != 0);
    }
    auto setFullscreen = [&](bool value) {
        fullscreen = value;
        if (fullscreen)
            window.create(sf::VideoMode::getDesktopMode(), "Word-Man", sf::State::Fullscreen);
        else
            window.create(sf::VideoMode({800, 600}), "Word-Man", sf::Style::Default, sf::State::Windowed);
        window.setFramerateLimit(60);
        if (icon.getSize().x > 0) window.setIcon(icon.getSize(), icon.getPixelsPtr());
        applyLetterbox(window, window.getSize());      // keep the 4:3 game area centered
    };

    Map gameMap;
    if (!gameMap.load("main/assets/graphic/game"))
        return 1;
    gameMap.fitWithMargins({800, 600}, 24.f, 16.f);   // 3 HUD rows on top, 2 at the bottom

    EatSystem eatSystem;
    if (!eatSystem.load("main/assets/graphic/game"))
        return 1;
    eatSystem.setTransform(gameMap.getPosition(), gameMap.getScale());

    MapBox mapBox;
    if (!mapBox.load("main/assets/graphic/game"))
        return 1;
    mapBox.setTransform(gameMap.getPosition(), gameMap.getScale());
    mapBox.setHitboxSize(6.f);

    const int totalDots = eatSystem.foodLeft() + eatSystem.powerLeft();

    EnemyManager enemies;
    if (!enemies.load("main/assets/graphic/game"))    // folders: Blinky, Pinky, Inky, Clyde
        return 1;
    enemies.setMap(&mapBox);
    enemies.setTransform(gameMap.getPosition(), gameMap.getScale());
    enemies.reset();        // Pac-Man's hitbox, in map pixels

    PickupSystem pickups;
    if (!pickups.load("main/assets/graphic/game/pickups"))
        return 1;
    pickups.setTransform(gameMap.getPosition(), gameMap.getScale());
    int level = 1;

    Hud hud;
    if (!hud.load("main/assets/TxT/PressStart2P-Regular.ttf"))
        return 1;
    hud.setTransform(gameMap.getPosition(), gameMap.getScale());
    hud.loadHighScore("main/Data/User_data/highscore.txt");

    TitleScreen title;
    if (!title.load("main/assets"))
        return 1;
    title.setTransform(gameMap.getPosition(), gameMap.getScale());
    title.setTitle("WORD-MAN");

    LoadingAnimation loading;
    if (!loading.load("main/assets/graphic/game"))
        return 1;
    loading.setTransform(gameMap.getPosition(), gameMap.getScale());
    loading.start();
    bool  skipLoading = false;
    float startLock = 0.f;      // blocks the start button for a moment after the intro is skipped
    title.reset();

    Player player;
    // change this path to where your GIF folder really is
    if (!player.load("main/assets/graphic/game/Pac-man"))
        return 1;
    // Start spot: center of the map, in the row below the ghost house (map pixel 113, 188)
    player.setPosition(gameMap.getPosition() + sf::Vector2f(112.f, 188.f) * gameMap.getScale());
    player.setMapBox(&mapBox);
    if (!hud.loadLifeIcon("main/assets/graphic/game/Pac-man/Pac Man.png"))
        hud.setLifeTexture(player.lifeTexture());   // fallback if the image is missing
    mapBox.setWrapMargin(player.getHalfWidth() / gameMap.getScale());   // jump only when fully hidden
    player.setDirection(Direction::Left);   // already moving left at launch
    player.setScale(gameMap.getScale());   // sprite scaled together with the maze
    player.setSpeed(60.f * gameMap.getScale());   // arcade speed: ~60 map pixels per second

    Audio& audio = Audio::get();
    audio.load("main/assets/audio/SFX");     // missing files only print a warning
    bool paused = false;
    bool extraLifeGiven = false;
    bool  passVisible = false;            // the LEVEL CLEAR screen is showing
    bool  nextLevelRequested = false;     // NEXT LEVEL was chosen
    float volumeToast = 0.f;
    bool  showInfo = false;
    bool  showModeSelect = false;     // the SELECT MODE screen is showing
    bool  modeComingSoon = false;     // "COMING SOON!" is showing
    bool  classicRequested = false;   // CLASSIC was chosen           // the INFO screen is showing              // time left for the little volume box

    // Menu screens (buttons are clickable with the mouse)
    if (!Panel::loadFont("main/assets/TxT/PressStart2P-Regular.ttf"))
        return 1;
    Panel titlePanel, hudPanel, pausePanel, deathPanel, passPanel, infoPanel, modePanel, slotPanel, spinPanel, quitPanel, settingsPanel;
    for (Panel* p : {&titlePanel, &hudPanel, &pausePanel, &deathPanel, &passPanel, &infoPanel, &modePanel, &slotPanel, &spinPanel, &quitPanel, &settingsPanel})
        p->setTransform(gameMap.getPosition(), gameMap.getScale());
    Menus::buildTitle(titlePanel);
    Menus::buildHudButtons(hudPanel);
    Menus::buildInfo(infoPanel);

    SlotMachine slot;
    if (!slot.load("main/assets", "main/Data/word.txt"))
        return 1;
    slot.setTransform(gameMap.getPosition(), gameMap.getScale());
    bool showWordSlot = false;
    bool showQuitConfirm = false;
    bool showSettings = false;        // the settings screen (home page) is showing
    auto refreshSettings = [&]() {    // rebuild it after the volume or mute changed
        if (showSettings) Menus::buildSettings(settingsPanel, audio.volume() / 100.f, audio.muted());
    };     // the "Do you want to exit?" screen is showing

    WordMode wordMode;
    if (!wordMode.load("main/assets/TxT/PressStart2P-Regular.ttf"))
        return 1;
    wordMode.setTransform(gameMap.getPosition(), gameMap.getScale());
    bool        wordModeActive = false;       // the current game is Word Finding
    bool        wordModeRequested = false;    // PLAY was pressed on the slot screen
    bool        wordRoundNeeded = false;      // scatter new letters when the round is ready
    std::string roundWord;
    int  wordBonus = 0;               // time bonus for the word that was just found
    auto decoyCount = [](int lvl) { return std::min(5, std::max(0, lvl - 1)); };   // wrong letters: 0 on level 1 ... 5
    auto highScorePath = [&]() -> std::string {                                   // each mode has its own high score
        return wordModeActive ? "main/Data/User_data/highscore_word.txt"
                              : "main/assets/TxT/highscore.txt";
    };
    bool        wantSpin = false;             // show the spin before the next word
    bool        skipSpin = false;             // BACK / Enter / Space / Esc skips the spin
    float       spinDoneTimer = 0.f;          // how long the finished word has been showing
    auto wordLength = [](int lvl) { return std::min(10, 4 + lvl); };   // level 1 = 5 letters ... level 6+ = 10                    // word for the next round ("" = pick a random one)        // the slot machine screen is showing

    // Sound settings from the last run (volume and mute)
    audio.setSettingsFile("main/Data/User_data/sound_settings.txt");
    audio.loadSettings();

    float ghostFreeze = 0.f;          // game freezes for a moment when a ghost is eaten
    float eatPause = 0.f;             // tiny pause after every dot
    bool  cutscenePending = false;    // play the intermission cutscene before the next level

    sf::Clock clock;

    // Playing -> Caught (freeze) -> Dying (animation) -> Respawn (pause) -> Playing
    enum class GameState { Loading, Playing, Caught, Dying, Respawn, LevelClear, Ready, GameOver, Title, DeathScreen, Cutscene, WordSpin };
    GameState gameState = GameState::Loading;
    float stateTimer = 0.f;
    int   lives = 3;
    bool  firstStart = true;                       // "PLAYER ONE" only on a new game
    GameState lastState = GameState::Playing;
    bool  levelFlashStarted = false;

    auto spawnPlayer = [&]() {
        player.setPosition(gameMap.getPosition() + sf::Vector2f(112.f, 188.f) * gameMap.getScale());
        player.setDirection(Direction::Left);
    };

    auto startNewGame = [&]() {
        audio.stopAll();
        audio.play(Sfx::GameStart);
        eatSystem.reset();                    // dots back, score 0
        lives = 3;
        level = 1;
        pickups.setLevel(1);
        enemies.reset();
        player.revive();
        spawnPlayer();
        enemies.markNewLevel();
        extraLifeGiven = false;
        paused = false;
        passVisible = false;
        nextLevelRequested = false;
        gameMap.stopFlash();
        firstStart = true;
        stateTimer = 0.f;
        gameState = GameState::Ready;
    };

    auto goToMenu = [&]() {
        hud.saveHighScore(highScorePath());
        audio.stopAll();
        gameMap.stopFlash();
        paused = false;
        passVisible = false;
        nextLevelRequested = false;
        levelFlashStarted = false;
        title.reset();
        stateTimer = 0.f;
        gameState = GameState::Title;
    };

    auto setPausedState = [&](bool value) {
        if (paused == value) return;
        paused = value;
        audio.setPaused(paused);
        if (paused) Menus::buildPause(pausePanel, audio.volume() / 100.f, audio.muted());
        refreshSettings();
    };

    auto changeVolume = [&](float delta) {
        audio.changeVolume(delta);
        volumeToast = 1.5f;
        if (paused) Menus::buildPause(pausePanel, audio.volume() / 100.f, audio.muted());
        refreshSettings();
    };

    auto onButton = [&](int id) {
        switch (id) {
            case Menus::BtnPlay:
                showModeSelect = true;                 // choose Classic or Word Finding first
                modeComingSoon = false;
                Menus::buildModeSelect(modePanel, false);
                startLock = 0.4f;                      // the same key press must not also pick a mode
                break;
            case Menus::BtnSkip:
                skipSpin = true;
                break;
            case Menus::BtnPlayWord:
                if (slot.finished()) {
                    roundWord = slot.word();
                    wordRoundNeeded = true;
                    wordModeRequested = true;
                    showWordSlot = false;
                    startLock = 0.4f;
                }
                break;
            case Menus::BtnSpin:
                slot.start(wordLength(1));
                Menus::buildWordSlot(slotPanel, false, 0);
                break;
            case Menus::BtnClassic:
                classicRequested = true;
                break;
            case Menus::BtnWordMode:
                modeComingSoon = true;                 // not made yet
                showModeSelect = false;
                showWordSlot = true;
                slot.start(wordLength(1));          // level 1: 5 letters
                Menus::buildWordSlot(slotPanel, false, 0);
                break;
            case Menus::BtnRetry:  wantSpin = wordModeActive; wordRoundNeeded = true; startNewGame(); break;
            case Menus::BtnNext:   nextLevelRequested = true; break;
            case Menus::BtnMenu:   showModeSelect = false; modeComingSoon = false; goToMenu(); break;
            case Menus::BtnSettings:
                showSettings = true;
                Menus::buildSettings(settingsPanel, audio.volume() / 100.f, audio.muted());
                break;
            case Menus::BtnQuit:
                showQuitConfirm = true;                // ask first
                Menus::buildQuitConfirm(quitPanel);
                break;
            case Menus::BtnQuitYes:    window.close(); break;
            case Menus::BtnQuitCancel: showQuitConfirm = false; break;
            case Menus::BtnInfo:   showInfo = true;  break;
            case Menus::BtnBack:   if (showWordSlot) {
                    showWordSlot = false;
                    showModeSelect = true;
                    modeComingSoon = false;
                    Menus::buildModeSelect(modePanel, false);
                } else {
                    showInfo = false; showModeSelect = false; modeComingSoon = false; showSettings = false;
                }
                break;
            case Menus::BtnPause:  if (gameState == GameState::Playing) setPausedState(true); break;
            case Menus::BtnResume: setPausedState(false); break;
            case Menus::BtnMute:
                audio.toggleMute();
                refreshSettings();
                Menus::buildPause(pausePanel, audio.volume() / 100.f, audio.muted());
                break;
            default: break;
        }
    };

    // The screen that receives mouse clicks right now
    auto activePanel = [&]() -> Panel* {
        if (showQuitConfirm) return &quitPanel;
        if (showSettings) return &settingsPanel;
        if (paused) return &pausePanel;
        switch (gameState) {
            case GameState::Title:       return showWordSlot ? &slotPanel : (showModeSelect ? &modePanel : (showInfo ? &infoPanel : &titlePanel));
            case GameState::WordSpin:    return &spinPanel;
            case GameState::Playing:     return &hudPanel;
            case GameState::DeathScreen: return &deathPanel;
            case GameState::LevelClear:  return passVisible ? &passPanel : nullptr;
            default:                     return nullptr;
        }
    };

    while (window.isOpen())
    {
        float dt = std::min(clock.restart().asSeconds(), 0.05f);
        float realDt = dt;

        // When the screen on display changes, start with no button highlighted
        {
            static Panel* lastPanel = nullptr;
            Panel* cur = activePanel();
            if (cur != lastPanel) {
                if (cur) cur->resetFocus();
                lastPanel = cur;
            }
        }                      // keeps running while the game is paused
        if (paused) dt = 0.f;
        if (volumeToast > 0.f) volumeToast -= realDt;
        if (showSettings) volumeToast = 0.f;      // the settings screen shows the volume itself
        if (startLock > 0.f) startLock -= dt;
        gameMap.update(dt);   // runs the maze flash

        while (const std::optional event = window.pollEvent())
        {
            if (gameState == GameState::Loading) {
                const auto* skipKey = event->getIf<sf::Event::KeyPressed>();
                if ((skipKey && (skipKey->code == sf::Keyboard::Key::Enter ||
                                 skipKey->code == sf::Keyboard::Key::Space ||
                                 skipKey->code == sf::Keyboard::Key::Escape)) ||
                    event->is<sf::Event::MouseButtonPressed>())
                    skipLoading = true;
            }

            if (gameState == GameState::Cutscene) {
                const auto* cutKey = event->getIf<sf::Event::KeyPressed>();
                if ((cutKey && (cutKey->code == sf::Keyboard::Key::Enter ||
                                cutKey->code == sf::Keyboard::Key::Space ||
                                cutKey->code == sf::Keyboard::Key::Escape)) ||
                    event->is<sf::Event::MouseButtonPressed>())
                    skipLoading = true;
            }

            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (gameState == GameState::WordSpin &&
                    (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space ||
                     key->code == sf::Keyboard::Key::Escape))
                    skipSpin = true;

                // Menu keys: arrows move the highlight, Enter uses it
                bool enterConsumed = false;
                {
                    Panel* nav = activePanel();
                    if (nav == &hudPanel) nav = nullptr;      // while playing the arrows steer Pac-Man
                    if (nav) {
                        using NavKey = sf::Keyboard::Key;
                        const bool left  = key->code == NavKey::Left;
                        const bool right = key->code == NavKey::Right;

                        if ((paused || showSettings) && (left || right))
                            changeVolume(right ? 5.f : -5.f);          // pause menu: left/right = volume
                        else if (key->code == NavKey::Up || left)
                            nav->keyNavigate(-1);
                        else if (key->code == NavKey::Down || right)
                            nav->keyNavigate(+1);
                        else if (key->code == NavKey::Enter && !key->alt && nav->focusedId() > 0) {
                            int id = nav->focusedId();
                            enterConsumed = true;
                            startLock = std::max(startLock, 0.4f);  // Enter must not also start the game
                            onButton(id);
                        }
                    }
                }
                if (key->code == sf::Keyboard::Key::F11 ||
                    (key->code == sf::Keyboard::Key::Enter && key->alt))
                    { setFullscreen(!fullscreen); autoFullscreen = fullscreen; }
                if (key->code == sf::Keyboard::Key::P && gameState == GameState::Playing) {
                    paused = !paused;
                    audio.setPaused(paused);
                    if (paused) Menus::buildPause(pausePanel, audio.volume() / 100.f, audio.muted());
                }
                if (key->code == sf::Keyboard::Key::M) audio.toggleMute();
                if (key->code == sf::Keyboard::Key::M) {
                    volumeToast = 1.5f;
                    if (paused) Menus::buildPause(pausePanel, audio.volume() / 100.f, audio.muted());
                }
                if (key->code == sf::Keyboard::Key::Hyphen || key->code == sf::Keyboard::Key::Subtract) changeVolume(-5.f);
                if (key->code == sf::Keyboard::Key::Equal  || key->code == sf::Keyboard::Key::Add)      changeVolume(5.f);
                if (gameState == GameState::Title && showModeSelect && startLock <= 0.f) {
                    if (key->code == sf::Keyboard::Key::Enter && !key->alt) classicRequested = true;
                    if (key->code == sf::Keyboard::Key::Escape) { showModeSelect = false; modeComingSoon = false; }
                }
                if (key->code == sf::Keyboard::Key::Enter && !key->alt && !enterConsumed &&
                    showWordSlot && slot.finished() && startLock <= 0.f)
                    onButton(Menus::BtnPlayWord);
                if (key->code == sf::Keyboard::Key::Escape && showWordSlot) {
                    showWordSlot = false;
                    showModeSelect = true;
                    Menus::buildModeSelect(modePanel, false);
                }
                if (key->code == sf::Keyboard::Key::Escape && showQuitConfirm) showQuitConfirm = false;   // Esc = cancel
                if (key->code == sf::Keyboard::Key::Escape && showSettings) showSettings = false;
                if (key->code == sf::Keyboard::Key::Escape && showInfo) showInfo = false;
                if (key->code == sf::Keyboard::Key::Escape) {
                    if (paused) setPausedState(false);
                    else if (gameState == GameState::Playing) setPausedState(true);
                }
                if (key->code == sf::Keyboard::Key::Enter && !key->alt && !enterConsumed && !showQuitConfirm) {     // Enter = the main button
                    if (paused) setPausedState(false);
                    else if (gameState == GameState::DeathScreen) startNewGame();
                    else if (gameState == GameState::LevelClear && passVisible) nextLevelRequested = true;
                }
            }

            if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>())
                changeVolume(wheel->delta > 0.f ? 5.f : -5.f);

            Panel* panel = activePanel();
            if (const auto* moved = event->getIf<sf::Event::MouseMoved>()) {
                if (panel) {
                    panel->mouseMoved(window.mapPixelToCoords(moved->position));
                    if ((paused || showSettings) && panel->sliderDragging())
                        audio.setVolume(panel->sliderValue() * 100.f, false);   // saved when the mouse is released
                }
            }
            if (const auto* press = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (panel && press->button == sf::Mouse::Button::Left) {
                    int id = panel->mousePressed(window.mapPixelToCoords(press->position));
                    if ((paused || showSettings) && panel->sliderDragging())
                        audio.setVolume(panel->sliderValue() * 100.f, false);   // saved when the mouse is released
                    if (id > 0) onButton(id);
                }
            }
            if (event->is<sf::Event::MouseButtonReleased>()) {
                if (settingsPanel.sliderDragging() || pausePanel.sliderDragging())
                    audio.saveSettings();                 // save once, when the slider is let go
                settingsPanel.mouseReleased();
                titlePanel.mouseReleased(); hudPanel.mouseReleased(); pausePanel.mouseReleased();
                deathPanel.mouseReleased(); passPanel.mouseReleased();
            }

            if (const auto* resized = event->getIf<sf::Event::Resized>())
                applyLetterbox(window, resized->size);
        }

        player.handleInput();

        // Level difficulty (speeds, scared time, Blinky's speed-up)
        const LevelConfig cfg = levelConfig(level);
        const float topSpeed = 75.f;                          // map pixels per second at 100%
        enemies.setDifficulty(level, cfg, topSpeed, eatSystem.foodLeft() + eatSystem.powerLeft());
        player.setSpeed((enemies.anyFrightened() ? cfg.pacFrightSpeed : cfg.pacSpeed)
                        * topSpeed * gameMap.getScale());

        // Ghost-eaten freeze and the tiny pause after every dot
        if (ghostFreeze > 0.f) {
            ghostFreeze -= dt;
            if (ghostFreeze <= 0.f) enemies.hideEaten(false);
        }
        const float gdt = ghostFreeze > 0.f ? 0.f : dt;       // game time (0 while frozen)
        const float pdt = eatPause > 0.f ? 0.f : gdt;         // Pac-Man's own time
        if (eatPause > 0.f) eatPause -= gdt;
        if (gameState == GameState::Playing) player.update(pdt);
        else if (gameState == GameState::Dying) player.update(dt);
        else if (gameState == GameState::Ready) player.update(0.f);   // stands still, but is drawn
        EatResult eaten;
        if (gameState == GameState::Playing)
            eaten = eatSystem.update(gdt, player.getPosition());
        if (eaten.power > 0) enemies.frighten(cfg.frightTime, cfg.flashes);
        eatPause += static_cast<float>(eaten.food + 3 * eaten.power) / 60.f;   // arcade: 1 frame per dot, 3 per pellet
        if (eaten.food > 0)  audio.play(Sfx::Dot);            // power pellet: ghosts turn blue

        // Word Finding: collect the letters
        if (wordModeActive) {
            WordPickup got = wordMode.update(dt, player.getPosition(),
                                             gameState == GameState::Playing && ghostFreeze <= 0.f);
            if (got.letter) {
                if (got.decoy) {                                   // wrong letter: lose points
                    int penalty = std::min(50, eatSystem.getScore());
                    eatSystem.addScore(-penalty);
                    audio.play(Sfx::Dot);
                    hud.addPopup("-50", got.pos, 1.0f);
                } else {
                    eatSystem.addScore(100);
                    audio.play(wordMode.complete() ? Sfx::ExtraLife : Sfx::Fruit);
                    hud.addPopup("+100", got.pos, 1.0f);
                }
            }
        }

        // the dots that became letters must not count as eaten (ghost release, fruit)
        int dotsEaten = totalDots - (eatSystem.foodLeft() + eatSystem.powerLeft()) - eatSystem.reservedCount();

        if (gameState == GameState::Playing) {
            PickupResult got = pickups.update(gdt, player.getPosition(), dotsEaten);
            if (got.points > 0) {
                eatSystem.addScore(got.points);
                audio.play(Sfx::Fruit);
                hud.addPopup(std::to_string(got.points), got.where, 2.0f);
            }
        }
        EnemyResult hit;
        if (gameState == GameState::Playing) hit = enemies.update(gdt, player.getPosition(),
                                         static_cast<int>(player.getDirection()), dotsEaten);
        eatSystem.addScore(hit.score);
        if (hit.ghostsEaten > 0) {
            audio.play(Sfx::EatGhost);
            ghostFreeze = 1.0f;                      // freeze for a second, score shows
            enemies.hideEaten(true);
        }
        if (hit.ghostsEaten > 0) hud.addPopup(std::to_string(hit.score), hit.eatenAt, 1.0f);

                if (hit.playerHit && gameState == GameState::Playing) {
            gameState = GameState::Caught;
            audio.stopAll();            // freeze everything for a moment
            stateTimer = 0.f;
        }

        // All dots and power-ups eaten: freeze, flash the maze, then start over
        if (gameState == GameState::Playing && (wordModeActive ? wordMode.complete() : eatSystem.allEaten())) {
            gameState = GameState::LevelClear;
            if (wordModeActive) {                              // faster word = bigger bonus
                wordBonus = wordMode.timeBonus();
                eatSystem.addScore(wordBonus);
            }
            stateTimer = 0.f;
            levelFlashStarted = false;
        }

        // Intermission cutscene after clearing level 2
        if (gameState == GameState::Ready && cutscenePending) {
            cutscenePending = false;
            audio.stopAll();
            loading.start();
            loading.setShowLoadingUi(false);
            gameState = GameState::Cutscene;
        }

        // Word Finding: show the spin animation before every new word (next level or retry)
        if (wordModeActive && wantSpin && gameState == GameState::Ready) {
            wantSpin = false;
            skipSpin = false;
            spinDoneTimer = 0.f;
            audio.stopAll();
            slot.start(wordLength(level));            // level 1 = 5 letters, +1 per level, max 10
            roundWord = slot.word();
            wordRoundNeeded = true;                   // the letters are scattered when READY! begins
            Menus::buildWordSpin(spinPanel, false, 0, level);
            gameState = GameState::WordSpin;
            stateTimer = 0.f;
        }

        // Word Finding: a fresh round has just been set up, so scatter the letters
        if (wordModeActive && wordRoundNeeded && gameState == GameState::Ready) {
            wordRoundNeeded = false;
            wordModeRequested = false;
            std::string w = roundWord.empty() ? slot.randomWord() : roundWord;
            roundWord.clear();
            wordMode.start(w, eatSystem, {112.f, 188.f}, decoyCount(level));
        }

        // Entering a new state
        if (gameState != lastState) {
            lastState = gameState;
            if (gameState == GameState::Ready) stateTimer = 0.f;
        }

        stateTimer += dt;

        // READY! (plus PLAYER ONE on a new game), then play starts
        if (gameState == GameState::Ready && stateTimer >= (firstStart ? std::max(3.3f, audio.length(Sfx::GameStart)) : 1.5f)) {
            gameState = GameState::Playing;
            firstStart = false;
        }

        // After "GAME OVER" has shown on the maze for 3 seconds: the death screen
        if (gameState == GameState::GameOver && stateTimer >= 3.0f) {
            hud.setScore(eatSystem.getScore());          // makes sure a new high score counts
            hud.saveHighScore(highScorePath());
            Menus::buildDeath(deathPanel, eatSystem.getScore(), hud.highScore(), level);
            gameState = GameState::DeathScreen;
            stateTimer = 0.f;
        }

        // After GAME OVER the game returns to the home page
        if (gameState == GameState::GameOver && stateTimer >= 4.0f) {
            hud.saveHighScore(highScorePath());
            title.reset();
            gameState = GameState::Title;
            stateTimer = 0.f;
        }

        // GAME OVER for 4 seconds, then a new game
        if (gameState == GameState::GameOver && stateTimer >= 4.0f) {
            hud.saveHighScore(highScorePath());
            eatSystem.reset();                     // dots back, score 0
            lives = 3;
            enemies.reset();
            player.revive();
            player.setPosition(gameMap.getPosition() + sf::Vector2f(112.f, 188.f) * gameMap.getScale());
            player.setDirection(Direction::Left);
            firstStart = true;
            stateTimer = 0.f;
            gameState = GameState::Ready;
        }

        if (gameState == GameState::LevelClear) {
            if (!levelFlashStarted && stateTimer >= 1.0f) {          // short freeze first
                gameMap.startFlash(2.0f, 0.25f);                      // 4 white flashes
                levelFlashStarted = true;
            }
            if (stateTimer >= 3.5f && !passVisible && !nextLevelRequested) {   // flash done: show the pass screen
                gameMap.stopFlash();
                passVisible = true;
                if (wordModeActive) Menus::buildWordWin(passPanel, wordMode.word(), level, eatSystem.getScore(),
                                    wordMode.playTime(), wordBonus);
                else                Menus::buildPass(passPanel, level, eatSystem.getScore());
            }
            if (nextLevelRequested) {                                 // flash finished: restart
                gameMap.stopFlash();

                passVisible = false;
                nextLevelRequested = false;
                ++level;
                enemies.markNewLevel();
                wordRoundNeeded = true;               // Word Finding: a new word for the next level
                wantSpin = wordModeActive;
                cutscenePending = (level == 3);       // intermission cutscene after clearing level 2                                                      // next level, next fruit
                pickups.setLevel(level);

                int keepScore = eatSystem.getScore();
                eatSystem.reset();                                    // every dot comes back
                eatSystem.addScore(keepScore);                        // reset() would zero the score

                enemies.reset();
                player.setPosition(gameMap.getPosition() + sf::Vector2f(112.f, 188.f) * gameMap.getScale());
                player.setDirection(Direction::Left);

                gameState = GameState::Ready;
                stateTimer = 0.f;
            }
        }
        if (gameState == GameState::Caught && stateTimer >= 1.0f) {
            gameState = GameState::Dying;             // ghosts vanish, Pac-Man dies
            stateTimer = 0.f;
            player.startDeath();
            audio.play(Sfx::Death);
            pickups.clearFruit();
            // (no maze flash on death)   // flashes until the animation ends (stopped below)
        } else if (gameState == GameState::Dying && player.deathFinished()) {
            if (--lives <= 0) gameState = GameState::GameOver;     // out of lives
            else              { gameState = GameState::Respawn; enemies.markDeath(dotsEaten); }           // short pause on the empty maze
            stateTimer = 0.f;
        } else if (gameState == GameState::Respawn && stateTimer >= 1.0f) {
            gameState = GameState::Ready;
            player.revive();
            enemies.reset();
            player.setPosition(gameMap.getPosition() + sf::Vector2f(112.f, 188.f) * gameMap.getScale());
            player.setDirection(Direction::Left);
        }

        // Extra life at 10,000 points
        int scoreNow = eatSystem.getScore();
        if (scoreNow < 10000)        extraLifeGiven = false;       // resets on a new game
        else if (!extraLifeGiven) {
            extraLifeGiven = true;
            ++lives;
            audio.play(Sfx::ExtraLife);
        }

        // Siren / power-up / eyes loops (only while the game is running)
        audio.update();
        if (!paused)
            audio.updateBackground(gameState == GameState::Playing,
                                   enemies.anyEaten(), enemies.anyFrightened(),
                                   eatSystem.foodLeft() + eatSystem.powerLeft(), totalDots);

        hud.setScore(eatSystem.getScore());
        hud.setLives(lives > 0 ? lives - 1 : 0);   // icons show the extra lives
        hud.showPlayerOne(gameState == GameState::Ready && firstStart && stateTimer < 1.8f);
        hud.showReady(gameState == GameState::Ready);
        hud.showGameOver(gameState == GameState::GameOver);
        hud.update(dt);

        window.clear(sf::Color::Black);

        if (gameState == GameState::WordSpin) {
            int stoppedBefore = slot.stoppedReels();
            bool wasFinished = slot.finished();
            slot.update(dt);
            if (slot.stoppedReels() > stoppedBefore) audio.play(Sfx::Dot);     // tick when a reel stops
            if (!wasFinished && slot.finished()) {
                audio.play(Sfx::Fruit);                                         // word found
                Menus::buildWordSpin(spinPanel, true, static_cast<int>(slot.word().size()), level);
            }
            if (slot.finished()) spinDoneTimer += dt;

            if (skipSpin || spinDoneTimer >= 1.6f) {              // finished or skipped: start the round
                skipSpin = false;
                stateTimer = 0.f;
                gameState = GameState::Ready;                     // continues below: READY!, then play
                if (firstStart) audio.play(Sfx::GameStart);       // after a Retry the intro jingle plays now
            } else {
                slot.draw(window);
                spinPanel.draw(window);
                hud.draw(window);
                window.display();
                continue;
            }
        }

        if (gameState == GameState::Cutscene) {
            loading.update(dt);
            if (loading.finished() || skipLoading) {
                skipLoading = false;
                loading.setShowLoadingUi(true);
                stateTimer = 0.f;
                gameState = GameState::Ready;         // the new level starts with READY!
            } else {
                loading.draw(window);
                window.display();
                continue;
            }
        }

        if (gameState == GameState::Loading) {
            loading.update(dt);
            if (loading.finished() || skipLoading) {
                skipLoading = false;
                startLock = 0.5f;
                if (!fullscreen && autoFullscreen) setFullscreen(true);   // go fullscreen when the intro ends                 // Enter must not also start the game
                title.reset();
                stateTimer = 0.f;
                gameState = GameState::Title;     // continues to the home page below
            } else {
                loading.draw(window);
                window.display();
                continue;
            }
        }
        if (gameState == GameState::Title) {
            title.update(dt);
            if (showWordSlot) {
                int stoppedBefore = slot.stoppedReels();
                bool wasFinished = slot.finished();
                slot.update(dt);
                if (slot.stoppedReels() > stoppedBefore) audio.play(Sfx::Dot);   // tick when a reel stops
                if (!wasFinished && slot.finished()) {
                    audio.play(Sfx::Fruit);                                       // word found
                    Menus::buildWordSlot(slotPanel, true, static_cast<int>(slot.word().size()));
                }
            }
            if (title.startRequested() && startLock <= 0.f && !showModeSelect && !showInfo && !showWordSlot && !showQuitConfirm && !showSettings &&
                !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) &&
                !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt)) {
                showModeSelect = true;                         // choose a mode first
                modeComingSoon = false;
                Menus::buildModeSelect(modePanel, false);
                startLock = 0.4f;
            }
            if (classicRequested || wordModeRequested) { wordModeActive = wordModeRequested; hud.setHighScore(0); hud.loadHighScore(highScorePath()); }   // pick the mode
            if ((classicRequested || wordModeRequested) &&
                !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) &&
                !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt)) {
                // Fresh game
                classicRequested = false;
                showModeSelect = false;
                audio.stopAll();
                audio.play(Sfx::GameStart);
                eatSystem.reset();                    // dots back, score 0
                lives = 3;
                level = 1;
                pickups.setLevel(1);
                enemies.markNewLevel();
                enemies.reset();
                player.revive();
                player.setPosition(gameMap.getPosition() + sf::Vector2f(112.f, 188.f) * gameMap.getScale());
                player.setDirection(Direction::Left);
                firstStart = true;
                stateTimer = 0.f;
                gameState = GameState::Ready;
            } else {
                if (!showModeSelect && !showWordSlot) title.draw(window);
                if (showWordSlot) { slot.draw(window); slotPanel.draw(window); }
                else if (showModeSelect) modePanel.draw(window);
                else if (showSettings) settingsPanel.draw(window);
                else if (showInfo) infoPanel.draw(window);
                else          titlePanel.draw(window);
                if (volumeToast > 0.f)
                    hudPanel.drawVolumeToast(window, audio.volume() / 100.f, audio.muted());
                hud.setLives(0);                      // no life icons on the home page
                if (showQuitConfirm) quitPanel.draw(window);
                hud.draw(window);
                window.display();
                continue;
            }
        }

        gameMap.draw(window);
        eatSystem.draw(window);
        pickups.draw(window);
        if (wordModeActive) wordMode.drawLetters(window);
        bool showEnemies = gameState == GameState::Playing || gameState == GameState::Caught ||
                           (gameState == GameState::Ready && !(firstStart && stateTimer < 1.8f));
        if (showEnemies)
            enemies.draw(window);
        if (ghostFreeze <= 0.f && !(gameState == GameState::Ready && firstStart && stateTimer < 1.8f))
            player.draw(window);                   // hidden during the first part of the intro

        if (paused)                                                pausePanel.draw(window);
        else if (gameState == GameState::Playing)                  hudPanel.draw(window);
        else if (gameState == GameState::DeathScreen)              deathPanel.draw(window);
        else if (gameState == GameState::LevelClear && passVisible) passPanel.draw(window);
        if (showQuitConfirm) quitPanel.draw(window);
        if (wordModeActive) wordMode.drawWordBar(window);
        if (volumeToast > 0.f && !paused)
            hudPanel.drawVolumeToast(window, audio.volume() / 100.f, audio.muted());
        hud.draw(window);
        window.display();
    }
    hud.saveHighScore(highScorePath());
    {
        std::ofstream s("main/Data/User_data/settings.txt");
        if (s) s << (autoFullscreen ? 1 : 0) << "\n";
    }
    return 0;
}
