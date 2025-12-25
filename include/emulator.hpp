/**
 * emulator.hpp
 * 
 * Top-level emulator controller.
 * Ties together all components and handles user commands.
 */

#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include "common.hpp"
#include "memory.hpp"
#include "register_file.hpp"
#include "assembler.hpp"
#include "cpu.hpp"
#include "pipeline.hpp"

class Emulator {
public:
    enum class Mode {
        SINGLE_CYCLE,
        PIPELINE
    };

    Emulator();

    // Load program from file
    bool load(const std::string& filename);

    // Load program from string
    bool load_source(const std::string& source);

    // Run the command loop
    void run();

    // Execute a single command, returns false to quit
    bool execute_command(const std::string& input);

private:
    Memory mem;
    RegisterFile regs;
    CPU cpu;
    Pipeline pipeline;
    Assembler assembler;
    Assembler::Result asm_result;

    Mode mode;
    bool running;
    bool program_loaded;

    // Command handlers
    void cmd_help();
    void cmd_load(const std::string& filename);
    void cmd_run();
    void cmd_step(int count);
    void cmd_reset();
    void cmd_regs();
    void cmd_reg(const std::string& reg_name);
    void cmd_mem(Address addr, int count);
    void cmd_pc();
    void cmd_set_pc(Address addr);
    void cmd_mode(const std::string& mode_str);
    void cmd_hazards(const std::string& state);
    void cmd_forward(const std::string& state);
    void cmd_break(const std::string& target);
    void cmd_breakpoints();
    void cmd_clear();
    void cmd_symbols();
    void cmd_disasm(Address addr, int count);
    void cmd_pipeline();
    void cmd_stats();

    // Helpers
    void print_welcome();
    void print_prompt();
    void print_instruction(Address pc);
    Address resolve_address(const std::string& str);
    std::vector<std::string> tokenize(const std::string& input);
};

#endif // EMULATOR_HPP
