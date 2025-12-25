/**
 * assembler.cpp
 * 
 * Two-pass assembler implementation.
 * Pass 1: Collect labels
 * Pass 2: Generate machine code
 */

#include "assembler.hpp"
#include <algorithm>
#include <cctype>

// =============================================================================
// String Helpers
// =============================================================================

std::string Assembler::trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string Assembler::to_lower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

std::vector<std::string> Assembler::split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, delim)) {
        std::string t = trim(tok);
        if (!t.empty()) out.push_back(t);
    }
    return out;
}

// =============================================================================
// Parsing Helpers
// =============================================================================

int Assembler::parse_reg(const std::string& s) {
    std::string r = to_lower(trim(s));
    if (r.empty()) return -1;

    if (r[0] == 'x') {
        try {
            int n = std::stoi(r.substr(1));
            return (n >= 0 && n < 32) ? n : -1;
        } catch (...) { return -1; }
    }

    static const std::map<std::string, int> abi = {
        {"zero",0},{"ra",1},{"sp",2},{"gp",3},{"tp",4},
        {"t0",5},{"t1",6},{"t2",7},{"s0",8},{"fp",8},{"s1",9},
        {"a0",10},{"a1",11},{"a2",12},{"a3",13},{"a4",14},{"a5",15},
        {"a6",16},{"a7",17},{"s2",18},{"s3",19},{"s4",20},{"s5",21},
        {"s6",22},{"s7",23},{"s8",24},{"s9",25},{"s10",26},{"s11",27},
        {"t3",28},{"t4",29},{"t5",30},{"t6",31}
    };
    auto it = abi.find(r);
    return (it != abi.end()) ? it->second : -1;
}

