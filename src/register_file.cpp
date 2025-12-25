/**
 * register_file.cpp
 * 
 * Implementation of the 32 general-purpose registers.
 */

#include "register_file.hpp"

RegisterFile::RegisterFile() {
    reset();
}

void RegisterFile::reset() {
    regs.fill(0);
}

Word RegisterFile::read(int reg) const {
    if (reg < 0 || reg >= NUM_REGISTERS) {
        throw std::out_of_range("Invalid register: " + std::to_string(reg));
    }
    // x0 always returns 0
    return (reg == 0) ? 0 : regs[reg];
}

void RegisterFile::write(int reg, Word value) {
    if (reg < 0 || reg >= NUM_REGISTERS) {
        throw std::out_of_range("Invalid register: " + std::to_string(reg));
    }
    // Writes to x0 are ignored
    if (reg != 0) {
        regs[reg] = value;
    }
}

void RegisterFile::dump() const {
    std::cout << "Registers:\n";
    for (int row = 0; row < 8; row++) {
        std::cout << "  ";
        for (int col = 0; col < 4; col++) {
            int reg = row * 4 + col;
            std::cout << "x" << std::setw(2) << std::left << reg
                      << "/" << std::setw(4) << std::left << reg_name(reg)
                      << "= " << to_hex(regs[reg]);
            if (col < 3) std::cout << "  ";
        }
        std::cout << "\n";
    }
}

void RegisterFile::dump_reg(int reg) const {
    if (reg < 0 || reg >= NUM_REGISTERS) {
        std::cout << "Invalid register: " << reg << "\n";
        return;
    }
    std::cout << "x" << reg << "/" << reg_name(reg)
              << " = " << to_hex(regs[reg])
              << " (" << static_cast<SignedWord>(regs[reg]) << ")\n";
}

const std::array<Word, NUM_REGISTERS>& RegisterFile::get_all() const {
    return regs;
}
