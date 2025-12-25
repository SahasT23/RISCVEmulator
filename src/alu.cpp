/**
 * alu.cpp
 * 
 * Implementation of ALU operations.
 */

#include "alu.hpp"

Word ALU::execute(AluOp op, Word a, Word b) {
    SignedWord sa = static_cast<SignedWord>(a);
    SignedWord sb = static_cast<SignedWord>(b);

    switch (op) {
        // Basic arithmetic
        case AluOp::ADD:
            return a + b;
        case AluOp::SUB:
            return a - b;

        // Shifts (use lower 5 bits of b)
        case AluOp::SLL:
            return a << (b & 0x1F);
        case AluOp::SRL:
            return a >> (b & 0x1F);
        case AluOp::SRA:
            return static_cast<Word>(sa >> (b & 0x1F));

        // Comparisons
        case AluOp::SLT:
            return (sa < sb) ? 1 : 0;
        case AluOp::SLTU:
            return (a < b) ? 1 : 0;

        // Logical
        case AluOp::XOR:
            return a ^ b;
        case AluOp::OR:
            return a | b;
        case AluOp::AND:
            return a & b;

        // M extension - Multiply
        case AluOp::MUL: {
            int64_t result = static_cast<int64_t>(sa) * static_cast<int64_t>(sb);
            return static_cast<Word>(result & 0xFFFFFFFF);
        }
        case AluOp::MULH: {
            int64_t result = static_cast<int64_t>(sa) * static_cast<int64_t>(sb);
            return static_cast<Word>((result >> 32) & 0xFFFFFFFF);
        }
        case AluOp::MULHSU: {
            int64_t result = static_cast<int64_t>(sa) * static_cast<uint64_t>(b);
            return static_cast<Word>((result >> 32) & 0xFFFFFFFF);
        }
        case AluOp::MULHU: {
            uint64_t result = static_cast<uint64_t>(a) * static_cast<uint64_t>(b);
            return static_cast<Word>((result >> 32) & 0xFFFFFFFF);
        }

        // M extension - Divide
        case AluOp::DIV: {
            if (sb == 0) return 0xFFFFFFFF;  // Division by zero
            if (sa == std::numeric_limits<SignedWord>::min() && sb == -1) {
                return static_cast<Word>(sa);  // Overflow case
            }
            return static_cast<Word>(sa / sb);
        }
        case AluOp::DIVU: {
            if (b == 0) return 0xFFFFFFFF;
            return a / b;
        }
        case AluOp::REM: {
            if (sb == 0) return static_cast<Word>(sa);
            if (sa == std::numeric_limits<SignedWord>::min() && sb == -1) {
                return 0;
            }
            return static_cast<Word>(sa % sb);
        }
        case AluOp::REMU: {
            if (b == 0) return a;
            return a % b;
        }

        // Pass-through (for LUI)
        case AluOp::PASS_B:
            return b;

        case AluOp::NONE:
        default:
            return 0;
    }
}

bool ALU::branch_taken(InsType type, Word rs1_val, Word rs2_val) {
    SignedWord s1 = static_cast<SignedWord>(rs1_val);
    SignedWord s2 = static_cast<SignedWord>(rs2_val);

    switch (type) {
        case InsType::BEQ:  return rs1_val == rs2_val;
        case InsType::BNE:  return rs1_val != rs2_val;
        case InsType::BLT:  return s1 < s2;
        case InsType::BGE:  return s1 >= s2;
        case InsType::BLTU: return rs1_val < rs2_val;
        case InsType::BGEU: return rs1_val >= rs2_val;
        default:            return false;
    }
}

std::string ALU::op_name(AluOp op) {
    switch (op) {
        case AluOp::ADD:    return "ADD";
        case AluOp::SUB:    return "SUB";
        case AluOp::SLL:    return "SLL";
        case AluOp::SRL:    return "SRL";
        case AluOp::SRA:    return "SRA";
        case AluOp::SLT:    return "SLT";
        case AluOp::SLTU:   return "SLTU";
        case AluOp::XOR:    return "XOR";
        case AluOp::OR:     return "OR";
        case AluOp::AND:    return "AND";
        case AluOp::MUL:    return "MUL";
        case AluOp::MULH:   return "MULH";
        case AluOp::MULHSU: return "MULHSU";
        case AluOp::MULHU:  return "MULHU";
        case AluOp::DIV:    return "DIV";
        case AluOp::DIVU:   return "DIVU";
        case AluOp::REM:    return "REM";
        case AluOp::REMU:   return "REMU";
        case AluOp::PASS_B: return "PASS_B";
        case AluOp::NONE:   return "NONE";
        default:            return "UNKNOWN";
    }
}
