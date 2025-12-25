/**
 * common.hpp
 * 
 * Shared types, constants, and utility functions used throughout the emulator.
 */

#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <array>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <optional>
#include <limits>

// =============================================================================
// Basic Types
// =============================================================================

using Word = uint32_t;          // 32-bit unsigned (main data type for RV32)
using SignedWord = int32_t;     // 32-bit signed
using Address = uint32_t;       // Memory address
using Byte = uint8_t;           // 8-bit
using HalfWord = uint16_t;      // 16-bit

constexpr int NUM_REGISTERS = 32;

// =============================================================================
// Instruction Format
// =============================================================================

enum class Format {
    R,      // Register-register (add, sub, etc.)
    I,      // Immediate (addi, loads, jalr)
    S,      // Store (sb, sh, sw)
    B,      // Branch (beq, bne, etc.)
    U,      // Upper immediate (lui, auipc)
    J,      // Jump (jal)
    UNKNOWN
};

// =============================================================================
// ALU Operations
// =============================================================================

enum class AluOp {
    ADD, SUB,
    SLL, SRL, SRA,          // Shifts
    SLT, SLTU,              // Set less than
    XOR, OR, AND,           // Logical
    MUL, MULH, MULHSU, MULHU,   // Multiply
    DIV, DIVU, REM, REMU,       // Divide
    PASS_B,                 // Pass second operand through (for LUI)
    NONE
};

// =============================================================================
// Instruction Types
// =============================================================================

enum class InsType {
    // R-type
    ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,
    // I-type arithmetic
    ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI,
    // Loads
    LB, LH, LW, LBU, LHU,
    // Stores
    SB, SH, SW,
    // Branches
    BEQ, BNE, BLT, BGE, BLTU, BGEU,
    // Jumps
    JAL, JALR,
    // Upper immediate
    LUI, AUIPC,
    // M extension
    MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU,
    // System
    ECALL, EBREAK,
    // Invalid
    UNKNOWN
};

// =============================================================================
// Decoded Instruction
// =============================================================================

struct Instruction {
    Word raw = 0x00000013;      // Raw 32-bit encoding (default: NOP)
    InsType type = InsType::ADDI;
    Format format = Format::I;
    
    int rd = 0;                 // Destination register
    int rs1 = 0;                // Source register 1
    int rs2 = 0;                // Source register 2
    SignedWord imm = 0;         // Immediate (sign-extended)
    
    // Control signals
    bool reg_write = false;     // Write to register file?
    bool mem_read = false;      // Read from memory?
    bool mem_write = false;     // Write to memory?
    bool mem_to_reg = false;    // Memory result to register?
    bool branch = false;        // Branch instruction?
    bool jump = false;          // Jump instruction?
    bool alu_src = false;       // Use immediate as ALU input B?
    AluOp alu_op = AluOp::NONE;
    
    Address pc = 0;             // PC where fetched
    std::string text;           // Disassembly string
    
    bool is_nop() const { return raw == 0x00000013 || raw == 0; }
};

// =============================================================================
// Pipeline Registers
// =============================================================================

struct IF_ID {
    Word instruction = 0x13;
    Address pc = 0;
    Address next_pc = 4;
    bool valid = false;
    
    void flush() { instruction = 0x13; pc = 0; next_pc = 4; valid = false; }
};

struct ID_EX {
    Instruction ins;
    Word rs1_val = 0;
    Word rs2_val = 0;
    Address pc = 0;
    Address next_pc = 4;
    bool valid = false;
    
    void flush() { ins = Instruction(); rs1_val = 0; rs2_val = 0; pc = 0; next_pc = 4; valid = false; }
};

struct EX_MEM {
    Instruction ins;
    Word alu_result = 0;
    Word rs2_val = 0;
    Address branch_target = 0;
    bool branch_taken = false;
    bool valid = false;
    
    void flush() { ins = Instruction(); alu_result = 0; rs2_val = 0; branch_target = 0; branch_taken = false; valid = false; }
};

struct MEM_WB {
    Instruction ins;
    Word alu_result = 0;
    Word mem_data = 0;
    bool valid = false;
    
    void flush() { ins = Instruction(); alu_result = 0; mem_data = 0; valid = false; }
};

// =============================================================================
// Forwarding
// =============================================================================

enum class Forward {
    NONE,       // Use register file value
    EX_MEM,     // Forward from EX/MEM
    MEM_WB      // Forward from MEM/WB
};

// =============================================================================
// Utility Functions
// =============================================================================

// Sign extend from a given bit width to 32 bits
inline SignedWord sign_extend(Word value, int bits) {
    Word sign_bit = 1U << (bits - 1);
    if (value & sign_bit) {
        Word mask = ~((1U << bits) - 1);
        return static_cast<SignedWord>(value | mask);
    }
    return static_cast<SignedWord>(value);
}

// Format as hex string
inline std::string to_hex(Word value, int width = 8) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setfill('0') << std::setw(width) << value;
    return oss.str();
}

// Register ABI name
inline std::string reg_name(int reg) {
    static const char* names[] = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };
    if (reg >= 0 && reg < 32) return names[reg];
    return "x" + std::to_string(reg);
}

// Instruction type to string
inline std::string ins_name(InsType type) {
    switch (type) {
        case InsType::ADD: return "add";
        case InsType::SUB: return "sub";
        case InsType::SLL: return "sll";
        case InsType::SLT: return "slt";
        case InsType::SLTU: return "sltu";
        case InsType::XOR: return "xor";
        case InsType::SRL: return "srl";
        case InsType::SRA: return "sra";
        case InsType::OR: return "or";
        case InsType::AND: return "and";
        case InsType::ADDI: return "addi";
        case InsType::SLTI: return "slti";
        case InsType::SLTIU: return "sltiu";
        case InsType::XORI: return "xori";
        case InsType::ORI: return "ori";
        case InsType::ANDI: return "andi";
        case InsType::SLLI: return "slli";
        case InsType::SRLI: return "srli";
        case InsType::SRAI: return "srai";
        case InsType::LB: return "lb";
        case InsType::LH: return "lh";
        case InsType::LW: return "lw";
        case InsType::LBU: return "lbu";
        case InsType::LHU: return "lhu";
        case InsType::SB: return "sb";
        case InsType::SH: return "sh";
        case InsType::SW: return "sw";
        case InsType::BEQ: return "beq";
        case InsType::BNE: return "bne";
        case InsType::BLT: return "blt";
        case InsType::BGE: return "bge";
        case InsType::BLTU: return "bltu";
        case InsType::BGEU: return "bgeu";
        case InsType::JAL: return "jal";
        case InsType::JALR: return "jalr";
        case InsType::LUI: return "lui";
        case InsType::AUIPC: return "auipc";
        case InsType::MUL: return "mul";
        case InsType::MULH: return "mulh";
        case InsType::MULHSU: return "mulhsu";
        case InsType::MULHU: return "mulhu";
        case InsType::DIV: return "div";
        case InsType::DIVU: return "divu";
        case InsType::REM: return "rem";
        case InsType::REMU: return "remu";
        case InsType::ECALL: return "ecall";
        case InsType::EBREAK: return "ebreak";
        default: return "unknown";
    }
}

#endif // COMMON_HPP
