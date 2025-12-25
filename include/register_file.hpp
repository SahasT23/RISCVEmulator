/**
 * register_file.hpp
 * 
 * 32 general-purpose registers (x0-x31).
 * x0 is hardwired to zero.
 */

#ifndef REGISTER_FILE_HPP
#define REGISTER_FILE_HPP

#include "common.hpp"

class RegisterFile {
public:
    RegisterFile();
    void reset();

    // Read register value
    Word read(int reg) const;

    // Write register value (writes to x0 are ignored)
    void write(int reg, Word value);

    // Display
    void dump() const;
    void dump_reg(int reg) const;

    // Direct access for debugging
    const std::array<Word, NUM_REGISTERS>& get_all() const;

private:
    std::array<Word, NUM_REGISTERS> regs;
};

#endif // REGISTER_FILE_HPP
