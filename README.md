# Pac-Man-Game (`v-0.2.0`)
The game with mixer of word finding genre!

<div align="center">
  <img src="https://github.com/sinhamajoni06-cell/Pac-Man-Game/blob/main/main/assets/graphic/game/name.png?raw=true" alt="WORD-MAN" width="420">
  <br>
  <a href="https://github.com/sinhamajoni06-cell/Pac-Man-Game/releases/tag/0.1.2">
    <img src="https://github.com/sinhamajoni06-cell/Pac-Man-Game/blob/main/main/assets/graphic/game/logo.png?raw=true" alt="Word-Man logo" width="140">
  </a>
  <br>
  <a href="https://github.com/sinhamajoni06-cell/Pac-Man-Game/releases/download/0.1.2/Wordman_0.1.2.zip">
    <img src="https://img.shields.io/badge/%E2%96%B6%20%20DOWNLOAD%20FOR%20WINDOWS-FFCC00?style=for-the-badge" alt="Download Word-Man for Windows" width="300">
  </a>
  <br><br>
  <b>Eat the dots. Dodge the ghosts. Beat the high score.</b>
  <br><br>
  <a href="https://github.com/sinhamajoni06-cell/Pac-Man-Game/releases/tag/0.1.2">
    <img src="https://img.shields.io/badge/version-0.1.2-E63946?style=flat-square" alt="Version 0.1.2">
  </a>
  <img src="https://img.shields.io/badge/platform-Windows-2979FF?style=flat-square" alt="Windows">
  <img src="https://img.shields.io/badge/made%20with-C%2B%2B%20%2B%20SFML-FFCC00?style=flat-square" alt="C++ and SFML">
  <a href="https://github.com/L-Malakar">
    <img src="https://img.shields.io/badge/Dev-LMalakar-00aa00?style=flat-square" alt="Dev L-Malakar">
  </a>
  <br><br>
  <img src="https://yt3.ggpht.com/BLmMBYURFwjGhRRAIw5KT_aVGrt1YZEwQI-TMpdGmMbiuOucv5SCumKGoY1mko14K_aAV9LzdyE9GA=s240-c-fcrop64=1,20000000dfffffff-nd-v1-rwa" alt="Word-Man logo" width="255">
</div>
  
---
<table>
  <tr>
    <td><img src="https://yt3.ggpht.com/elyjG7UpLsRxA5GgqiF8P4ytdXqlqWW1G2a6p-oq9edKyjKZoZkMRD0X-2tPxPafGPLlJWigpY3PfYc=s640-c-fcrop64=1,38130000c7ecffff-rw-nd-v1" alt="Word-Man logo 2" width="100%"></td>
    <td><img src="https://yt3.ggpht.com/ax9sEo_AWQJbs5oThPINdXzaEwX5y5hzCxGS_8TEh_ixWsa77pCGm6b8YnrI_N0TPcZFrmGUCn-k=s640-c-fcrop64=1,38130000c7ecffff-rw-nd-v1" alt="Word-Man logo 1" width="100%"></td>
  </tr>
</table>

# Download (`Windows`)

<table>
  <tr>
    <td align="left"><b>Source Code</b>: (<a href="https://github.com/sinhamajoni06-cell/Pac-Man-Game/tree/Backup--v-0.2.0"><code>Pac-Man-Game.zip</code></a>)</td>
    <td align="right"><a href="https://github.com/sinhamajoni06-cell/Pac-Man-Game/archive/refs/heads/Backup--v-0.2.0.zip">Download</a></td>
  </tr>
  </tr>
    <td align="left"><b>Word-Man(v0.1.2)</b>: (<a href="https://github.com/sinhamajoni06-cell/Pac-Man-Game/releases/tag/0.1.2"><code>Word-Man.zip</code></a>)</td>
    <td align="right"><a href="https://github.com/sinhamajoni06-cell/Pac-Man-Game/releases/download/0.1.2/Wordman_0.1.2.zip">Download</a></td>
  </tr>
  </tr>
    <td align="left"><b>G++ Installation and Setup</b></td>
    <td align="right"><a href="https://github.com/sinhamajoni06-cell/Zero-Theory#g-installation-and-setup">Open link </a></td>
  </tr>
  </tr>
    <td align="left"><b>Development Worning</b></td>
    <td align="right"><a href="https://github.com/sinhamajoni06-cell/Zero-Theory#development-worning-">Open link </a></td>
  </tr>
