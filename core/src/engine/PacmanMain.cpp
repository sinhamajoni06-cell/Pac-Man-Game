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
#include "loading_animation.h"
#include <algorithm>
#include <fstream>
#include <string>
#include "ui_panel.h"
#include "menus.h"

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

int main()
{
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Word-Man");
    window.setFramerateLimit(60);

    sf::Image icon;
    if (icon.loadFromFile("main/assets/graphic/game/logo.png"))
        window.setIcon(icon.getSize(), icon.getPixelsPtr());

    bool fullscreen = false;
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
    hud.loadHighScore("main/assets/TxT/highscore.txt");

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
    float volumeToast = 0.f;              // time left for the little volume box

    // Menu screens (buttons are clickable with the mouse)
    if (!Panel::loadFont("main/assets/TxT/PressStart2P-Regular.ttf"))
        return 1;
    Panel titlePanel, hudPanel, pausePanel, deathPanel, passPanel;
    for (Panel* p : {&titlePanel, &hudPanel, &pausePanel, &deathPanel, &passPanel})
        p->setTransform(gameMap.getPosition(), gameMap.getScale());
    Menus::buildTitle(titlePanel);
    Menus::buildHudButtons(hudPanel);

    {   // volume from the last run
        std::ifstream f("main/assets/TxT/volume.txt");
        float v = 0.f;
        if (f >> v) audio.setVolume(v);
    }

    sf::Clock clock;

    // Playing -> Caught (freeze) -> Dying (animation) -> Respawn (pause) -> Playing
    enum class GameState { Loading, Playing, Caught, Dying, Respawn, LevelClear, Ready, GameOver, Title, DeathScreen };
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
        hud.saveHighScore("main/assets/TxT/highscore.txt");
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
    };

    auto changeVolume = [&](float delta) {
        audio.changeVolume(delta);
        volumeToast = 1.5f;
        if (paused) Menus::buildPause(pausePanel, audio.volume() / 100.f, audio.muted());
    };

    auto onButton = [&](int id) {
        switch (id) {
            case Menus::BtnPlay:
            case Menus::BtnRetry:  startNewGame(); break;
            case Menus::BtnNext:   nextLevelRequested = true; break;
            case Menus::BtnMenu:   goToMenu(); break;
            case Menus::BtnQuit:   window.close(); break;
            case Menus::BtnPause:  if (gameState == GameState::Playing) setPausedState(true); break;
            case Menus::BtnResume: setPausedState(false); break;
            case Menus::BtnMute:
                audio.toggleMute();
                Menus::buildPause(pausePanel, audio.volume() / 100.f, audio.muted());
                break;
            default: break;
        }
    };

    // The screen that receives mouse clicks right now
    auto activePanel = [&]() -> Panel* {
        if (paused) return &pausePanel;
        switch (gameState) {
            case GameState::Title:       return &titlePanel;
            case GameState::Playing:     return &hudPanel;
            case GameState::DeathScreen: return &deathPanel;
            case GameState::LevelClear:  return passVisible ? &passPanel : nullptr;
            default:                     return nullptr;
        }
    };

    while (window.isOpen())
    {
        float dt = clock.restart().asSeconds();
        float realDt = dt;                      // keeps running while the game is paused
        if (paused) dt = 0.f;
        if (volumeToast > 0.f) volumeToast -= realDt;
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

            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::F11 ||
                    (key->code == sf::Keyboard::Key::Enter && key->alt))
                    setFullscreen(!fullscreen);
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
                if (key->code == sf::Keyboard::Key::Escape) {
                    if (paused) setPausedState(false);
                    else if (gameState == GameState::Playing) setPausedState(true);
                }
                if (key->code == sf::Keyboard::Key::Enter && !key->alt) {     // Enter = the main button
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
                    if (paused && panel->sliderDragging())
                        audio.setVolume(panel->sliderValue() * 100.f);
                }
            }
            if (const auto* press = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (panel && press->button == sf::Mouse::Button::Left) {
                    int id = panel->mousePressed(window.mapPixelToCoords(press->position));
                    if (paused && panel->sliderDragging())
                        audio.setVolume(panel->sliderValue() * 100.f);
                    if (id > 0) onButton(id);
                }
            }
            if (event->is<sf::Event::MouseButtonReleased>()) {
                titlePanel.mouseReleased(); hudPanel.mouseReleased(); pausePanel.mouseReleased();
                deathPanel.mouseReleased(); passPanel.mouseReleased();
            }

            if (const auto* resized = event->getIf<sf::Event::Resized>())
                applyLetterbox(window, resized->size);
        }

        player.handleInput();
        if (gameState == GameState::Playing || gameState == GameState::Dying) player.update(dt);
        else if (gameState == GameState::Ready) player.update(0.f);   // stands still, but is drawn
        EatResult eaten;
        if (gameState == GameState::Playing)
            eaten = eatSystem.update(dt, player.getPosition());
        if (eaten.power > 0) enemies.frighten();
        if (eaten.food > 0)  audio.play(Sfx::Dot);            // power pellet: ghosts turn blue

        int dotsEaten = totalDots - (eatSystem.foodLeft() + eatSystem.powerLeft());

        if (gameState == GameState::Playing) {
            PickupResult got = pickups.update(dt, player.getPosition(), dotsEaten);
            if (got.points > 0) {
                eatSystem.addScore(got.points);
                audio.play(Sfx::Fruit);
                hud.addPopup(std::to_string(got.points), got.where, 2.0f);
            }
        }
        EnemyResult hit;
        if (gameState == GameState::Playing) hit = enemies.update(dt, player.getPosition(),
                                         static_cast<int>(player.getDirection()), dotsEaten);
        eatSystem.addScore(hit.score);
        if (hit.ghostsEaten > 0) audio.play(Sfx::EatGhost);
        if (hit.ghostsEaten > 0) hud.addPopup(std::to_string(hit.score), hit.eatenAt, 1.0f);

                if (hit.playerHit && gameState == GameState::Playing) {
            gameState = GameState::Caught;
            audio.stopAll();            // freeze everything for a moment
            stateTimer = 0.f;
        }

        // All dots and power-ups eaten: freeze, flash the maze, then start over
        if (gameState == GameState::Playing && eatSystem.allEaten()) {
            gameState = GameState::LevelClear;
            stateTimer = 0.f;
            levelFlashStarted = false;
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
            hud.saveHighScore("main/assets/TxT/highscore.txt");
            Menus::buildDeath(deathPanel, eatSystem.getScore(), hud.highScore(), level);
            gameState = GameState::DeathScreen;
            stateTimer = 0.f;
        }

        // After GAME OVER the game returns to the home page
        if (gameState == GameState::GameOver && stateTimer >= 4.0f) {
            hud.saveHighScore("main/assets/TxT/highscore.txt");
            title.reset();
            gameState = GameState::Title;
            stateTimer = 0.f;
        }

        // GAME OVER for 4 seconds, then a new game
        if (gameState == GameState::GameOver && stateTimer >= 4.0f) {
            hud.saveHighScore("main/assets/TxT/highscore.txt");
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
                Menus::buildPass(passPanel, level, eatSystem.getScore());
            }
            if (nextLevelRequested) {                                 // flash finished: restart
                gameMap.stopFlash();

                passVisible = false;
                nextLevelRequested = false;
                ++level;                                                      // next level, next fruit
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
            else              gameState = GameState::Respawn;           // short pause on the empty maze
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

        if (gameState == GameState::Loading) {
            loading.update(dt);
            if (loading.finished() || skipLoading) {
                skipLoading = false;
                startLock = 0.5f;
                if (!fullscreen) setFullscreen(true);   // go fullscreen when the intro ends                 // Enter must not also start the game
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
            if (title.startRequested() && startLock <= 0.f &&
                !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) &&
                !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt)) {
                // Fresh game
                audio.stopAll();
                audio.play(Sfx::GameStart);
                eatSystem.reset();                    // dots back, score 0
                lives = 3;
                level = 1;
                pickups.setLevel(1);
                enemies.reset();
                player.revive();
                player.setPosition(gameMap.getPosition() + sf::Vector2f(112.f, 188.f) * gameMap.getScale());
                player.setDirection(Direction::Left);
                firstStart = true;
                stateTimer = 0.f;
                gameState = GameState::Ready;
            } else {
                title.draw(window);
                titlePanel.draw(window);
                if (volumeToast > 0.f)
                    hudPanel.drawVolumeToast(window, audio.volume() / 100.f, audio.muted());
                hud.setLives(0);                      // no life icons on the home page
                hud.draw(window);
                window.display();
                continue;
            }
        }

        gameMap.draw(window);
        eatSystem.draw(window);
        pickups.draw(window);
        bool showEnemies = gameState == GameState::Playing || gameState == GameState::Caught ||
                           (gameState == GameState::Ready && !(firstStart && stateTimer < 1.8f));
        if (showEnemies)
            enemies.draw(window);
        if (!(gameState == GameState::Ready && firstStart && stateTimer < 1.8f))
            player.draw(window);                   // hidden during the first part of the intro

        if (paused)                                                pausePanel.draw(window);
        else if (gameState == GameState::Playing)                  hudPanel.draw(window);
        else if (gameState == GameState::DeathScreen)              deathPanel.draw(window);
        else if (gameState == GameState::LevelClear && passVisible) passPanel.draw(window);
        if (volumeToast > 0.f && !paused)
            hudPanel.drawVolumeToast(window, audio.volume() / 100.f, audio.muted());
        hud.draw(window);
        window.display();
    }
    hud.saveHighScore("main/assets/TxT/highscore.txt");
    {
        std::ofstream f("main/assets/TxT/volume.txt");
        if (f) f << audio.volume() << "\n";
    }
    return 0;
}
