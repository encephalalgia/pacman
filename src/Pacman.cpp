#include "Pacman.h"
#include <fstream>
#include <iostream>

/**
 * For loading a binary ROM file.
 * @param array a pointer to where to dump the file's contents
 * @param path path to the file
 * @param addr base address to start reading the file from
 * @param sz size of the array
 * @return true if there were no errors opening/reading the file; false otherwise
 */
bool load(std::uint8_t* array, const std::string& path, const int addr, const int sz)
{
    std::ifstream file {path, std::ios::binary};

    if (!file.is_open()) {
        SDL_Log("error: can't open file '%s'.\n", path.c_str());
        return false;
    }

    file.read(reinterpret_cast<char*>(array) + addr, sz);

    if (file.bad()) {
        SDL_Log("error [errno=%d]: failed when reading file '%s'.\n", errno, path.c_str());
        return false;
    }

    return true;
}

/**
 * Decodes a strip of pixels for a tile or sprite. The word 'image' here refers to either a tile or sprite.
 * @param rom pointer to array of bytes
 * @param array where to put the decoded pixels
 * @param imageNum how many images have been decoded before this call (i.e. 0, 1, 2, ..., n)
 * @param stripNum how many strips have been decoded before this call (i.e. 0, 1, 2, ..., n)
 * @param pitch how many pixels per row in the image
 * @param x the coordinate of the strip's top-left corner on the x-axis
 * @param y the coordinate of the strip's top left corner on the y-axis
 */
void decodeStrip(const std::uint8_t* rom, std::uint8_t* array, const int imageNum, const int stripNum, const int pitch, const int x, const int y)
{
    for (int j {0}; j != 8; ++j) {
        const std::uint8_t byte {rom[j + (stripNum * 8) + (imageNum * pitch * (pitch / 8 * 2))]};
        const int xBase {(y * pitch * 4) + ((x + 1) * 8 - 1) - j};

        array[xBase + pitch * 3] = ((byte & 0x10) >> 3U) | ((byte & 0x01U) >> 0U);
        array[xBase + pitch * 2] = ((byte & 0x20) >> 4U) | ((byte & 0x02U) >> 1U);
        array[xBase + pitch * 1] = ((byte & 0x40) >> 5U) | ((byte & 0x04U) >> 2U);
        array[xBase + pitch * 0] = ((byte & 0x80) >> 6U) | ((byte & 0x08U) >> 3U);
    }
}

Pacman::Pacman(const std::uint8_t ds) : dipswitch{ds}
{
    const std::string dir {"roms/"};
    active &= load(rom, dir + "pacman.6e", 0, 0x1000);
    active &= load(rom, dir + "pacman.6f", 0x1000, 0x1000);
    active &= load(rom, dir + "pacman.6h", 0x2000, 0x1000);
    active &= load(rom, dir + "pacman.6j", 0x3000, 0x1000);
    if (active) active &= initVideo();
    if (active) active &= preload(dir);
}

std::uint8_t Pacman::read8(std::uint16_t addr) const
{
    addr &= 0x7FFFU;

    if (addr < 0x4000) {
        return rom[addr];
    } else if (addr < 0x5000) {
        return ram[addr - 0x4000];
    } else if (addr < 0x5100) { // Memory Mapped Registers
        /**
         * Read registers not used in Pac-Man
         * 0x5004: 1 player start lamp
         * 0x5005: 2 player start lamp
         * 0x5006: Coin lockout
         * 0x5007: Coin Counter
         */

        if (addr == 0x5003) {
            return flipScreen;
        } else if (addr < 0x5003 or (0x5007 < addr and addr < 0x5040)) { // IN0 (joystick and coin slot)
            return input0;
        } else if (addr < 0x5080) { // IN1 (joystick and start buttons)
            return input1;
        } else if (addr < 0x50C0) { // Dip Switch Settings
            return dipswitch;
        }
    } else {
        SDL_Log("error: attempt to read at %04X\n", addr);
    }
    return 0xFF;
}

