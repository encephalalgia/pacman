#include "Pacman.h"
#include <format>
#include <fstream>
#include <iostream>

Pacman::Pacman(const std::uint8_t ds) : dipswitch{ds}
{
    const std::string dir {"roms/"};
    active &= load(rom, dir + "pacman.6e", 0, 0x1000);
    active &= load(rom, dir + "pacman.6f", 0x1000, 0x1000);
    active &= load(rom, dir + "pacman.6h", 0x2000, 0x1000);
    active &= load(rom, dir + "pacman.6j", 0x3000, 0x1000);
    active &= load(colorRom, dir + "82s123.7f", 0, 0x20);
    active &= load(paletteRom, dir + "82s126.4a", 0, 0x100);
    active &= load(tileRom, dir + "pacman.5e", 0, 0x1000);
    active &= load(spriteRom, dir + "pacman.5f", 0, 0x1000);
    if (active) active &= initVideo();
    if (active) preload();
}

std::uint8_t Pacman::read8(std::uint16_t addr) const
{
    addr &= 0x7FFFU;

    if (addr < 0x4000) {
        return rom[addr];
    } else if (addr < 0x5000) {
        if (addr - 0x4000 < 0 or addr - 0x4000 >= 0x1000)
            std::cout << std::format("READ RAM: {:0>4X}\n", addr - 0x4000);
        return ram[addr - 0x4000];
    } else if (addr < 0x5100) { // IO
        if (addr == 0x5003) {
            return flipScreen;
        } else if (0x5007 < addr and addr < 0x5040) { // IN0 (joystick and coin slot)
            return input0;
        } else if (addr < 0x5080) { // IN1 (joystick and start buttons)
            return input1;
        } else if (addr < 0x50C0) { // Dip Switch Settings
            return dipswitch;
        }
    } else {
        std::cout << std::format("error: attempt to read at {:0>4X}\n", addr);
    }
    return 0xFF;
}

