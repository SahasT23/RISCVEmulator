/**
 * decoder.cpp
 * 
 * Implementation of instruction decoding.
 */

#include "decoder.hpp"

// =============================================================================
// Bit Extraction
// =============================================================================

Word Decoder::bits(Word val, int hi, int lo) {
    return (val >> lo) & ((1U << (hi - lo + 1)) - 1);
}

int Decoder::get_rd(Word raw)      { return static_cast<int>(bits(raw, 11, 7)); }
int Decoder::get_rs1(Word raw)     { return static_cast<int>(bits(raw, 19, 15)); }
int Decoder::get_rs2(Word raw)     { return static_cast<int>(bits(raw, 24, 20)); }
Word Decoder::get_funct3(Word raw) { return bits(raw, 14, 12); }
Word Decoder::get_funct7(Word raw) { return bits(raw, 31, 25); }

// =============================================================================
// Immediate Extraction (with sign extension)
// =============================================================================

SignedWord Decoder::imm_i(Word raw) {
    // imm[11:0] = raw[31:20]
    return sign_extend(bits(raw, 31, 20), 12);
}

SignedWord Decoder::imm_s(Word raw) {
    // imm[11:5] = raw[31:25], imm[4:0] = raw[11:7]
    Word imm = (bits(raw, 31, 25) << 5) | bits(raw, 11, 7);
    return sign_extend(imm, 12);
}

SignedWord Decoder::imm_b(Word raw) {
    // imm[12|10:5] = raw[31:25], imm[4:1|11] = raw[11:7]
    Word imm = (bits(raw, 31, 31) << 12) |
               (bits(raw, 7, 7) << 11) |
               (bits(raw, 30, 25) << 5) |
               (bits(raw, 11, 8) << 1);
    return sign_extend(imm, 13);
}

SignedWord Decoder::imm_u(Word raw) {
    // imm[31:12] = raw[31:12]
    return static_cast<SignedWord>(raw & 0xFFFFF000);
}

SignedWord Decoder::imm_j(Word raw) {
    // imm[20|10:1|11|19:12] = raw[31:12]
    Word imm = (bits(raw, 31, 31) << 20) |
               (bits(raw, 19, 12) << 12) |
               (bits(raw, 20, 20) << 11) |
               (bits(raw, 30, 21) << 1);
    return sign_extend(imm, 21);
}

// =============================================================================
// Disassembly
// =============================================================================

std::string Decoder::disassemble(const Instruction& ins) {
    std::ostringstream oss;
    std::string name = ins_name(ins.type);

    switch (ins.format) {
        case Format::R:
            oss << name << " " << reg_name(ins.rd) << ", "
                << reg_name(ins.rs1) << ", " << reg_name(ins.rs2);
            break;

        case Format::I:
            if (ins.mem_read) {
                // Load: lw rd, offset(rs1)
                oss << name << " " << reg_name(ins.rd) << ", "
                    << ins.imm << "(" << reg_name(ins.rs1) << ")";
            } else if (ins.type == InsType::JALR) {
                oss << name << " " << reg_name(ins.rd) << ", "
                    << reg_name(ins.rs1) << ", " << ins.imm;
            } else if (ins.type == InsType::ECALL || ins.type == InsType::EBREAK) {
                oss << name;
            } else {
                oss << name << " " << reg_name(ins.rd) << ", "
                    << reg_name(ins.rs1) << ", " << ins.imm;
            }
            break;

        case Format::S:
            oss << name << " " << reg_name(ins.rs2) << ", "
                << ins.imm << "(" << reg_name(ins.rs1) << ")";
            break;

        case Format::B:
            oss << name << " " << reg_name(ins.rs1) << ", "
                << reg_name(ins.rs2) << ", " << ins.imm;
            break;

        case Format::U:
            oss << name << " " << reg_name(ins.rd) << ", "
                << to_hex((static_cast<Word>(ins.imm) >> 12) & 0xFFFFF, 5);
            break;

        case Format::J:
            oss << name << " " << reg_name(ins.rd) << ", " << ins.imm;
            break;

        default:
            oss << "unknown";
            break;
    }

    return oss.str();
}

// =============================================================================
// Main Decode Function
// =============================================================================

