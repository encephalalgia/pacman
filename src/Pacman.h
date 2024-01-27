#ifndef PACMAN_PACMAN_H
#define PACMAN_PACMAN_H


#include <cstdint>
#include <array>
#include "SDL.h"
#include "z80.h"

/**
 * Pac-Man hardware for emulator: memory, i/o, video.
 */
class Pacman {
public:
    using Z80 = z80<Pacman>;
    using Tile = std::uint8_t[64];
    using Palette = std::uint32_t[4];

    // Constructor (also sets the active boolean).
    explicit Pacman(std::uint8_t ds);

    /**
     * Reads a byte from the provided address (memory mapped).
     * @param addr the address to read from
     * @return the byte read
     */
    [[nodiscard]] std::uint8_t read8(std::uint16_t addr) const;

    /**
     * Writes an byte to the provided address (memory mapped).
     * @param addr the address to write to
     * @param val the byte to write
     */
    void write8(std::uint16_t addr, std::uint8_t val);

    // Reads a word from the address given (uses read8).
    [[nodiscard]] std::uint16_t read16(const std::uint16_t addr) { return read8(addr + 1) << 8 | read8(addr); }

    // Writes a word to the address given (uses write8).
    void write16(const std::uint16_t addr, const std::uint16_t val) { write8(addr, val & 0xFF); write8(addr + 1, val >> 8); }

    /**
     * Input (unused in Pac-Man).
     * @param cpu A pointer to the CPU
     * @param port The port to read from
     * @return the byte read
     */
    [[nodiscard]] std::uint8_t input(const Pacman::Z80 *cpu, const std::uint8_t port) const { return 0; }

    /**
     * Output (port 0 sets interrupt vector).
     * @param cpu A pointer to the CPU
     * @param port The port to write to
     * @param val a byte to write
     */
    void output(const Pacman::Z80 *cpu, const std::uint8_t port, const std::uint8_t val) { if (port == 0) interruptVector = val; }

    /**
     * Call on key press.
     * @param scancode an SDL2 scancode representing the key pressed
     */
    void onKeyDown(SDL_Scancode scancode);

    /**
      * Call on key unpress.
      * @param scancode an SDL2 scancode representing the key unpressed
      */
    void onKeyUp(SDL_Scancode scancode);

    // Draws the current contents of VRAM to the screen.
    void draw();

    // Cleans up SDL2 objects.
    void off();

    // True if Pacman has been initialized successfully; false otherwise.
    bool active {true};
    std::uint8_t interruptVector {};
    bool interruptEnabled {false};
private:
    // input constants
    static constexpr std::uint8_t up {0b00000001U};
    static constexpr std::uint8_t left {0b00000010U};
    static constexpr std::uint8_t right {0b00000100U};
    static constexpr std::uint8_t down {0b00001000U};
    static constexpr std::uint8_t skip {0b00010000U};
    static constexpr std::uint8_t test {0b00010000U};
    static constexpr std::uint8_t coin1 {0b00100000U};
    static constexpr std::uint8_t onePlayer {0b00100000U};
    static constexpr std::uint8_t coin2 {0b01000000U};
    static constexpr std::uint8_t twoPlayer {0b01000000U};
    static constexpr std::uint8_t credit {0b10000000U};

    // display constants
    static constexpr int screenWidth {224};
    static constexpr int screenHeight {288};
    static constexpr int scaleFactor {3};
    static constexpr int pitch {screenWidth * sizeof(std::uint32_t)};

    /**
     * For loading a binary ROM file.
     * @param array a pointer to where to dump the file's contents
     * @param path path to the file
     * @param addr base address to start reading the file from
     * @param sz size of the array
     * @return true if there were no errors opening/reading the file; false otherwise
     */
    static bool load(std::uint8_t* array, const std::string& path, int addr, int sz);

    /**
     * Initializes SDL2 objects.
     * @return true if SDL2 encountered no errors initializing each object; false otherwise
     */
    bool initVideo();

    // Preloads the game's tiles, sprites, colors, and palettes from their respective ROMs into a convenient format.
    void preload();

    /**
     *
     * @param loc
     * @param x
     * @param y
     */
    void drawTile(int loc, int x, int y);

    const std::uint8_t dipswitch; // game settings
    std::uint8_t input0 {}, input1 {0b10000000U}; // setting cabinet mode to upright
    bool soundEnabled {false}, flipScreen {false};

    std::uint32_t rasterBuffer[screenHeight][screenWidth] {};
    SDL_Window* window {nullptr};
    SDL_Renderer* renderer {nullptr};
    SDL_Texture* texture {nullptr};

    std::uint8_t rom[0x4000] {};
    /**
     * 0x4000 - 0x43FF: 1024 vram (tile information)
     * 0x4400 - 0x47FF: 1024 vram (tile palettes)
     * 0x4800 - 0x4FEF: 2032 ram
     */
    std::uint8_t ram[0x1000] {};
    std::uint8_t spriteNum[0x10] {};

    std::uint8_t colorRom[0x20] {};
    std::uint8_t paletteRom[0x100] {};
    std::uint8_t tileRom[0x1000] {};
    std::uint8_t spriteRom[0x1000] {};

    std::array<Tile, 256> tiles {};
    std::array<Palette, 32> palettes {};
};


#endif //PACMAN_PACMAN_H
