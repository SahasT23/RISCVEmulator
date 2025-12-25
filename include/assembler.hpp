/**
 * assembler.hpp
 * 
 * Two-pass assembler for RISC-V assembly.
 * Handles labels, directives, pseudo-instructions.
 */

#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include "common.hpp"
#include "memory.hpp"

class Assembler {
public:
    // Result of assembly
    struct Result {
        bool success = false;
        std::vector<Word> text;                     // Machine code
        std::vector<Byte> data;                     // Data bytes
        Address text_addr = Memory::TEXT_BASE;
        Address data_addr = Memory::DATA_BASE;
        std::map<std::string, Address> symbols;     // Label -> address
        std::map<Address, std::string> source_map;  // PC -> source line
        std::vector<std::string> errors;
    };

    // Assemble from string
    Result assemble(const std::string& source);

    // Assemble from file
    Result assemble_file(const std::string& filename);

private:
    // State during assembly
    std::map<std::string, Address> labels;
    std::vector<std::string> errors;
    std::vector<Word> text_out;
    std::vector<Byte> data_out;
    std::map<Address, std::string> source_map;
    Address text_addr;
    Address data_addr;
    bool in_data;
    int line_num;

    // String helpers
    static std::string trim(const std::string& s);
    static std::string to_lower(const std::string& s);
    static std::vector<std::string> split(const std::string& s, char delim);

    // Parsing helpers
    int parse_reg(const std::string& s);
    bool parse_imm(const std::string& s, SignedWord& val);
    bool parse_mem(const std::string& s, SignedWord& offset, int& reg);

    // Encoding helpers
    Word enc_r(int op, int rd, int f3, int rs1, int rs2, int f7);
    Word enc_i(int op, int rd, int f3, int rs1, SignedWord imm);
    Word enc_s(int op, int f3, int rs1, int rs2, SignedWord imm);
    Word enc_b(int op, int f3, int rs1, int rs2, SignedWord imm);
    Word enc_u(int op, int rd, SignedWord imm);
    Word enc_j(int op, int rd, SignedWord imm);

    // Emit instruction
    void emit(Word w, const std::string& src);

    // Process a line
    void process_line(const std::string& line, bool first_pass);

    // Handle directives
    bool handle_directive(const std::string& line, bool first_pass);

    // Handle pseudo-instructions
    bool handle_pseudo(const std::string& mnem, std::vector<std::string>& ops,
                       const std::string& src, bool first_pass);

    // Handle real instructions
    bool handle_instruction(const std::string& mnem, std::vector<std::string>& ops,
                            const std::string& src, bool first_pass);

    // Error helper
    void error(const std::string& msg);
};

#endif // ASSEMBLER_HPP