bool Assembler::parse_imm(const std::string& s, SignedWord& val) {
    std::string t = trim(s);
    if (t.empty()) return false;
    try {
        if (t.size() > 2 && (t[1] == 'x' || t[1] == 'X')) {
            val = static_cast<SignedWord>(std::stoul(t, nullptr, 16));
        } else if (t.size() > 2 && (t[1] == 'b' || t[1] == 'B')) {
            val = static_cast<SignedWord>(std::stoul(t.substr(2), nullptr, 2));
        } else {
            val = std::stoi(t);
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool Assembler::parse_mem(const std::string& s, SignedWord& offset, int& reg) {
    size_t lp = s.find('(');
    size_t rp = s.find(')');
    if (lp == std::string::npos || rp == std::string::npos) return false;

    std::string off_str = trim(s.substr(0, lp));
    std::string reg_str = trim(s.substr(lp + 1, rp - lp - 1));

    offset = 0;
    if (!off_str.empty() && !parse_imm(off_str, offset)) return false;

    reg = parse_reg(reg_str);
    return reg >= 0;
}

// =============================================================================
// Encoding Helpers
// =============================================================================

Word Assembler::enc_r(int op, int rd, int f3, int rs1, int rs2, int f7) {
    return (f7 << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | op;
}

Word Assembler::enc_i(int op, int rd, int f3, int rs1, SignedWord imm) {
    return ((imm & 0xFFF) << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | op;
}

Word Assembler::enc_s(int op, int f3, int rs1, int rs2, SignedWord imm) {
    int hi = (imm >> 5) & 0x7F;
    int lo = imm & 0x1F;
    return (hi << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | (lo << 7) | op;
}

Word Assembler::enc_b(int op, int f3, int rs1, int rs2, SignedWord imm) {
    int b12 = (imm >> 12) & 1;
    int b11 = (imm >> 11) & 1;
    int b10_5 = (imm >> 5) & 0x3F;
    int b4_1 = (imm >> 1) & 0xF;
    return (b12 << 31) | (b10_5 << 25) | (rs2 << 20) | (rs1 << 15) |
           (f3 << 12) | (b4_1 << 8) | (b11 << 7) | op;
}

Word Assembler::enc_u(int op, int rd, SignedWord imm) {
    return (imm & 0xFFFFF000) | (rd << 7) | op;
}

Word Assembler::enc_j(int op, int rd, SignedWord imm) {
    int b20 = (imm >> 20) & 1;
    int b19_12 = (imm >> 12) & 0xFF;
    int b11 = (imm >> 11) & 1;
    int b10_1 = (imm >> 1) & 0x3FF;
    return (b20 << 31) | (b10_1 << 21) | (b11 << 20) | (b19_12 << 12) | (rd << 7) | op;
}

// =============================================================================
// Emit & Error
// =============================================================================

void Assembler::emit(Word w, const std::string& src) {
    source_map[text_addr] = src;
    text_out.push_back(w);
    text_addr += 4;
}

void Assembler::error(const std::string& msg) {
    errors.push_back("Line " + std::to_string(line_num) + ": " + msg);
}

// =============================================================================
// Directives
// =============================================================================

bool Assembler::handle_directive(const std::string& line, bool first_pass) {
    auto parts = split(line, ' ');
    if (parts.empty()) return true;

    std::string dir = to_lower(parts[0]);

    if (dir == ".text") {
        in_data = false;
        return true;
    }
    if (dir == ".data") {
        in_data = true;
        return true;
    }
    if (dir == ".globl" || dir == ".global") {
        return true;  // Ignore
    }

    if (dir == ".word") {
        for (size_t i = 1; i < parts.size(); i++) {
            SignedWord val;
            if (parse_imm(parts[i], val)) {
                if (!first_pass && in_data) {
                    for (int j = 0; j < 4; j++)
                        data_out.push_back((val >> (j * 8)) & 0xFF);
                }
                if (in_data) data_addr += 4;
            }
        }
        return true;
    }

    if (dir == ".half") {
        for (size_t i = 1; i < parts.size(); i++) {
            SignedWord val;
            if (parse_imm(parts[i], val)) {
                if (!first_pass && in_data) {
                    data_out.push_back(val & 0xFF);
                    data_out.push_back((val >> 8) & 0xFF);
                }
                if (in_data) data_addr += 2;
            }
        }
        return true;
    }

    if (dir == ".byte") {
        for (size_t i = 1; i < parts.size(); i++) {
            SignedWord val;
            if (parse_imm(parts[i], val)) {
                if (!first_pass && in_data) data_out.push_back(val & 0xFF);
                if (in_data) data_addr += 1;
            }
        }
        return true;
    }

    if (dir == ".asciz" || dir == ".string") {
        size_t q1 = line.find('"');
        size_t q2 = line.rfind('"');
        if (q1 != std::string::npos && q2 > q1) {
            std::string str = line.substr(q1 + 1, q2 - q1 - 1);
            size_t len = 0;
            for (size_t i = 0; i < str.size(); i++) {
                Byte c = str[i];
                if (str[i] == '\\' && i + 1 < str.size()) {
                    i++;
                    switch (str[i]) {
                        case 'n': c = '\n'; break;
                        case 't': c = '\t'; break;
                        case 'r': c = '\r'; break;
                        case '0': c = '\0'; break;
                        case '\\': c = '\\'; break;
                        case '"': c = '"'; break;
                        default: c = str[i]; break;
                    }
                }
                if (!first_pass && in_data) data_out.push_back(c);
                len++;
            }
            if (!first_pass && in_data) data_out.push_back(0);
            if (in_data) data_addr += len + 1;
        }
        return true;
    }

    if (dir == ".space") {
        if (parts.size() > 1) {
            SignedWord sz;
            if (parse_imm(parts[1], sz)) {
                if (!first_pass && in_data) {
                    for (int i = 0; i < sz; i++) data_out.push_back(0);
                }
                if (in_data) data_addr += sz;
            }
        }
        return true;
    }

    if (dir == ".align") {
        if (parts.size() > 1) {
            SignedWord p;
            if (parse_imm(parts[1], p)) {
                int align = 1 << p;
                if (in_data) {
                    while (data_addr % align != 0) {
                        if (!first_pass) data_out.push_back(0);
                        data_addr++;
                    }
                } else {
                    while (text_addr % align != 0) {
                        if (!first_pass) emit(0x00000013, "");  // NOP
                        else text_addr += 4;
                    }
                }
            }
        }
        return true;
    }

    return true;
}

// =============================================================================
// Pseudo-instructions
// =============================================================================

bool Assembler::handle_pseudo(const std::string& mnem, std::vector<std::string>& ops,
                               const std::string& src, bool first_pass) {
    // nop
    if (mnem == "nop") {
        if (!first_pass) emit(0x00000013, src);
        else text_addr += 4;
        return true;
    }

    // mv rd, rs -> addi rd, rs, 0
    if (mnem == "mv" && ops.size() == 2) {
        int rd = parse_reg(ops[0]);
        int rs = parse_reg(ops[1]);
        if (!first_pass) emit(enc_i(0b0010011, rd, 0, rs, 0), src);
        else text_addr += 4;
        return true;
    }

    // not rd, rs -> xori rd, rs, -1
    if (mnem == "not" && ops.size() == 2) {
        int rd = parse_reg(ops[0]);
        int rs = parse_reg(ops[1]);
        if (!first_pass) emit(enc_i(0b0010011, rd, 0b100, rs, -1), src);
        else text_addr += 4;
        return true;
    }

    // neg rd, rs -> sub rd, x0, rs
    if (mnem == "neg" && ops.size() == 2) {
        int rd = parse_reg(ops[0]);
        int rs = parse_reg(ops[1]);
        if (!first_pass) emit(enc_r(0b0110011, rd, 0, 0, rs, 0b0100000), src);
        else text_addr += 4;
        return true;
    }

    // seqz rd, rs -> sltiu rd, rs, 1
    if (mnem == "seqz" && ops.size() == 2) {
        int rd = parse_reg(ops[0]);
        int rs = parse_reg(ops[1]);
        if (!first_pass) emit(enc_i(0b0010011, rd, 0b011, rs, 1), src);
        else text_addr += 4;
        return true;
    }

    // snez rd, rs -> sltu rd, x0, rs
    if (mnem == "snez" && ops.size() == 2) {
        int rd = parse_reg(ops[0]);
        int rs = parse_reg(ops[1]);
        if (!first_pass) emit(enc_r(0b0110011, rd, 0b011, 0, rs, 0), src);
        else text_addr += 4;
        return true;
    }

    // sltz rd, rs -> slt rd, rs, x0
    if (mnem == "sltz" && ops.size() == 2) {
        int rd = parse_reg(ops[0]);
        int rs = parse_reg(ops[1]);
        if (!first_pass) emit(enc_r(0b0110011, rd, 0b010, rs, 0, 0), src);
        else text_addr += 4;
        return true;
    }

    // sgtz rd, rs -> slt rd, x0, rs
    if (mnem == "sgtz" && ops.size() == 2) {
        int rd = parse_reg(ops[0]);
        int rs = parse_reg(ops[1]);
        if (!first_pass) emit(enc_r(0b0110011, rd, 0b010, 0, rs, 0), src);
        else text_addr += 4;
        return true;
    }

    // li rd, imm
    if (mnem == "li" && ops.size() == 2) {
        int rd = parse_reg(ops[0]);
        SignedWord imm;
        if (!parse_imm(ops[1], imm)) {
            error("Invalid immediate");
            return true;
        }
        if (imm >= -2048 && imm < 2048) {
            if (!first_pass) emit(enc_i(0b0010011, rd, 0, 0, imm), src);
            else text_addr += 4;
        } else {
            Word upper = ((imm + 0x800) >> 12) & 0xFFFFF;
            SignedWord lower = imm - (upper << 12);
            if (!first_pass) {
                emit(enc_u(0b0110111, rd, upper << 12), src);
                if (lower != 0) emit(enc_i(0b0010011, rd, 0, rd, lower), src);
            } else {
                text_addr += 4;
                if (lower != 0) text_addr += 4;
            }
        }
        return true;
    }

    // la rd, label
    if (mnem == "la" && ops.size() == 2) {
        int rd = parse_reg(ops[0]);
        std::string label = trim(ops[1]);
        if (!first_pass) {
            auto it = labels.find(label);
            if (it == labels.end()) {
                error("Unknown label: " + label);
                return true;
            }
            SignedWord off = it->second - text_addr;
            Word upper = ((off + 0x800) >> 12) & 0xFFFFF;
            SignedWord lower = off - (upper << 12);
            emit(enc_u(0b0010111, rd, upper << 12), src);
            emit(enc_i(0b0010011, rd, 0, rd, lower), src);
        } else {
            text_addr += 8;
        }
        return true;
    }

    // j offset -> jal x0, offset
    if (mnem == "j" && ops.size() == 1) {
        if (!first_pass) {
            SignedWord off;
            if (parse_imm(ops[0], off)) {
                emit(enc_j(0b1101111, 0, off), src);
            } else {
                auto it = labels.find(ops[0]);
                if (it != labels.end()) {
                    off = it->second - text_addr;
                    emit(enc_j(0b1101111, 0, off), src);
                } else {
                    error("Unknown label: " + ops[0]);
                }
            }
        } else {
            text_addr += 4;
        }
        return true;
    }

    // jr rs -> jalr x0, rs, 0
    if (mnem == "jr" && ops.size() == 1) {
        int rs = parse_reg(ops[0]);
        if (!first_pass) emit(enc_i(0b1100111, 0, 0, rs, 0), src);
        else text_addr += 4;
        return true;
    }

    // ret -> jalr x0, ra, 0
    if (mnem == "ret") {
        if (!first_pass) emit(enc_i(0b1100111, 0, 0, 1, 0), src);
        else text_addr += 4;
        return true;
    }

    // call label -> jal ra, label
    if (mnem == "call" && ops.size() == 1) {
        if (!first_pass) {
            auto it = labels.find(ops[0]);
            if (it != labels.end()) {
                SignedWord off = it->second - text_addr;
                emit(enc_j(0b1101111, 1, off), src);
            } else {
                error("Unknown label: " + ops[0]);
            }
        } else {
            text_addr += 4;
        }
        return true;
    }

    // tail label -> jal x0, label
    if (mnem == "tail" && ops.size() == 1) {
        if (!first_pass) {
            auto it = labels.find(ops[0]);
            if (it != labels.end()) {
                SignedWord off = it->second - text_addr;
                emit(enc_j(0b1101111, 0, off), src);
            } else {
                error("Unknown label: " + ops[0]);
            }
        } else {
            text_addr += 4;
        }
        return true;
    }

    // beqz rs, label -> beq rs, x0, label
    if (mnem == "beqz" && ops.size() == 2) {
        int rs = parse_reg(ops[0]);
        if (!first_pass) {
            SignedWord off;
            if (parse_imm(ops[1], off)) {
                emit(enc_b(0b1100011, 0b000, rs, 0, off), src);
            } else {
                auto it = labels.find(ops[1]);
                if (it != labels.end()) {
                    off = it->second - text_addr;
                    emit(enc_b(0b1100011, 0b000, rs, 0, off), src);
                }
            }
        } else {
            text_addr += 4;
        }
        return true;
    }

    // bnez rs, label -> bne rs, x0, label
    if (mnem == "bnez" && ops.size() == 2) {
        int rs = parse_reg(ops[0]);
        if (!first_pass) {
            auto it = labels.find(ops[1]);
            if (it != labels.end()) {
                SignedWord off = it->second - text_addr;
                emit(enc_b(0b1100011, 0b001, rs, 0, off), src);
            }
        } else {
            text_addr += 4;
        }
        return true;
    }

    // blez rs, label -> bge x0, rs, label
    if (mnem == "blez" && ops.size() == 2) {
        int rs = parse_reg(ops[0]);
        if (!first_pass) {
            auto it = labels.find(ops[1]);
            if (it != labels.end()) {
                SignedWord off = it->second - text_addr;
                emit(enc_b(0b1100011, 0b101, 0, rs, off), src);
            }
        } else {
            text_addr += 4;
        }
        return true;
    }

    // bgez rs, label -> bge rs, x0, label
    if (mnem == "bgez" && ops.size() == 2) {
        int rs = parse_reg(ops[0]);
        if (!first_pass) {
            auto it = labels.find(ops[1]);
            if (it != labels.end()) {
                SignedWord off = it->second - text_addr;
                emit(enc_b(0b1100011, 0b101, rs, 0, off), src);
            }
        } else {
            text_addr += 4;
        }
        return true;
    }

    // bltz rs, label -> blt rs, x0, label
    if (mnem == "bltz" && ops.size() == 2) {
        int rs = parse_reg(ops[0]);
        if (!first_pass) {
            auto it = labels.find(ops[1]);
            if (it != labels.end()) {
                SignedWord off = it->second - text_addr;
                emit(enc_b(0b1100011, 0b100, rs, 0, off), src);
            }
        } else {
            text_addr += 4;
        }
        return true;
    }

    // bgtz rs, label -> blt x0, rs, label
    if (mnem == "bgtz" && ops.size() == 2) {
        int rs = parse_reg(ops[0]);
        if (!first_pass) {
            auto it = labels.find(ops[1]);
            if (it != labels.end()) {
                SignedWord off = it->second - text_addr;
                emit(enc_b(0b1100011, 0b100, 0, rs, off), src);
            }
        } else {
            text_addr += 4;
        }
        return true;
    }

    // bgt rs, rt, label -> blt rt, rs, label
    if (mnem == "bgt" && ops.size() == 3) {
        int rs = parse_reg(ops[0]);
        int rt = parse_reg(ops[1]);
        if (!first_pass) {
            auto it = labels.find(ops[2]);
            if (it != labels.end()) {
                SignedWord off = it->second - text_addr;
                emit(enc_b(0b1100011, 0b100, rt, rs, off), src);
            }
        } else {
            text_addr += 4;
        }
        return true;
    }

    // ble rs, rt, label -> bge rt, rs, label
    if (mnem == "ble" && ops.size() == 3) {
        int rs = parse_reg(ops[0]);
        int rt = parse_reg(ops[1]);
        if (!first_pass) {
            auto it = labels.find(ops[2]);
            if (it != labels.end()) {
                SignedWord off = it->second - text_addr;
                emit(enc_b(0b1100011, 0b101, rt, rs, off), src);
            }
        } else {
            text_addr += 4;
        }
        return true;
    }

    // bgtu rs, rt, label -> bltu rt, rs, label
    if (mnem == "bgtu" && ops.size() == 3) {
        int rs = parse_reg(ops[0]);
        int rt = parse_reg(ops[1]);
        if (!first_pass) {
            auto it = labels.find(ops[2]);
            if (it != labels.end()) {
                SignedWord off = it->second - text_addr;
                emit(enc_b(0b1100011, 0b110, rt, rs, off), src);
            }
        } else {
            text_addr += 4;
        }
        return true;
    }

    // bleu rs, rt, label -> bgeu rt, rs, label
    if (mnem == "bleu" && ops.size() == 3) {
        int rs = parse_reg(ops[0]);
        int rt = parse_reg(ops[1]);
        if (!first_pass) {
            auto it = labels.find(ops[2]);
            if (it != labels.end()) {
                SignedWord off = it->second - text_addr;
                emit(enc_b(0b1100011, 0b111, rt, rs, off), src);
            }
        } else {
            text_addr += 4;
        }
        return true;
    }

    return false;  // Not a pseudo-instruction
}

// =============================================================================
// Real Instructions
// =============================================================================

bool Assembler::handle_instruction(const std::string& mnem, std::vector<std::string>& ops,
                                    const std::string& src, bool first_pass) {
    // R-type
    static const std::map<std::string, std::tuple<int,int>> r_ops = {
        {"add",{0b000,0b0000000}}, {"sub",{0b000,0b0100000}},
        {"sll",{0b001,0b0000000}}, {"slt",{0b010,0b0000000}},
        {"sltu",{0b011,0b0000000}}, {"xor",{0b100,0b0000000}},
        {"srl",{0b101,0b0000000}}, {"sra",{0b101,0b0100000}},
        {"or",{0b110,0b0000000}}, {"and",{0b111,0b0000000}},
        {"mul",{0b000,0b0000001}}, {"mulh",{0b001,0b0000001}},
        {"mulhsu",{0b010,0b0000001}}, {"mulhu",{0b011,0b0000001}},
        {"div",{0b100,0b0000001}}, {"divu",{0b101,0b0000001}},
        {"rem",{0b110,0b0000001}}, {"remu",{0b111,0b0000001}}
    };

    auto r_it = r_ops.find(mnem);
    if (r_it != r_ops.end() && ops.size() == 3) {
        int rd = parse_reg(ops[0]);
        int rs1 = parse_reg(ops[1]);
        int rs2 = parse_reg(ops[2]);
        auto [f3, f7] = r_it->second;
        if (!first_pass) emit(enc_r(0b0110011, rd, f3, rs1, rs2, f7), src);
        else text_addr += 4;
        return true;
    }

    // I-type arithmetic
    static const std::map<std::string, int> i_arith = {
        {"addi",0b000}, {"slti",0b010}, {"sltiu",0b011},
        {"xori",0b100}, {"ori",0b110}, {"andi",0b111}
    };

    auto i_it = i_arith.find(mnem);
    if (i_it != i_arith.end() && ops.size() == 3) {
        int rd = parse_reg(ops[0]);
        int rs1 = parse_reg(ops[1]);
        SignedWord imm;
        parse_imm(ops[2], imm);
        if (!first_pass) emit(enc_i(0b0010011, rd, i_it->second, rs1, imm), src);
        else text_addr += 4;
        return true;
    }

    // I-type shifts
    if ((mnem == "slli" || mnem == "srli" || mnem == "srai") && ops.size() == 3) {
        int rd = parse_reg(ops[0]);
        int rs1 = parse_reg(ops[1]);
        SignedWord shamt;
        parse_imm(ops[2], shamt);
        int f3 = (mnem == "slli") ? 0b001 : 0b101;
        int f7 = (mnem == "srai") ? 0b0100000 : 0b0000000;
        Word ins = (f7 << 25) | ((shamt & 0x1F) << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | 0b0010011;
        if (!first_pass) emit(ins, src);
        else text_addr += 4;
        return true;
    }

    // Loads
    static const std::map<std::string, int> loads = {
        {"lb",0b000}, {"lh",0b001}, {"lw",0b010}, {"lbu",0b100}, {"lhu",0b101}
    };

    auto ld_it = loads.find(mnem);
    if (ld_it != loads.end() && ops.size() == 2) {
        int rd = parse_reg(ops[0]);
        SignedWord off; int rs1;
        parse_mem(ops[1], off, rs1);
        if (!first_pass) emit(enc_i(0b0000011, rd, ld_it->second, rs1, off), src);
        else text_addr += 4;
        return true;
    }

    // Stores
    static const std::map<std::string, int> stores = {
        {"sb",0b000}, {"sh",0b001}, {"sw",0b010}
    };

    auto st_it = stores.find(mnem);
    if (st_it != stores.end() && ops.size() == 2) {
        int rs2 = parse_reg(ops[0]);
        SignedWord off; int rs1;
        parse_mem(ops[1], off, rs1);
        if (!first_pass) emit(enc_s(0b0100011, st_it->second, rs1, rs2, off), src);
        else text_addr += 4;
        return true;
    }

    // Branches
    static const std::map<std::string, int> branches = {
        {"beq",0b000}, {"bne",0b001}, {"blt",0b100},
        {"bge",0b101}, {"bltu",0b110}, {"bgeu",0b111}
    };

    auto br_it = branches.find(mnem);
    if (br_it != branches.end() && ops.size() == 3) {
        int rs1 = parse_reg(ops[0]);
        int rs2 = parse_reg(ops[1]);
        if (!first_pass) {
            SignedWord off;
            if (parse_imm(ops[2], off)) {
                emit(enc_b(0b1100011, br_it->second, rs1, rs2, off), src);
            } else {
                auto it = labels.find(ops[2]);
                if (it != labels.end()) {
                    off = it->second - text_addr;
                    emit(enc_b(0b1100011, br_it->second, rs1, rs2, off), src);
                } else {
                    error("Unknown label: " + ops[2]);
                }
            }
        } else {
            text_addr += 4;
        }
        return true;
    }

    // JAL
    if (mnem == "jal") {
        int rd = 1;
        std::string target;
        if (ops.size() == 1) {
            target = ops[0];
        } else if (ops.size() == 2) {
            rd = parse_reg(ops[0]);
            target = ops[1];
        } else {
            error("Invalid jal format");
            return true;
        }
        if (!first_pass) {
            SignedWord off;
            if (parse_imm(target, off)) {
                emit(enc_j(0b1101111, rd, off), src);
            } else {
                auto it = labels.find(target);
                if (it != labels.end()) {
                    off = it->second - text_addr;
                    emit(enc_j(0b1101111, rd, off), src);
                } else {
                    error("Unknown label: " + target);
                }
            }
        } else {
            text_addr += 4;
        }
        return true;
    }

    // JALR
    if (mnem == "jalr") {
        int rd = 1, rs1 = 0;
        SignedWord off = 0;
        if (ops.size() == 1) {
            rs1 = parse_reg(ops[0]);
            rd = 1;
        } else if (ops.size() == 2) {
            rd = parse_reg(ops[0]);
            parse_mem(ops[1], off, rs1);
        } else if (ops.size() == 3) {
            rd = parse_reg(ops[0]);
            rs1 = parse_reg(ops[1]);
            parse_imm(ops[2], off);
        }
        if (!first_pass) emit(enc_i(0b1100111, rd, 0, rs1, off), src);
        else text_addr += 4;
        return true;
    }

    // LUI
    if (mnem == "lui" && ops.size() == 2) {
        int rd = parse_reg(ops[0]);
        SignedWord imm;
        parse_imm(ops[1], imm);
        if (!first_pass) emit(enc_u(0b0110111, rd, imm << 12), src);
        else text_addr += 4;
        return true;
    }

    // AUIPC
    if (mnem == "auipc" && ops.size() == 2) {
        int rd = parse_reg(ops[0]);
        SignedWord imm;
        parse_imm(ops[1], imm);
        if (!first_pass) emit(enc_u(0b0010111, rd, imm << 12), src);
        else text_addr += 4;
        return true;
    }

    // ECALL
    if (mnem == "ecall") {
        if (!first_pass) emit(0x00000073, src);
        else text_addr += 4;
        return true;
    }

    // EBREAK
    if (mnem == "ebreak") {
        if (!first_pass) emit(0x00100073, src);
        else text_addr += 4;
        return true;
    }

    error("Unknown instruction: " + mnem);
    return true;
}

// =============================================================================
// Process Line
// =============================================================================

void Assembler::process_line(const std::string& orig, bool first_pass) {
    std::string line = trim(orig);

    // Remove comments
    size_t hash = line.find('#');
    if (hash != std::string::npos) line = trim(line.substr(0, hash));
    if (line.empty()) return;

    // Check for label
    size_t colon = line.find(':');
    if (colon != std::string::npos) {
        std::string label = trim(line.substr(0, colon));
        if (first_pass) {
            labels[label] = in_data ? data_addr : text_addr;
        }
        line = trim(line.substr(colon + 1));
        if (line.empty()) return;
    }

    // Directive?
    if (line[0] == '.') {
        handle_directive(line, first_pass);
        return;
    }

    // Instruction
    if (!in_data) {
        std::string mnem;
        std::string rest;
        size_t sp = line.find_first_of(" \t");
        if (sp != std::string::npos) {
            mnem = to_lower(trim(line.substr(0, sp)));
            rest = trim(line.substr(sp));
        } else {
            mnem = to_lower(line);
        }

        auto ops = split(rest, ',');

        if (!handle_pseudo(mnem, ops, orig, first_pass)) {
            handle_instruction(mnem, ops, orig, first_pass);
        }
    }
}

// =============================================================================
// Main Assemble
// =============================================================================

Assembler::Result Assembler::assemble(const std::string& source) {
    Result res;
    res.text_addr = Memory::TEXT_BASE;
    res.data_addr = Memory::DATA_BASE;

    // Reset state
    labels.clear();
    errors.clear();
    text_out.clear();
    data_out.clear();
    source_map.clear();

    // Split into lines
    std::vector<std::string> lines;
    std::istringstream ss(source);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);

    // Pass 1: collect labels
    text_addr = Memory::TEXT_BASE;
    data_addr = Memory::DATA_BASE;
    in_data = false;
    line_num = 0;
    for (const auto& l : lines) {
        line_num++;
        process_line(l, true);
    }

    // Pass 2: generate code
    text_addr = Memory::TEXT_BASE;
    data_addr = Memory::DATA_BASE;
    in_data = false;
    line_num = 0;
    for (const auto& l : lines) {
        line_num++;
        process_line(l, false);
    }

    res.success = errors.empty();
    res.text = text_out;
    res.data = data_out;
    res.symbols = labels;
    res.source_map = source_map;
    res.errors = errors;
    return res;
}

Assembler::Result Assembler::assemble_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        Result res;
        res.success = false;
        res.errors.push_back("Cannot open file: " + filename);
        return res;
    }
    std::stringstream buf;
    buf << file.rdbuf();
    return assemble(buf.str());
}
