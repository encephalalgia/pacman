#include <iostream>
#include <cmath>
#include "SDL.h"
#include "z80.h"
#include "Components/Memory.h"

using Z80 = z80<Memory>;

int main(int argc, char** argv)
{
    // initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init() failed. SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    static constexpr int clockSpeed {static_cast<int>(2.5e6)}; // 2.5 MHz
    static constexpr int cycles {clockSpeed / 60};
    const int frameTime {static_cast<int>(std::ceil(1.0 / 60.0 * 1e3))};

    Z80 z {};

    bool quit {false};
    int exceeded {0};
    while(!quit) {
        // process SDL events
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        unsigned long long begin {SDL_GetTicks64()};
        exceeded = z.run(cycles + exceeded);

        // draw a frame

        // sleep until the next frame accounting for spent time
        SDL_Delay(std::max(0LL, frameTime - static_cast<long long>(SDL_GetTicks64() - begin)));
    }

    std::cout << "Hello, World!" << std::endl;
    return 0;
}