<table>

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
├── build.bat                      # build and launch script (builds Wordman.exe)
├── Pacman/                        # build output (Wordman.exe, DLLs, copied assets, error.log)
├── main/
│   └── assets/
│       ├── audio/
│       │   └── SFX/
│       │       ├── game_start.mp3     # intro jingle
│       │       ├── dot_1.mp3          # waka-waka (alternates with dot_2)
│       │       ├── dot_2.mp3
│       │       ├── siren_1.mp3        # background siren (faster as dots run out)
│       │       ├── siren_2.mp3
│       │       ├── siren_3.mp3
│       │       ├── power_up.mp3       # loop while ghosts are blue
│       │       ├── eyes.mp3           # loop while eaten ghosts run home
│       │       ├── eat_ghost.mp3
│       │       ├── fruit.mp3
│       │       ├── death.mp3
│       │       ├── extra_life.mp3
│       │       ├── pause.mp3
│       │       └── pause_beat.mp3
│       ├── TxT/
│       │   ├── PressStart2P-Regular.ttf   # arcade font (HUD, menus, loading text)
│       │   ├── highscore.txt              # created by the game (best score)
│       │   ├── volume.txt                 # created by the game (saved volume)
│       │   ├── settings.txt               # created by the game (fullscreen choice)
│       │   └── name/
│       │       ├── crackman.ttf           # logo fonts for the "WORD-MAN" title
│       │       ├── crackman front.ttf
│       │       └── crackman back.ttf
│       └── graphic/
│           └── game/
│               ├── game.png
│               ├── logo.png               # window icon
│               ├── app.ico                # exe icon (embedded by build.bat)
│               ├── map.png                # walls only (not used by the code)
│               ├── Pac-man/
│               │   ├── Pac-Man (Up).gif
│               │   ├── Pac-Man (Down).gif
│               │   ├── Pac-Man (Left).gif
│               │   ├── Pac-Man (Right).gif
│               │   ├── Pac-Man (Death Animation).gif   # plays once when caught
│               │   ├── Pac-Man (Intermission #1).gif   # big Pac-Man in the intro and cutscene
│               │   └── Pac Man.png                     # life icon
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
│               ├── pickups/               # bonus fruit (SVG, drawn by nanosvg)
│               │   ├── cherry.svg
│               │   ├── strawberry.svg
│               │   ├── orange.svg
│               │   ├── apple.svg
│               │   ├── melon.svg
│               │   ├── galaxian.svg
│               │   └── key.svg            # (bell.svg is picked up automatically if you add it)
│               └── map/
│                   ├── map_full_.png      # blue maze walls, no dots (drawing, hitbox and the white flash)
│                   ├── food.png           # small dots
│                   └── power_up.png       # 4 big power pellets
└── core/
    ├── lib/
    │   ├── header/
    │   │   ├── player.h             # Pac-Man movement, input, GIF animation, death animation
    │   │   ├── map.h                # map drawing, level-clear flash
    │   │   ├── map_box.h            # wall hitbox, lane movement, tunnel teleport
    │   │   ├── eat_system.h         # eating dots, power-up blinking, score
    │   │   ├── pickup_system.h      # bonus fruit and the level indicator
    │   │   ├── hud.h                # 1UP, score, HIGH SCORE, READY!, GAME OVER, life icons
    │   │   ├── title_screen.h       # home page: WORD-MAN logo, character list
    │   │   ├── loading_animation.h  # start-up chase scene, loading text and bar, cutscene
    │   │   ├── ui_panel.h           # menu screens: buttons, slider, volume popup
    │   │   ├── menus.h              # title, pause, death, pass and info screens
    │   │   ├── audio_system.h       # global sound system (Audio::get())
    │   │   ├── level_config.h       # difficulty per level (speeds, scared time, timing)
    │   │   ├── version.h            # version number for the INFO screen
    │   │   ├── stb_image.h          # GIF decoder (third-party, single header)
    │   │   ├── nanosvg.h            # SVG reader (third-party, single header)
    │   │   ├── nanosvgrast.h        # SVG rasterizer (third-party, single header)
    │   │   └── enemy/
    │   │       ├── gif_animation.h    # GIF loader (+ blue/white scared frames)
    │   │       ├── ghost.h            # ghost base class: tile movement, house, modes
    │   │       ├── blinky.h           # chases Pac-Man directly (speeds up near the end)
    │   │       ├── pinky.h            # aims 4 tiles ahead of Pac-Man
    │   │       ├── inky.h             # flanks using Blinky's position
    │   │       ├── clyde.h            # chases when far, retreats when close
    │   │       └── enemy_manager.h    # scatter/chase timing, scared mode, house release, collisions
    │   └── cpp/
    │       ├── game.cpp
    │       ├── player.cpp
    │       ├── map.cpp
    │       ├── map_box.cpp
    │       ├── eat_system.cpp
    │       ├── pickup_system.cpp
    │       ├── hud.cpp
    │       ├── title_screen.cpp
    │       ├── loading_animation.cpp
    │       ├── ui_panel.cpp
    │       ├── menus.cpp
    │       ├── audio_system.cpp
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
            └── PacmanMain.cpp     # main(): game loop, states, input, error log and fullscreen
```
