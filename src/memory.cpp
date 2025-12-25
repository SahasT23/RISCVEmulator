/**
 * memory.cpp
 * 
 * Implementation of the memory subsystem.
 * Uses sparse storage (std::map) so we don't allocate 4GB.
 * Little-endian byte order.
 */

#include "memory.hpp"

Memory::Memory() : read_count(0), write_count(0) {}

void Memory::reset() {
    mem.clear();
    read_count = 0;
    write_count = 0;
}

// =============================================================================
// Byte Access
// =============================================================================

Byte Memory::read_byte(Address addr) {
    read_count++;
    auto it = mem.find(addr);
    return (it != mem.end()) ? it->second : 0;
}

void Memory::write_byte(Address addr, Byte value) {
    write_count++;
    mem[addr] = value;
}

// =============================================================================
// Half-Word Access (16-bit, little-endian)
// =============================================================================

HalfWord Memory::read_half(Address addr) {
    Byte lo = read_byte(addr);
    Byte hi = read_byte(addr + 1);
    return static_cast<HalfWord>(lo) | (static_cast<HalfWord>(hi) << 8);
}

void Memory::write_half(Address addr, HalfWord value) {
    write_byte(addr, value & 0xFF);
    write_byte(addr + 1, (value >> 8) & 0xFF);
}

// =============================================================================
// Word Access (32-bit, little-endian)
// =============================================================================

Word Memory::read_word(Address addr) {
    Byte b0 = read_byte(addr);
    Byte b1 = read_byte(addr + 1);
    Byte b2 = read_byte(addr + 2);
    Byte b3 = read_byte(addr + 3);
    return static_cast<Word>(b0) |
           (static_cast<Word>(b1) << 8) |
           (static_cast<Word>(b2) << 16) |
           (static_cast<Word>(b3) << 24);
}

void Memory::write_word(Address addr, Word value) {
    write_byte(addr, value & 0xFF);
    write_byte(addr + 1, (value >> 8) & 0xFF);
    write_byte(addr + 2, (value >> 16) & 0xFF);
    write_byte(addr + 3, (value >> 24) & 0xFF);
}

// =============================================================================
// Signed Loads
// =============================================================================

SignedWord Memory::read_byte_signed(Address addr) {
    Byte val = read_byte(addr);
    return sign_extend(val, 8);
}

SignedWord Memory::read_half_signed(Address addr) {
    HalfWord val = read_half(addr);
    return sign_extend(val, 16);
}

// =============================================================================
// Bulk Operations
// =============================================================================

void Memory::write_block(Address addr, const std::vector<Word>& words) {
    for (const Word& w : words) {
        write_word(addr, w);
        addr += 4;
    }
}

void Memory::write_bytes(Address addr, const std::vector<Byte>& bytes) {
    for (Byte b : bytes) {
        write_byte(addr++, b);
    }
}

// =============================================================================
// Display
// =============================================================================

void Memory::dump(Address start, size_t bytes) const {
    std::cout << "Memory [" << to_hex(start) << " - " << to_hex(start + bytes - 1) << "]:\n";
    
    for (size_t i = 0; i < bytes; i += 16) {
        Address addr = start + i;
        std::cout << to_hex(addr) << ": ";
        
        // Hex bytes
        for (size_t j = 0; j < 16 && (i + j) < bytes; j++) {
            auto it = mem.find(addr + j);
            if (it != mem.end()) {
                std::cout << std::hex << std::setfill('0') << std::setw(2)
                          << static_cast<int>(it->second) << " ";
            } else {
                std::cout << ".. ";
            }
            if (j == 7) std::cout << " ";
        }
        
        // ASCII
        std::cout << " |";
        for (size_t j = 0; j < 16 && (i + j) < bytes; j++) {
            auto it = mem.find(addr + j);
            if (it != mem.end()) {
                char c = static_cast<char>(it->second);
                std::cout << ((c >= 32 && c < 127) ? c : '.');
            } else {
                std::cout << '.';
            }
        }
        std::cout << "|\n";
    }
    std::cout << std::dec;
}

void Memory::dump_words(Address start, size_t count) const {
    std::cout << "Memory words [" << to_hex(start) << "]:\n";
    for (size_t i = 0; i < count; i++) {
        Address addr = start + i * 4;
        Word val = 0;
        for (int j = 0; j < 4; j++) {
            auto it = mem.find(addr + j);
            if (it != mem.end()) {
                val |= static_cast<Word>(it->second) << (j * 8);
            }
        }
        std::cout << "  " << to_hex(addr) << ": " << to_hex(val) << "\n";
    }
}

// =============================================================================
// Stats
// =============================================================================

size_t Memory::bytes_used() const {
    return mem.size();
}

uint64_t Memory::get_read_count() const {
    return read_count;
}

uint64_t Memory::get_write_count() const {
    return write_count;
}
