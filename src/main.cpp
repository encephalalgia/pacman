#include "SDL.h"
#include "z80.h"
#include "Pacman.h"

using Z80 = z80<Pacman>;

int main(int argc, char** argv)
{
    // initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init() failed. SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    static constexpr int clockSpeed {static_cast<int>(3.072e6)}; // 3.072 MHz
    static constexpr int cyclesPerFrame {clockSpeed / 60};
    const int frameTime {static_cast<int>(1.0 / 60.0 * 1e3)};

    // dipswitch command line parsing
    std::uint8_t dipswitch {0b11001001};
    using namespace std::string_view_literals;
    for (int i {1}; i < argc; ++i) {
        bool hasNext {i + 1 != argc};
        if (!hasNext) break;

        std::string setting {argv[i + 1]};
        if (argv[i] == "-coins_per_game"sv) {
            try {
                dipswitch &= 0xFC;
                switch (std::stoi(setting)) {
                    case 0: dipswitch |= 0b00; break; // free play
                    case 1: dipswitch |= 0b01; break; // 1 coin per game
                    case 2: dipswitch |= 0b10; break; // 1 coin per 2 games
                    case 3: dipswitch |= 0b11; break; // 2 coins per game
                }
            } catch (std::exception& e) {
                SDL_Log("error: failed to read integer for '-coins_per_game' parameter, using default=1 coin per game.\n");
                dipswitch |= 0b01;
            }
        } else if (argv[i] == "-lives_per_game"sv) {
            try {
                dipswitch &= 0xF3;
                switch (std::stoi(setting)) {
                    case 1: dipswitch |= 0b0000; break; // 1 life
                    case 2: dipswitch |= 0b0100; break; // 2 lives
                    case 3: dipswitch |= 0b1000; break; // 3 lives
                    case 5: dipswitch |= 0b1100; break; // 5 lives
                }
            } catch (std::exception& e) {
                SDL_Log("error: failed to read integer for '-lives_per_game' parameter, using default=3 lives per game.\n");
                dipswitch |= 0b1000;
            }
        } else if (argv[i] == "-extra_life_score"sv) {
            try {
                dipswitch &= 0xCF;
                switch (std::stoi(setting)) {
                    case 10000: dipswitch |= 0b000000; break; // 10000 points
                    case 15000: dipswitch |= 0b010000; break; // 15000 points
                    case 20000: dipswitch |= 0b100000; break; // 20000 points
                    case 0: dipswitch |= 0b110000; break; // none
                }
            } catch (std::exception& e) {
                SDL_Log("error: failed to read integer for '-extra_life_score' parameter, using default=10000 for extra life.\n");
                dipswitch |= 0b000000;
            }
        } else if (argv[i] == "-difficulty"sv) {
            dipswitch &= 0xBF;
            if (setting == "HARD") {
                dipswitch |= 0b0000000;
            } else {
                dipswitch |= 0b1000000;
                if (setting != "NORMAL")
                    SDL_Log("error: failed to read integer for '-difficulty' parameter, using default=normal.\n");
            }
        } else if (argv[i] == "-ghost_names"sv) {
            dipswitch &= 0x7F;
            if (setting == "ALT") {
                dipswitch |= 0b00000000;
            } else {
                dipswitch |= 0b10000000;
                if (setting != "NORMAL")
                    SDL_Log("error: failed to read integer for '-ghost_names' parameter, using default=normal.\n");
            }
        } else {
            SDL_Log("Unrecognized command line argument '%s'.\nAvailable parameters are:\n\t-coins_per_game <0,1,2,3>\n\t-lives_per_game <1,2,3,5>\n\t-extra_life_score <10000,15000,20000,0>\n\t-difficulty <NORMAL,HARD>\n\t-ghost_names <NORMAL,ALT>\n\n", argv[i]);
        }
        ++i;
    }

    Pacman pacman {dipswitch};
    Z80 z {pacman};

    bool quit {!(pacman.active)};
    int cycles {cyclesPerFrame};

    while(!quit) {
        // process SDL events
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                pacman.onKeyDown(e.key.keysym.scancode);
            } else if (e.type == SDL_KEYUP) {
                pacman.onKeyUp(e.key.keysym.scancode);
            }
        }

        unsigned long long begin {SDL_GetTicks64()};
        cycles = cyclesPerFrame + z.run(cycles); // z.run -> a negative value representing the number of exceeded cycles

        // draw a frame
        pacman.draw();

        // generate interrupt if enabled
        if (pacman.interruptEnabled)
            z.reqInt(pacman.interruptVector);

        // sleep until the next frame accounting for spent time
        SDL_Delay(std::max(0LL, frameTime - static_cast<long long>(SDL_GetTicks64() - begin)));
    }

    pacman.off();
    SDL_Quit();
    return 0;
}
