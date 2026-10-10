# Pac-Man-Game
The game with mixer of word finding genre!


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
│               ├── Pac-man/
│               │   ├── Pac-Man (Up).gif
│               │   ├── Pac-Man (Down).gif
│               │   ├── Pac-Man (Left).gif
│               │   └── Pac-Man (Right).gif
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
    │   │   └── stb_image.h        # GIF decoder (third-party, single header)
    │   └── cpp/
    │       ├── game.cpp
    │       ├── player.cpp
    │       ├── map.cpp
    │       ├── map_box.cpp
    │       └── eat_system.cpp
    └── src/
        └── engine/
            └── PacmanMain.cpp     # main() and the game loop
```
