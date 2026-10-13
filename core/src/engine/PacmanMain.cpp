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
#include <algorithm>

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

    sf::Clock clock;

    // Playing -> Caught (freeze) -> Dying (animation) -> Respawn (pause) -> Playing
    enum class GameState { Playing, Caught, Dying, Respawn, LevelClear, Ready, GameOver, Title };
    GameState gameState = GameState::Title;
    float stateTimer = 0.f;
    int   lives = 3;
    bool  firstStart = true;                       // "PLAYER ONE" only on a new game
    GameState lastState = GameState::Playing;
    bool  levelFlashStarted = false;

    while (window.isOpen())
    {
        float dt = clock.restart().asSeconds();
        if (paused) dt = 0.f;
        gameMap.update(dt);   // runs the maze flash

        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::P && gameState == GameState::Playing) {
                    paused = !paused;
                    audio.setPaused(paused);
                }
                if (key->code == sf::Keyboard::Key::M) audio.toggleMute();
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
            if (stateTimer >= 3.5f) {                                 // flash finished: restart
                gameMap.stopFlash();

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
        if (gameState == GameState::Title) {
            title.update(dt);
            if (title.startRequested()) {
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
        hud.draw(window);
        window.display();
    }
    hud.saveHighScore("main/assets/TxT/highscore.txt");
    return 0;
}