void Pacman::write8(std::uint16_t addr, const std::uint8_t val)
{
    addr &= 0x7FFFU;

    if (addr < 0x4000) {
        SDL_Log("error: attempt to write to rom %02X at %04X\n", val, addr);
    } else if (addr < 0x5000) {
        ram[addr - 0x4000] = val;
    } else if (addr < 0x5100) { // Memory Mapped Registers
        /**
         * Write registers not used in Pac-Man:
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
            spritePos[addr - 0x5060] = val;
        }
    } else {
        SDL_Log("error: attempt to write %02X at %04X\n", val, addr);
    }
}

void Pacman::onKeyDown(SDL_Scancode scancode)
{
    switch (scancode) {
        case SDL_SCANCODE_UP: input0 &= ~up; input1 &= ~up; break;
        case SDL_SCANCODE_LEFT: input0 &= ~left; input1 &= ~left; break;
        case SDL_SCANCODE_RIGHT: input0 &= ~right; input1 &= ~right; break;
        case SDL_SCANCODE_DOWN: input0 &= ~down; input1 &= ~down; break;
        case SDL_SCANCODE_1: input0 &= ~coin1; break;
        case SDL_SCANCODE_RETURN: input1 &= ~onePlayer; break;
        case SDL_SCANCODE_2: input0 &= ~coin2; break;
        case SDL_SCANCODE_P: input1 &= ~twoPlayer; break;
        case SDL_SCANCODE_C: input0 &= ~credit; break;
        default: break;
    }
}

void Pacman::onKeyUp(SDL_Scancode scancode)
{
    switch (scancode) {
        case SDL_SCANCODE_UP: input0 |= up; input1 |= up; break;
        case SDL_SCANCODE_LEFT: input0 |= left; input1 |= left; break;
        case SDL_SCANCODE_RIGHT: input0 |= right; input1 |= right; break;
        case SDL_SCANCODE_DOWN: input0 |= down; input1 |= down; break;
        case SDL_SCANCODE_SPACE: input0 ^= rackAdvance; break; // switch 0=on, 1=off
        case SDL_SCANCODE_T: input1 ^= test; break; // switch 0=on, 1=off
        case SDL_SCANCODE_1: input0 |= coin1; break;
        case SDL_SCANCODE_RETURN: input1 |= onePlayer; break;
        case SDL_SCANCODE_2: input0 |= coin2; break;
        case SDL_SCANCODE_P: input1 |= twoPlayer; break;
        case SDL_SCANCODE_C: input0 |= credit; break;
        default: break;
    }
}

void Pacman::drawTile(const int loc, const int x, const int y)
{
    const Tile& tile {tiles[ram[loc]]};
    const Palette& palette {palettes[ram[loc + 0x400] & 0x3F]};

    for (int i {0}; i != 8; ++i) {
        for (int j {0}; j != 8; ++j)
            rasterBuffer[y*8 + i][x*8 + j] = palette[tile[j + (i << 3)]];
    }
}

void Pacman::drawSprite(const int loc, const int x, const int y)
{
    if (x > screenWidth) return;
    const std::uint8_t byte0 {ram[loc]}; // upper 6 bits are the sprite #, bit 1 is flip-x, bit 0 is flip-y
    const Sprite& sprite {sprites[byte0 >> 2]};
    const Palette& palette {palettes[ram[loc + 1] & 0x3F]};
    const bool flipX {static_cast<bool>(byte0 & 0b10)};
    const bool flipY {static_cast<bool>(byte0 & 0b01)};

    for (int i {0}; i != 16; ++i) {
        for (int j {0}; j != 16; ++j) {
            const std::uint32_t color {palette[sprite[j + (i << 4)]]};
            const int xCoord {x + (flipX ? 15 - j : j)};
            const int yCoord {y + (flipY ? 15 - i : i)};

            if (color == black or xCoord < 0 or xCoord >= screenWidth)
                continue;

            rasterBuffer[yCoord][xCoord] = color;
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

    // sprites (drawn in reverse order)
    for (int i {7}; i != -1; --i) {
        const int n {i * 2}; // step by 2
        drawSprite(n + 0xFF0, screenWidth - spritePos[n] + 15, screenHeight - spritePos[n + 1] - 16);
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
        SDL_Log("SDL_CreateWindow() failed. SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!renderer){
        SDL_Log("SDL_CreateRenderer() failed. SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ABGR8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                screenWidth, screenHeight);
    if (!texture){
        SDL_Log("SDL_CreateTexture() failed. SDL_Error: %s\n", SDL_GetError());
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

bool Pacman::preload(const std::string& dir)
{
    std::uint8_t colorRom[0x20] {};
    std::uint8_t paletteRom[0x100] {};
    std::uint8_t tileRom[0x1000] {};
    std::uint8_t spriteRom[0x1000] {};

    // load the color, palette, tile and sprite roms
    if (!(load(colorRom, dir + "82s123.7f", 0, 0x20)
            and load(paletteRom, dir + "82s126.4a", 0, 0x100)
            and load(tileRom, dir + "pacman.5e", 0, 0x1000)
            and load(spriteRom, dir + "pacman.5f", 0, 0x1000)))
        return false;

    // decode palettes
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

    // decode tiles
    for (int i {0}; i != tiles.size(); ++i) {
        Tile& tile {tiles[i]};

        decodeStrip(tileRom, tile, i, 0, 8, 0, 1);
        decodeStrip(tileRom, tile, i, 1, 8, 0, 0);
    }

    // decode sprites
    for (int i {0}; i != sprites.size(); ++i) {
        Sprite& sprite {sprites[i]};

        decodeStrip(spriteRom, sprite, i, 0, 16, 1, 3);
        decodeStrip(spriteRom, sprite, i, 1, 16, 1, 0);
        decodeStrip(spriteRom, sprite, i, 2, 16, 1, 1);
        decodeStrip(spriteRom, sprite, i, 3, 16, 1, 2);
        decodeStrip(spriteRom, sprite, i, 4, 16, 0, 3);
        decodeStrip(spriteRom, sprite, i, 5, 16, 0, 0);
        decodeStrip(spriteRom, sprite, i, 6, 16, 0, 1);
        decodeStrip(spriteRom, sprite, i, 7, 16, 0, 2);
    }

    return true;
}
