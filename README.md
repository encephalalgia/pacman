# Pac-Man Emulator
This is an emulator of the 1980 Pac-Man arcade machine by Midway. Mainly created because I wanted a quick project that utilized my [Z80 emulator](https://github.com/harrow22/z80). Sound has not (yet?) been implemented.
<p align="center">
  <img alt="Pac-Man attract mode gif" src="https://raw.githubusercontent.com/harrow22/pacman/master/examples/demo.gif" height="400" />
</p>

# Controls
| Key         | Action                            |
|-------------|-----------------------------------|
| C           | deposit credits                   |
| enter       | start the game in one-player mode |
| P           | start the game in two-player mode |
| up arrow    | move up                           |
| left arrow  | move left                         |
| right arrow | move right                        | 
| down arrow  | move down                         |
| 1           | coin slot 1                       |
| 2           | coin slot 2                       |
| T           | switch board test on/off          |
| Space       | switch level skip on/off          |

# Usage
### Dependencies
* A c++ compiler
* CMake 3.26+
* A CMake build generator
* SDL2
* Pac-Man roms [(see instructions)](roms)

### Build & Run
```angular2html
git clone https://github.com/harrow22/pacman.git
cd pacman
cmake -S . -B build -G <generator> -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_PREFIX_PATH=<path/to/sdl>
cmake --build build --config Release
build/src/pacman.exe <PARAMETERS...>
```

### DIP Switch
Replace `<PARAMETERS...>` with zero or more of the following:

| Parameter               | Range                  | Default | Description                                                                  |
|-------------------------|------------------------|---------|------------------------------------------------------------------------------| 
| `-coins_per_game <n>`   | [0,3]                  | 1       | changes the number of coins required to start a game                         |
| `-lives_per_game <n>`   | 1,2,3 or 5             | 3       | changes the number of lives per game                                         |
| `-extra_life_score <n>` | 10000,15000,20000 or 0 | 10000   | changes the number of points needed to gain an extra life, none=0            |
| `-difficulty <str>`     | NORMAL or HARD         | NORMAL  | changes the algorithm the ghosts use, making less places for Pac-Man to hide |
| `-ghost_names <str>`    | NORMAL or ALT          | NORMAL  | changes the ghosts nicknames                                                 |

# Resources
* [Chris Lomont's Pac-Man Emulation Guide](https://www.lomont.org/software/games/pacman/PacmanEmulation.pdf)
* [superzazu's Pac-Man Emulator](https://github.com/superzazu/pac)