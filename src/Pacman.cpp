#include "Pacman.h"
#include <fstream>
#include <iostream>

Pacman::Pacman(const std::uint8_t ds) : dipswitch{ds}
{
    const std::string dir {"/../../assets/roms/"};
    active &= load(rom, dir + "pacman.6e", 0, 0x1000);
    active &= load(rom, dir + "pacman.6f", 0x1000, 0x1000);
    active &= load(rom, dir + "pacman.6h", 0x2000, 0x1000);
    active &= load(rom, dir + "pacman.6j", 0x3000, 0x1000);
    active &= load(colorRom, dir + "82s123.7f", 0, 0x20);
    active &= load(paletteRom, dir + "82s126.4a", 0, 0x100);
    active &= load(tileRom, dir + "pacman.5e", 0, 0x1000);
    active &= load(spriteRom, dir + "pacman.5f", 0, 0x1000);

    if (active) {
        active = false;

        // set up display contents
        window = SDL_CreateWindow(
                "Pac-Man",
                SDL_WINDOWPOS_CENTERED,
                SDL_WINDOWPOS_CENTERED,
                screenWidth * scaleFactor,
                screenHeight * scaleFactor,
                SDL_WINDOW_SHOWN);
        if (!window) {
            SDL_Log("SDL_CreateWindow() failed. SDL_Error: %s\n", SDL_GetError());
            return;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
        if (!renderer){
            SDL_Log("SDL_CreateRenderer() failed. SDL_Error: %s\n", SDL_GetError());
            return;
        }

        texture = SDL_CreateTexture(renderer,
                                    SDL_PIXELFORMAT_ABGR8888,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    screenWidth, screenHeight);
        if (!texture){
            SDL_Log("SDL_CreateTexture() failed. SDL_Error: %s\n", SDL_GetError());
            return;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderSetLogicalSize(renderer, screenWidth, screenHeight);

        // initialize screen
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        active = true;
    }
}

std::uint8_t Pacman::read8(const std::uint16_t addr) const
{
    if (addr < 0x4000) {
        return rom[addr];
    } else if (addr < 0x4800) {
        return vram[addr - 0x4000];
    } else if (addr < 0x4FF0) {
        return ram[addr - 0x4800];
    } else if (addr < 0x5100) { // IO
        if (addr < 0x5040) { // IN0 (joystick and coin slot)
            return input0;
        } else if (addr < 0x5080) { // IN1 (joystick and start buttons)
            return input1;
        } else if (addr < 0x50C0) { // Dip Switch Settings
            return dipswitch;
        }
    } else {
        std::cerr << std::format("error: attempt to read at {:>04X}\n", addr);
    }
    return 0;
}

void Pacman::write8(const std::uint16_t addr, const std::uint8_t val)
{
    if (0x3FFF < addr and addr < 0x4800) {
        vram[addr - 0x4000] = val;
    } else if (addr < 0x4FF0) {
        ram[addr - 0x4800] = val;
    } else if (addr < 0x5100) { // IO
        /**
         * Registers not used in Pac-Man:
         * 0x5002: ??? Aux board enable?
         * 0x5004: 1 player start lamp
         * 0x5005: 2 player start lamp
         * 0x5006: Coin lockout
         * 0x5007: Coin Counter
         * 0x5040-0x5060: TODO sound
         * 0x50C0-0x5100: Watchdog reset
         */

        if (addr == 0x5000) { // Interrupt enable
            interruptEnabled = val & 0b1;
        } else if (addr == 0x5001) { // Sound enable
            soundEnabled = val & 0b1;
        } else if (addr == 0x5003) { // Flip screen
            flipScreen = val & 0b1;
        }  else if (0x505F < addr and addr < 0x5070) {
            spriteNum[addr - 0x5060] = val;
        }
    } else {
        std::cerr << std::format("error: attempt to write {:>02X} at {:>04X}\n", val, addr);
    }
}

void Pacman::onKeyDown(SDL_Scancode scancode)
{
    switch (scancode) {
        case SDL_SCANCODE_UP: input0 |= up; input1 |= up; break;
        case SDL_SCANCODE_LEFT: input0 |= left; input1 |= left; break;
        case SDL_SCANCODE_RIGHT: input0 |= right; input1 |= right; break;
        case SDL_SCANCODE_DOWN: input0 |= down; input1 |= down; break;
        case SDL_SCANCODE_SPACE: input0 |= skip; break;
        case SDL_SCANCODE_T: input1 |= test; break;
        case SDL_SCANCODE_1: input0 |= coin1; break;
        case SDL_SCANCODE_RETURN: input1 |= onePlayer; break;
        case SDL_SCANCODE_2: input0 |= coin2; break;
        case SDL_SCANCODE_P: input1 |= twoPlayer; break;
        case SDL_SCANCODE_C: input0 |= credit; break;
        default: break;
    }
}

void Pacman::onKeyUp(SDL_Scancode scancode)
{
    switch (scancode) {
        case SDL_SCANCODE_UP: input0 &= ~up; input1 &= ~up; break;
        case SDL_SCANCODE_LEFT: input0 &= ~left; input1 &= ~left; break;
        case SDL_SCANCODE_RIGHT: input0 &= ~right; input1 &= ~right; break;
        case SDL_SCANCODE_DOWN: input0 &= ~down; input1 &= ~down; break;
        case SDL_SCANCODE_SPACE: input0 &= ~skip; break;
        case SDL_SCANCODE_T: input1 &= ~test; break;
        case SDL_SCANCODE_1: input0 &= ~coin1; break;
        case SDL_SCANCODE_RETURN: input1 &= ~onePlayer; break;
        case SDL_SCANCODE_2: input0 &= ~coin2; break;
        case SDL_SCANCODE_P: input1 &= ~twoPlayer; break;
        case SDL_SCANCODE_C: input0 &= ~credit; break;
        default: break;
    }
}

void Pacman::draw()
{
    /*
    for (int i {0}; i != vram.size(); ++i) {
        const std::uint8_t byte{vram[i]};
        const int col {i * 8 / screenHeight};
        int rowBegin {i * 8 % screenHeight};

        for (int bit {0}; bit != 8; ++bit, ++rowBegin) {
            const int row {screenHeight - rowBegin - 1};

            // transpose the array (rotate 90 degrees counter-clockwise)
            rasterBuffer[row][col] = (byte >> bit) & 0b1 ? 0xFFFFFFFF : 0xFF000000;
        }
    }*/

    SDL_UpdateTexture(texture, nullptr, rasterBuffer, pitch);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void Pacman::off()
{
    SDL_DestroyTexture(texture);
    texture = nullptr;
    SDL_DestroyRenderer(renderer);
    renderer = nullptr;
    SDL_DestroyWindow(window);
    window = nullptr;
}

bool Pacman::load(std::uint8_t* array, const std::string& path, const int addr, const int sz)
{
    std::ifstream file {path, std::ios::binary};

    if (!file.is_open()) {
        std::cerr << std::format("error: can't open file '{:s}'.\n", path);
        return false;
    }

    file.read(reinterpret_cast<char*>(array) + addr, sz);

    if (file.bad()) {
        std::cerr << std::format("error [errno={:d}]: failed when reading file '{:s}'.\n", errno, path);
        return false;
    }

    return true;
}

