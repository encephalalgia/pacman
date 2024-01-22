#ifndef PACMAN_MEMORY_H
#define PACMAN_MEMORY_H

#include <cstdint>
#include <array>
#include "z80.h"

class Memory {
public:
    using Z80 = z80<Memory>;
    std::array<std::uint8_t, 0x10000> ram {};

    [[nodiscard]] std::uint8_t read8(const std::uint16_t addr) const { return ram[addr]; }
    void write8(const std::uint16_t addr, const std::uint8_t val) { ram[addr] = val; }

    [[nodiscard]] std::uint16_t read16(const std::uint16_t addr) const { return ram[static_cast<std::uint16_t>(addr + 1)] << 8 | ram[addr]; }
    void write16(const std::uint16_t addr, const std::uint16_t val) { ram[addr] = val & 0xFF; ram[static_cast<std::uint16_t>(addr + 1)] = val >> 8; }

    [[nodiscard]] std::uint8_t input(const Z80* cpu, const std::uint8_t port) const { return ram[port]; }
    void output(const Z80* cpu, const std::uint8_t port, const std::uint8_t val) { ram[port] = val; }
};


#endif //PACMAN_MEMORY_H