void Pacman::write8(std::uint16_t addr, const std::uint8_t val)
{
    addr &= 0x7FFFU;

    if (addr < 0x4000) {
        std::cout << std::format("error: attempt to write to rom {:0>2X} at {:0>4X}\n", val, addr);
    } else if (addr < 0x5000) {
        if (addr - 0x4000 < 0 or addr - 0x4000 >= 0x1000)
            std::cout << std::format("WRITE RAM: {:0>2X} at {:0>4X}\n", val, addr - 0x4000);
        ram[addr - 0x4000] = val;
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
        std::cout << std::format("error: attempt to write {:0>2X} at {:0>4X}\n", val, addr);
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

void Pacman::drawTile(const int loc, const int x, const int y)
{
    const Tile& tile {tiles[ram[loc]]};
    const Palette& palette {palettes[ram[loc + 0x400] & 0x0F]};

    for (int i {0}; i != 8; ++i) {
        for (int j {0}; j != 8; ++j) {
            //std::cout << std::format("(x={:d},y={:d}), (px={:d},py={:d}), loc={:d}, tile_no={:d}, palette_no={:d}, pixel={:d}, color={:0>8X}\n", x, y, x*8 + j, y*8 + i, loc, ram[loc], ram[loc + 0x400], tile[j+(i*8)], palette[tile[j + (i * 8)]]);
            rasterBuffer[y*8 + i][x*8 + j] = palette[tile[j + (i * 8)]];
        }
    }
}

void Pacman::draw()
{
    // bottom of screen
    for (int y {0}; y != 2; ++y) {
        for (int x {2}; x != 30; ++x)
            drawTile(x + (y * 32), 29 - x, y + 34);
    }

    // middle of screen
    for (int x {0}; x != 28; ++x) {
        for (int y {0}; y != 32; ++y)
            drawTile(64 + y + (x * 32), 27 - x, y + 2);
    }

    // top of screen
    for (int y {0}; y != 2; ++y) {
        for (int x {2}; x != 30; ++x)
            drawTile(960 + x + (y * 32), 29 - x, y);
    }

    SDL_UpdateTexture(texture, nullptr, rasterBuffer, pitch);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

bool Pacman::initVideo()
{
    // display contents
    window = SDL_CreateWindow(
            "Pac-Man",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            screenWidth * scaleFactor,
            screenHeight * scaleFactor,
            SDL_WINDOW_SHOWN);
    if (!window) {
        std::cout << std::format("SDL_CreateWindow() failed. SDL_Error: {:s}\n", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!renderer){
        std::cout << std::format("SDL_CreateRenderer() failed. SDL_Error: {:s}\n", SDL_GetError());
        return false;
    }

    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ABGR8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                screenWidth, screenHeight);
    if (!texture){
        std::cout << std::format("SDL_CreateTexture() failed. SDL_Error: {:s}\n", SDL_GetError());
        return false;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderSetLogicalSize(renderer, screenWidth, screenHeight);

    // initialize screen
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    return true;
}


void Pacman::off()
{
    if (texture != nullptr) SDL_DestroyTexture(texture);
    texture = nullptr;
    if (renderer != nullptr) SDL_DestroyRenderer(renderer);
    renderer = nullptr;
    if (window != nullptr) SDL_DestroyWindow(window);
    window = nullptr;
}

bool Pacman::load(std::uint8_t* array, const std::string& path, const int addr, const int sz)
{
    std::ifstream file {path, std::ios::binary};

    if (!file.is_open()) {
        std::cout << std::format("error: can't open file '{:s}'.\n", path);
        return false;
    }

    file.read(reinterpret_cast<char*>(array) + addr, sz);

    if (file.bad()) {
        std::cout << std::format("error [errno={:d}]: failed when reading file '{:s}'.\n", errno, path);
        return false;
    }

    return true;
}

void Pacman::preload()
{
    // load tiles
    for (int i {0}; i != tiles.size(); ++i) {
        Tile& tile {tiles[i]};
        for (int j {0}; j != 8; ++j) {
            const std::uint8_t byte {tileRom[j + (i * 16)]};
            const int msb {byte >> 4U};
            const int lsb {byte & 0xF};
            const int coord {39 - j}; // first 4 bytes draw the bottom row

            tile[coord + 24] = (((msb & 0b0001U) << 1U) | (lsb & 0b0001U)) >> 0U;
            tile[coord + 16] = (((msb & 0b0010U) << 1U) | (lsb & 0b0010U)) >> 1U;
            tile[coord + 8] = (((msb & 0b0100U) << 1U) | (lsb & 0b0100U)) >> 2U;
            tile[coord + 0] = (((msb & 0b1000U) << 1U) | (lsb & 0b1000U)) >> 3U;
        }

        for (int j {0}; j != 8; ++j) {
            const std::uint8_t byte {tileRom[j + 8 + (i * 16)]};
            const int msb {byte >> 4U};
            const int lsb {byte & 0xF};
            const int coord {7 - j}; // last 4 bytes draw the top row

            tile[coord + 24] = (((msb & 0b0001U) << 1U) | (lsb & 0b0001U)) >> 0U;
            tile[coord + 16] = (((msb & 0b0010U) << 1U) | (lsb & 0b0010U)) >> 1U;
            tile[coord + 8] = (((msb & 0b0100U) << 1U) | (lsb & 0b0100U)) >> 2U;
            tile[coord + 0] = (((msb & 0b1000U) << 1U) | (lsb & 0b1000U)) >> 3U;
        }
    }

    // load palettes
    for (int i {0}; i != palettes.size(); ++i) {
        Palette& palette {palettes[i]};
        for (int j {0}; j != 4; ++j) {
            const std::uint8_t color {colorRom[paletteRom[j + (i * 4)] & 0x0F]};
            std::uint8_t r {static_cast<uint8_t>(
                                    (((color >> 0U) & 0b1) * 0x21)
                                    + (((color >> 1U) & 0b1) * 0x47)
                                    + (((color >> 2U) & 0b1) * 0x97))};
            std::uint8_t g {static_cast<uint8_t>(
                                    (((color >> 3U) & 0b1) * 0x21)
                                    + (((color >> 4U) & 0b1) * 0x47)
                                    + (((color >> 5U) & 0b1) * 0x97))};
            std::uint8_t b {static_cast<uint8_t>(
                                    (((color >> 6U) & 0b1) * 0x51)
                                    + (((color >> 7U) & 0b1) * 0xAE))};
            palette[j] = (0xFF << 24U) | (b << 16U) | (g << 8U) | (r << 0U);
        }
    }
}