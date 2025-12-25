/**
 * decoder.hpp
 * 
 * Instruction decoder.
 * Takes a 32-bit instruction word and extracts all fields,
 * determines instruction type, and generates control signals.
 */

#ifndef DECODER_HPP
#define DECODER_HPP

#include "common.hpp"

class Decoder {
public:
    // Decode a 32-bit instruction
    static Instruction decode(Word raw, Address pc = 0);

    // Display decoded instruction details
    static void print(const Instruction& ins);

private:
    // Opcode constants
    static constexpr Word OP_LUI    = 0b0110111;
    static constexpr Word OP_AUIPC  = 0b0010111;
    static constexpr Word OP_JAL    = 0b1101111;
    static constexpr Word OP_JALR   = 0b1100111;
    static constexpr Word OP_BRANCH = 0b1100011;
    static constexpr Word OP_LOAD   = 0b0000011;
    static constexpr Word OP_STORE  = 0b0100011;
    static constexpr Word OP_IMM    = 0b0010011;
    static constexpr Word OP_REG    = 0b0110011;
    static constexpr Word OP_SYSTEM = 0b1110011;

    // Bit extraction helpers
    static Word bits(Word val, int hi, int lo);
    static int get_rd(Word raw);
    static int get_rs1(Word raw);
    static int get_rs2(Word raw);
    static Word get_funct3(Word raw);
    static Word get_funct7(Word raw);

    // Immediate extraction
    static SignedWord imm_i(Word raw);
    static SignedWord imm_s(Word raw);
    static SignedWord imm_b(Word raw);
    static SignedWord imm_u(Word raw);
    static SignedWord imm_j(Word raw);

    // Disassembly generation
    static std::string disassemble(const Instruction& ins);
};

#endif // DECODER_HPP
