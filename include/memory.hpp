/**
 * memory.hpp
 * 
 * Memory subsystem for the RISC-V emulator.
 * Byte-addressable, little-endian, sparse storage.
 */

#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "common.hpp"

class Memory {
public:
    // Memory layout constants
    static constexpr Address TEXT_BASE = 0x00000000;
    static constexpr Address DATA_BASE = 0x10000000;
    static constexpr Address STACK_TOP = 0x7FFFFFF0;

    Memory();
    void reset();

    // Byte access
    Byte read_byte(Address addr);
    void write_byte(Address addr, Byte value);

    // Half-word access (16-bit)
    HalfWord read_half(Address addr);
    void write_half(Address addr, HalfWord value);

    // Word access (32-bit)
    Word read_word(Address addr);
    void write_word(Address addr, Word value);

    // Signed loads
    SignedWord read_byte_signed(Address addr);
    SignedWord read_half_signed(Address addr);

    // Bulk operations
    void write_block(Address addr, const std::vector<Word>& words);
    void write_bytes(Address addr, const std::vector<Byte>& bytes);

    // Display
    void dump(Address start, size_t bytes = 64) const;
    void dump_words(Address start, size_t count = 8) const;

    // Stats
    size_t bytes_used() const;
    uint64_t get_read_count() const;
    uint64_t get_write_count() const;

private:
    std::map<Address, Byte> mem;
    uint64_t read_count;
    uint64_t write_count;
};

#endif // MEMORY_HPP
