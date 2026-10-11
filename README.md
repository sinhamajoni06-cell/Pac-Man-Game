# Pac-Man-Game
The game with mixer of word finding genre!

## Ghost AI
| Ghost  | Chase behavior                                   | Scatter corner |
|--------|--------------------------------------------------|----------------|
| Blinky | Targets Pac-Man's tile                           | Top right      |
| Pinky  | Targets 4 tiles ahead of Pac-Man                 | Top left       |
| Inky   | Mirrors Blinky around a point 2 tiles ahead      | Bottom right   |
| Clyde  | Chases when 8+ tiles away, otherwise retreats    | Bottom left    |

# File structure-
```
Pacman-project/
├── build.bat                      # build and launch script
├── Pacman/                        # build output (Pacman.exe, DLLs, copied assets)
├── main/
│   └── assets/
│       └── graphic/
│           └── game/
│               ├── game.png
│               ├── map.png                # walls only
│               ├── Pac-man/
│               │   ├── Pac-Man (Up).gif
│               │   ├── Pac-Man (Down).gif
│               │   ├── Pac-Man (Left).gif
│               │   └── Pac-Man (Right).gif
│               ├── Blinky/                # red ghost
│               │   ├── Blinky (Up).gif
│               │   ├── Blinky (Down).gif
│               │   ├── Blinky (Left).gif
│               │   ├── Blinky (Right).gif
│               │   └── Blinky (Intermission 3).gif    # not used yet
│               ├── Pinky/                 # pink ghost
│               │   ├── Pinky (Up).gif
│               │   ├── Pinky (Down).gif
│               │   ├── Pinky (Left).gif
│               │   └── Pinky (Right).gif
│               ├── Inky/                  # cyan ghost
│               │   ├── Inky (Up).gif
│               │   ├── Inky (Down).gif
│               │   ├── Inky (Left).gif
│               │   └── Inky (Right).gif
│               ├── Clyde/                 # orange ghost
│               │   ├── Clyde (Up).gif
│               │   ├── Clyde (Down).gif
│               │   ├── Clyde (Left).gif
│               │   └── Clyde (Right).gif
│               ├── IZIKE/                 # not used yet
│               └── map/
│                   ├── map_full_.png      # maze walls, no dots (used for drawing and the hitbox)
│                   ├── food.png           # small dots
│                   └── power_up.png       # 4 big power pellets
└── core/
    ├── lib/
    │   ├── header/
    │   │   ├── player.h           # Pac-Man movement, input, GIF animation
    │   │   ├── map.h              # map drawing
    │   │   ├── map_box.h          # wall hitbox, lane movement, tunnel teleport
    │   │   ├── eat_system.h       # eating dots, power-up blinking, score
    │   │   ├── stb_image.h        # GIF decoder (third-party, single header)
    │   │   └── enemy/
    │   │       ├── gif_animation.h    # GIF loader (+ blue/white scared frames)
    │   │       ├── ghost.h            # ghost base class: tile movement, house, modes
    │   │       ├── blinky.h           # chases Pac-Man directly
    │   │       ├── pinky.h            # aims 4 tiles ahead of Pac-Man
    │   │       ├── inky.h             # flanks using Blinky's position
    │   │       ├── clyde.h            # chases when far, retreats when close
    │   │       └── enemy_manager.h    # scatter/chase timing, scared mode, collisions
    │   └── cpp/
    │       ├── game.cpp
    │       ├── player.cpp
    │       ├── map.cpp
    │       ├── map_box.cpp
    │       ├── eat_system.cpp
    │       └── enemy/
    │           ├── gif_animation.cpp
    │           ├── ghost.cpp
    │           ├── blinky.cpp
    │           ├── pinky.cpp
    │           ├── inky.cpp
    │           ├── clyde.cpp
    │           └── enemy_manager.cpp
    └── src/
        └── engine/
            └── PacmanMain.cpp     # main() and the game loop
```