Instruction Decoder::decode(Word raw, Address pc) {
    Instruction ins;
    ins.raw = raw;
    ins.pc = pc;
    ins.rd = get_rd(raw);
    ins.rs1 = get_rs1(raw);
    ins.rs2 = get_rs2(raw);

    Word opcode = bits(raw, 6, 0);
    Word funct3 = get_funct3(raw);
    Word funct7 = get_funct7(raw);

    switch (opcode) {
        // =================================================================
        // LUI
        // =================================================================
        case OP_LUI:
            ins.type = InsType::LUI;
            ins.format = Format::U;
            ins.imm = imm_u(raw);
            ins.reg_write = true;
            ins.alu_src = true;
            ins.alu_op = AluOp::PASS_B;
            break;

        // =================================================================
        // AUIPC
        // =================================================================
        case OP_AUIPC:
            ins.type = InsType::AUIPC;
            ins.format = Format::U;
            ins.imm = imm_u(raw);
            ins.reg_write = true;
            ins.alu_src = true;
            ins.alu_op = AluOp::ADD;
            break;

        // =================================================================
        // JAL
        // =================================================================
        case OP_JAL:
            ins.type = InsType::JAL;
            ins.format = Format::J;
            ins.imm = imm_j(raw);
            ins.reg_write = true;
            ins.jump = true;
            break;

        // =================================================================
        // JALR
        // =================================================================
        case OP_JALR:
            ins.type = InsType::JALR;
            ins.format = Format::I;
            ins.imm = imm_i(raw);
            ins.reg_write = true;
            ins.jump = true;
            ins.alu_src = true;
            ins.alu_op = AluOp::ADD;
            break;

        // =================================================================
        // Branches
        // =================================================================
        case OP_BRANCH:
            ins.format = Format::B;
            ins.imm = imm_b(raw);
            ins.branch = true;
            switch (funct3) {
                case 0b000: ins.type = InsType::BEQ; break;
                case 0b001: ins.type = InsType::BNE; break;
                case 0b100: ins.type = InsType::BLT; break;
                case 0b101: ins.type = InsType::BGE; break;
                case 0b110: ins.type = InsType::BLTU; break;
                case 0b111: ins.type = InsType::BGEU; break;
                default:    ins.type = InsType::UNKNOWN; break;
            }
            break;

        // =================================================================
        // Loads
        // =================================================================
        case OP_LOAD:
            ins.format = Format::I;
            ins.imm = imm_i(raw);
            ins.reg_write = true;
            ins.mem_read = true;
            ins.mem_to_reg = true;
            ins.alu_src = true;
            ins.alu_op = AluOp::ADD;
            switch (funct3) {
                case 0b000: ins.type = InsType::LB; break;
                case 0b001: ins.type = InsType::LH; break;
                case 0b010: ins.type = InsType::LW; break;
                case 0b100: ins.type = InsType::LBU; break;
                case 0b101: ins.type = InsType::LHU; break;
                default:    ins.type = InsType::UNKNOWN; break;
            }
            break;

        // =================================================================
        // Stores
        // =================================================================
        case OP_STORE:
            ins.format = Format::S;
            ins.imm = imm_s(raw);
            ins.mem_write = true;
            ins.alu_src = true;
            ins.alu_op = AluOp::ADD;
            switch (funct3) {
                case 0b000: ins.type = InsType::SB; break;
                case 0b001: ins.type = InsType::SH; break;
                case 0b010: ins.type = InsType::SW; break;
                default:    ins.type = InsType::UNKNOWN; break;
            }
            break;

        // =================================================================
        // I-type Arithmetic
        // =================================================================
        case OP_IMM:
            ins.format = Format::I;
            ins.imm = imm_i(raw);
            ins.reg_write = true;
            ins.alu_src = true;
            switch (funct3) {
                case 0b000:
                    ins.type = InsType::ADDI;
                    ins.alu_op = AluOp::ADD;
                    break;
                case 0b010:
                    ins.type = InsType::SLTI;
                    ins.alu_op = AluOp::SLT;
                    break;
                case 0b011:
                    ins.type = InsType::SLTIU;
                    ins.alu_op = AluOp::SLTU;
                    break;
                case 0b100:
                    ins.type = InsType::XORI;
                    ins.alu_op = AluOp::XOR;
                    break;
                case 0b110:
                    ins.type = InsType::ORI;
                    ins.alu_op = AluOp::OR;
                    break;
                case 0b111:
                    ins.type = InsType::ANDI;
                    ins.alu_op = AluOp::AND;
                    break;
                case 0b001:
                    ins.type = InsType::SLLI;
                    ins.alu_op = AluOp::SLL;
                    ins.imm = ins.rs2;  // shamt in rs2 field
                    break;
                case 0b101:
                    ins.imm = ins.rs2;  // shamt in rs2 field
                    if (funct7 & 0x20) {
                        ins.type = InsType::SRAI;
                        ins.alu_op = AluOp::SRA;
                    } else {
                        ins.type = InsType::SRLI;
                        ins.alu_op = AluOp::SRL;
                    }
                    break;
                default:
                    ins.type = InsType::UNKNOWN;
                    break;
            }
            break;

        // =================================================================
        // R-type (including M extension)
        // =================================================================
        case OP_REG:
            ins.format = Format::R;
            ins.reg_write = true;
            ins.imm = 0;

            if (funct7 == 0x01) {
                // M extension
                switch (funct3) {
                    case 0b000: ins.type = InsType::MUL;    ins.alu_op = AluOp::MUL; break;
                    case 0b001: ins.type = InsType::MULH;   ins.alu_op = AluOp::MULH; break;
                    case 0b010: ins.type = InsType::MULHSU; ins.alu_op = AluOp::MULHSU; break;
                    case 0b011: ins.type = InsType::MULHU;  ins.alu_op = AluOp::MULHU; break;
                    case 0b100: ins.type = InsType::DIV;    ins.alu_op = AluOp::DIV; break;
                    case 0b101: ins.type = InsType::DIVU;   ins.alu_op = AluOp::DIVU; break;
                    case 0b110: ins.type = InsType::REM;    ins.alu_op = AluOp::REM; break;
                    case 0b111: ins.type = InsType::REMU;   ins.alu_op = AluOp::REMU; break;
                    default:    ins.type = InsType::UNKNOWN; break;
                }
            } else {
                // Base RV32I
                switch (funct3) {
                    case 0b000:
                        if (funct7 & 0x20) {
                            ins.type = InsType::SUB;
                            ins.alu_op = AluOp::SUB;
                        } else {
                            ins.type = InsType::ADD;
                            ins.alu_op = AluOp::ADD;
                        }
                        break;
                    case 0b001: ins.type = InsType::SLL;  ins.alu_op = AluOp::SLL; break;
                    case 0b010: ins.type = InsType::SLT;  ins.alu_op = AluOp::SLT; break;
                    case 0b011: ins.type = InsType::SLTU; ins.alu_op = AluOp::SLTU; break;
                    case 0b100: ins.type = InsType::XOR;  ins.alu_op = AluOp::XOR; break;
                    case 0b101:
                        if (funct7 & 0x20) {
                            ins.type = InsType::SRA;
                            ins.alu_op = AluOp::SRA;
                        } else {
                            ins.type = InsType::SRL;
                            ins.alu_op = AluOp::SRL;
                        }
                        break;
                    case 0b110: ins.type = InsType::OR;  ins.alu_op = AluOp::OR; break;
                    case 0b111: ins.type = InsType::AND; ins.alu_op = AluOp::AND; break;
                    default:    ins.type = InsType::UNKNOWN; break;
                }
            }
            break;

        // =================================================================
        // System
        // =================================================================
        case OP_SYSTEM:
            ins.format = Format::I;
            ins.imm = imm_i(raw);
            if (ins.imm == 0) {
                ins.type = InsType::ECALL;
            } else if (ins.imm == 1) {
                ins.type = InsType::EBREAK;
            } else {
                ins.type = InsType::UNKNOWN;
            }
            break;

        default:
            ins.type = InsType::UNKNOWN;
            ins.format = Format::UNKNOWN;
            break;
    }

    ins.text = disassemble(ins);
    return ins;
}

// =============================================================================
// Debug Print
// =============================================================================

void Decoder::print(const Instruction& ins) {
    std::cout << "PC: " << to_hex(ins.pc)
              << "  Raw: " << to_hex(ins.raw)
              << "  " << ins.text << "\n";
    std::cout << "  rd=" << ins.rd << " rs1=" << ins.rs1
              << " rs2=" << ins.rs2 << " imm=" << ins.imm << "\n";
    std::cout << "  RegWr=" << ins.reg_write
              << " MemRd=" << ins.mem_read
              << " MemWr=" << ins.mem_write
              << " Branch=" << ins.branch
              << " Jump=" << ins.jump << "\n";
}
