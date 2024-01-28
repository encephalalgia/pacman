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
    using Palette = std::uint32_t[4];
    using Tile = std::uint8_t[64];
    using Sprite = std::uint8_t[256];

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
    [[nodiscard]] std::uint16_t read16(const std::uint16_t addr) const { return read8(addr + 1) << 8 | read8(addr); }

    // Writes a word to the address given (uses write8).
    void write16(const std::uint16_t addr, const std::uint16_t val) { write8(addr, val & 0xFF); write8(addr + 1, val >> 8); }

    /**
     * Input (unused in Pac-Man).
     * @param cpu A pointer to the CPU
     * @param port The port to read from
     * @return the byte read
     */
    [[nodiscard]] static std::uint8_t input(const Pacman::Z80 *cpu, const std::uint8_t port) { return 0; }

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
    static constexpr std::uint8_t rackAdvance {0b00010000U};
    static constexpr std::uint8_t test {0b00010000U};
    static constexpr std::uint8_t coin1 {0b00100000U};
    static constexpr std::uint8_t onePlayer {0b00100000U};
    static constexpr std::uint8_t coin2 {0b01000000U};
    static constexpr std::uint8_t twoPlayer {0b01000000U};
    static constexpr std::uint8_t credit {0b10000000U};

    // display constants
    static constexpr std::uint32_t black {0xFF000000};
    static constexpr int screenWidth {224};
    static constexpr int screenHeight {288};
    static constexpr int scaleFactor {3};
    static constexpr int pitch {screenWidth * sizeof(std::uint32_t)};

    /**
     * Draws a tile decoded from the tile rom.
     * @param loc the byte that selects the tile from tile ram
     * @param x the x coordinate [0,27] (tile's upper left corner)
     * @param y the y coordinate [0,35] (tile's upper left corner)
     */
    void drawTile(int loc, int x, int y);

    /**
     * Draws a sprite decoded from the sprite rom.
     * @param loc selects the sprite from sprite ram
     * @param x the x coordinate [0,27] (sprite's upper left corner)
     * @param y the y coordinate [0,35] (sprite's upper left corner)
     */
    void drawSprite(int loc, int x, int y);

    /**
     * Initializes SDL2 objects.
     * @return true if SDL2 encountered no errors initializing each object; false otherwise
     */
    bool initVideo();

    /**
     * Preloads the game's tiles, sprites, colors, and palettes from their respective ROMs into a convenient format.
     * @param dir the rom file's parent directory
     * @return true if preload encountered no errors preloading the images; false otherwise
     */
    bool preload(const std::string& dir);

    const std::uint8_t dipswitch; // game settings
    std::uint8_t input0 {0b10011111}, input1 {0b11111111U}; // default cabinet mode is upright and board test is off
    bool soundEnabled {false}, flipScreen {false};

    std::uint32_t rasterBuffer[screenHeight][screenWidth] {};
    SDL_Window* window {nullptr};
    SDL_Renderer* renderer {nullptr};
    SDL_Texture* texture {nullptr};

    /**
     * 0x0000-0x4000: 16384 game rom
     * 0x4000-0x43FF: 1024 video ram
     * 0x4400-0x47FF: 1024 color ram
     * 0x4800-0x4FEF: 2032 ram
     * 0x4FF0-0x4FFF: 16 sprite ram
     */
    std::uint8_t rom[0x4000] {};
    std::uint8_t ram[0x1000] {};
    std::uint8_t spritePos[0x10] {};

    std::array<Palette, 32> palettes {};
    std::array<Tile, 256> tiles {};
    std::array<Sprite, 64> sprites {};
};


#endif //PACMAN_PACMAN_H
