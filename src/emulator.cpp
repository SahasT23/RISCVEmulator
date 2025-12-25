/**
 * emulator.cpp
 * 
 * Top-level emulator implementation.
 */

#include "emulator.hpp"
#include "decoder.hpp"
#include <algorithm>
#include <sstream>

Emulator::Emulator()
    : cpu(mem, regs), pipeline(mem, regs),
      mode(Mode::SINGLE_CYCLE), running(true), program_loaded(false) {}

// =============================================================================
// Program Loading
// =============================================================================

bool Emulator::load(const std::string& filename) {
    asm_result = assembler.assemble_file(filename);
    
    if (!asm_result.success) {
        std::cout << "Assembly failed:\n";
        for (const auto& err : asm_result.errors) {
            std::cout << "  " << err << "\n";
        }
        return false;
    }

    // Reset state
    mem.reset();
    regs.reset();
    cpu.reset();
    pipeline.reset();

    // Load text segment
    mem.write_block(asm_result.text_addr, asm_result.text);

    // Load data segment
    mem.write_bytes(asm_result.data_addr, asm_result.data);

    // Initialize stack pointer
    regs.write(2, Memory::STACK_TOP);

    program_loaded = true;
    std::cout << "Loaded " << asm_result.text.size() << " instructions, "
              << asm_result.data.size() << " bytes data\n";
    std::cout << "Entry point: " << to_hex(asm_result.text_addr) << "\n";

    return true;
}

bool Emulator::load_source(const std::string& source) {
    asm_result = assembler.assemble(source);

    if (!asm_result.success) {
        std::cout << "Assembly failed:\n";
        for (const auto& err : asm_result.errors) {
            std::cout << "  " << err << "\n";
        }
        return false;
    }

    mem.reset();
    regs.reset();
    cpu.reset();
    pipeline.reset();

    mem.write_block(asm_result.text_addr, asm_result.text);
    mem.write_bytes(asm_result.data_addr, asm_result.data);
    regs.write(2, Memory::STACK_TOP);

    program_loaded = true;
    return true;
}

// =============================================================================
// Command Loop
// =============================================================================

void Emulator::run() {
    print_welcome();

    std::string input;
    while (running) {
        print_prompt();
        if (!std::getline(std::cin, input)) break;
        if (!execute_command(input)) break;
    }

    std::cout << "Goodbye!\n";
}

bool Emulator::execute_command(const std::string& input) {
    auto tokens = tokenize(input);
    if (tokens.empty()) return true;

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    if (cmd == "quit" || cmd == "exit" || cmd == "q") {
        running = false;
        return false;
    }
    else if (cmd == "help" || cmd == "h" || cmd == "?") {
        cmd_help();
    }
    else if (cmd == "load" || cmd == "l") {
        if (tokens.size() < 2) {
            std::cout << "Usage: load <filename>\n";
        } else {
            cmd_load(tokens[1]);
        }
    }
    else if (cmd == "run" || cmd == "r") {
        cmd_run();
    }
    else if (cmd == "step" || cmd == "s") {
        int count = 1;
        if (tokens.size() > 1) {
            try { count = std::stoi(tokens[1]); } catch (...) {}
        }
        cmd_step(count);
    }
    else if (cmd == "reset") {
        cmd_reset();
    }
    else if (cmd == "regs" || cmd == "registers") {
        cmd_regs();
    }
    else if (cmd == "reg") {
        if (tokens.size() < 2) {
            std::cout << "Usage: reg <register>\n";
        } else {
            cmd_reg(tokens[1]);
        }
    }
    else if (cmd == "mem" || cmd == "memory" || cmd == "m") {
        if (tokens.size() < 2) {
            std::cout << "Usage: mem <address> [count]\n";
        } else {
            Address addr = resolve_address(tokens[1]);
            int count = 64;
            if (tokens.size() > 2) {
                try { count = std::stoi(tokens[2]); } catch (...) {}
            }
            cmd_mem(addr, count);
        }
    }
    else if (cmd == "pc") {
        if (tokens.size() > 1) {
            Address addr = resolve_address(tokens[1]);
            cmd_set_pc(addr);
        } else {
            cmd_pc();
        }
    }
    else if (cmd == "mode") {
        if (tokens.size() < 2) {
            std::cout << "Current mode: " << (mode == Mode::SINGLE_CYCLE ? "single" : "pipeline") << "\n";
        } else {
            cmd_mode(tokens[1]);
        }
    }
    else if (cmd == "hazards") {
        if (tokens.size() < 2) {
            std::cout << "Hazard detection: " << (pipeline.get_hazard_detection() ? "on" : "off") << "\n";
        } else {
            cmd_hazards(tokens[1]);
        }
    }
    else if (cmd == "forward" || cmd == "forwarding") {
        if (tokens.size() < 2) {
            std::cout << "Forwarding: " << (pipeline.get_forwarding() ? "on" : "off") << "\n";
        } else {
            cmd_forward(tokens[1]);
        }
    }
    else if (cmd == "break" || cmd == "b") {
        if (tokens.size() < 2) {
            cmd_breakpoints();
        } else {
            cmd_break(tokens[1]);
        }
    }
    else if (cmd == "clear") {
        cmd_clear();
    }
    else if (cmd == "symbols" || cmd == "sym") {
        cmd_symbols();
    }
    else if (cmd == "disasm" || cmd == "d") {
        Address addr = (mode == Mode::SINGLE_CYCLE) ? cpu.get_pc() : pipeline.get_pc();
        int count = 10;
        if (tokens.size() > 1) addr = resolve_address(tokens[1]);
        if (tokens.size() > 2) {
            try { count = std::stoi(tokens[2]); } catch (...) {}
        }
        cmd_disasm(addr, count);
    }
    else if (cmd == "pipeline" || cmd == "pipe" || cmd == "p") {
        cmd_pipeline();
    }
    else if (cmd == "stats") {
        cmd_stats();
    }
    else {
        std::cout << "Unknown command: " << cmd << ". Type 'help' for commands.\n";
    }

    return true;
}

// =============================================================================
// Command Implementations
// =============================================================================

void Emulator::cmd_help() {
    std::cout << "Commands:\n"
              << "  load <file>       Load assembly file\n"
              << "  run               Run until halt or breakpoint\n"
              << "  step [n]          Execute n instructions (default 1)\n"
              << "  reset             Reset CPU state\n"
              << "  regs              Show all registers\n"
              << "  reg <name>        Show single register\n"
              << "  mem <addr> [n]    Show n bytes of memory\n"
              << "  pc [addr]         Show or set PC\n"
              << "  mode <s|p>        Set single-cycle or pipeline mode\n"
              << "  hazards <on|off>  Toggle hazard detection\n"
              << "  forward <on|off>  Toggle forwarding\n"
              << "  break <addr>      Set breakpoint\n"
              << "  clear             Clear all breakpoints\n"
              << "  symbols           Show symbol table\n"
              << "  disasm [addr] [n] Disassemble instructions\n"
              << "  pipeline          Show pipeline state\n"
              << "  stats             Show statistics\n"
              << "  quit              Exit emulator\n";
}

void Emulator::cmd_load(const std::string& filename) {
    load(filename);
}

void Emulator::cmd_run() {
    if (!program_loaded) {
        std::cout << "No program loaded\n";
        return;
    }

    if (mode == Mode::SINGLE_CYCLE) {
        cpu.run();
        std::cout << "Halted at PC=" << to_hex(cpu.get_pc()) << "\n";
        print_instruction(cpu.get_pc());
    } else {
        pipeline.run();
        std::cout << "Halted at PC=" << to_hex(pipeline.get_pc()) << "\n";
    }
}

void Emulator::cmd_step(int count) {
    if (!program_loaded) {
        std::cout << "No program loaded\n";
        return;
    }

    for (int i = 0; i < count; i++) {
        bool cont;
        if (mode == Mode::SINGLE_CYCLE) {
            cont = cpu.step();
            print_instruction(cpu.get_last_instruction().pc);
        } else {
            cont = pipeline.cycle();
            pipeline.print_state();
        }

        if (!cont) {
            if (mode == Mode::SINGLE_CYCLE && cpu.is_halted()) {
                std::cout << "Program halted\n";
            } else if (mode == Mode::PIPELINE && pipeline.is_halted()) {
                std::cout << "Program halted\n";
            } else {
                std::cout << "Breakpoint hit\n";
            }
            break;
        }
    }
}

void Emulator::cmd_reset() {
    if (!program_loaded) {
        std::cout << "No program loaded\n";
        return;
    }

    // Reload the program
    mem.reset();
    regs.reset();
    cpu.reset();
    pipeline.reset();

    mem.write_block(asm_result.text_addr, asm_result.text);
    mem.write_bytes(asm_result.data_addr, asm_result.data);
    regs.write(2, Memory::STACK_TOP);

    std::cout << "Reset complete\n";
}

void Emulator::cmd_regs() {
    regs.dump();
}

void Emulator::cmd_reg(const std::string& name) {
    std::string r = name;
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);

    int reg = -1;
    if (r[0] == 'x') {
        try { reg = std::stoi(r.substr(1)); } catch (...) {}
    } else {
        static const std::map<std::string, int> abi = {
            {"zero",0},{"ra",1},{"sp",2},{"gp",3},{"tp",4},
            {"t0",5},{"t1",6},{"t2",7},{"s0",8},{"fp",8},{"s1",9},
            {"a0",10},{"a1",11},{"a2",12},{"a3",13},{"a4",14},{"a5",15},
            {"a6",16},{"a7",17},{"s2",18},{"s3",19},{"s4",20},{"s5",21},
            {"s6",22},{"s7",23},{"s8",24},{"s9",25},{"s10",26},{"s11",27},
            {"t3",28},{"t4",29},{"t5",30},{"t6",31}
        };
        auto it = abi.find(r);
        if (it != abi.end()) reg = it->second;
    }

    if (reg >= 0 && reg < 32) {
        regs.dump_reg(reg);
    } else {
        std::cout << "Unknown register: " << name << "\n";
    }
}

void Emulator::cmd_mem(Address addr, int count) {
    mem.dump(addr, count);
}

void Emulator::cmd_pc() {
    Address pc = (mode == Mode::SINGLE_CYCLE) ? cpu.get_pc() : pipeline.get_pc();
    std::cout << "PC = " << to_hex(pc) << "\n";
    print_instruction(pc);
}

void Emulator::cmd_set_pc(Address addr) {
    if (mode == Mode::SINGLE_CYCLE) {
        cpu.set_pc(addr);
    } else {
        pipeline.set_pc(addr);
    }
    std::cout << "PC set to " << to_hex(addr) << "\n";
}

void Emulator::cmd_mode(const std::string& mode_str) {
    std::string m = mode_str;
    std::transform(m.begin(), m.end(), m.begin(), ::tolower);

    if (m == "single" || m == "s") {
        mode = Mode::SINGLE_CYCLE;
        std::cout << "Mode: single-cycle\n";
    } else if (m == "pipeline" || m == "pipe" || m == "p") {
        mode = Mode::PIPELINE;
        std::cout << "Mode: pipeline\n";
    } else {
        std::cout << "Unknown mode. Use 'single' or 'pipeline'\n";
    }
}

void Emulator::cmd_hazards(const std::string& state) {
    std::string s = state;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    if (s == "on" || s == "1" || s == "true") {
        pipeline.set_hazard_detection(true);
        std::cout << "Hazard detection: on\n";
    } else if (s == "off" || s == "0" || s == "false") {
        pipeline.set_hazard_detection(false);
        std::cout << "Hazard detection: off\n";
    } else {
        std::cout << "Use 'on' or 'off'\n";
    }
}

void Emulator::cmd_forward(const std::string& state) {
    std::string s = state;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    if (s == "on" || s == "1" || s == "true") {
        pipeline.set_forwarding(true);
        std::cout << "Forwarding: on\n";
    } else if (s == "off" || s == "0" || s == "false") {
        pipeline.set_forwarding(false);
        std::cout << "Forwarding: off\n";
    } else {
        std::cout << "Use 'on' or 'off'\n";
    }
}

void Emulator::cmd_break(const std::string& target) {
    Address addr = resolve_address(target);

    if (mode == Mode::SINGLE_CYCLE) {
        cpu.add_breakpoint(addr);
    } else {
        pipeline.add_breakpoint(addr);
    }
    std::cout << "Breakpoint set at " << to_hex(addr) << "\n";
}

void Emulator::cmd_breakpoints() {
    std::cout << "Breakpoints: (use 'break <addr>' to add, 'clear' to remove all)\n";
    // Note: Would need to expose breakpoint list from CPU/Pipeline for full implementation
}

void Emulator::cmd_clear() {
    cpu.clear_breakpoints();
    pipeline.clear_breakpoints();
    std::cout << "All breakpoints cleared\n";
}

void Emulator::cmd_symbols() {
    if (!program_loaded) {
        std::cout << "No program loaded\n";
        return;
    }

    std::cout << "Symbols:\n";
    for (const auto& [name, addr] : asm_result.symbols) {
        std::cout << "  " << to_hex(addr) << "  " << name << "\n";
    }
}

void Emulator::cmd_disasm(Address addr, int count) {
    std::cout << "Disassembly:\n";
    for (int i = 0; i < count; i++) {
        Address pc = addr + i * 4;
        Word raw = mem.read_word(pc);
        if (raw == 0) continue;

        Instruction ins = Decoder::decode(raw, pc);

        // Check if there's source for this address
        std::string src;
        auto it = asm_result.source_map.find(pc);
        if (it != asm_result.source_map.end()) {
            src = "  ; " + it->second;
        }

        std::cout << "  " << to_hex(pc) << ": " << to_hex(raw) << "  "
                  << std::left << std::setw(20) << ins.text << src << "\n";
    }
    std::cout << std::right;
}

void Emulator::cmd_pipeline() {
    if (mode != Mode::PIPELINE) {
        std::cout << "Pipeline view only available in pipeline mode\n";
        return;
    }
    pipeline.print_state();
}

void Emulator::cmd_stats() {
    std::cout << "Statistics:\n";

    if (mode == Mode::SINGLE_CYCLE) {
        std::cout << "  Mode: single-cycle\n";
        std::cout << "  Cycles: " << cpu.get_cycle_count() << "\n";
        std::cout << "  Instructions: " << cpu.get_instruction_count() << "\n";
        std::cout << "  CPI: 1.0\n";
    } else {
        std::cout << "  Mode: pipeline\n";
        std::cout << "  Cycles: " << pipeline.get_cycle_count() << "\n";
        std::cout << "  Instructions: " << pipeline.get_instruction_count() << "\n";
        
        uint64_t ins = pipeline.get_instruction_count();
        if (ins > 0) {
            double cpi = static_cast<double>(pipeline.get_cycle_count()) / ins;
            std::cout << "  CPI: " << std::fixed << std::setprecision(2) << cpi << "\n";
        }

        std::cout << "  Stalls: " << pipeline.get_stall_count() << "\n";
        std::cout << "  Flushes: " << pipeline.get_flush_count() << "\n";
        std::cout << "  Forwards: " << pipeline.get_forward_count() << "\n";
        std::cout << "  Hazard detection: " << (pipeline.get_hazard_detection() ? "on" : "off") << "\n";
        std::cout << "  Forwarding: " << (pipeline.get_forwarding() ? "on" : "off") << "\n";
    }

    std::cout << "  Memory reads: " << mem.get_read_count() << "\n";
    std::cout << "  Memory writes: " << mem.get_write_count() << "\n";
}

// =============================================================================
// Helpers
// =============================================================================

void Emulator::print_welcome() {
    std::cout << "\n";
    std::cout << "RISC-V Emulator (RV32IM)\n";
    std::cout << "Type 'help' for commands\n";
    std::cout << "\n";
}

void Emulator::print_prompt() {
    std::string mode_str = (mode == Mode::SINGLE_CYCLE) ? "single" : "pipe";
    Address pc = (mode == Mode::SINGLE_CYCLE) ? cpu.get_pc() : pipeline.get_pc();
    std::cout << "[" << mode_str << " " << to_hex(pc) << "] > ";
}

void Emulator::print_instruction(Address pc) {
    Word raw = mem.read_word(pc);
    Instruction ins = Decoder::decode(raw, pc);
    std::cout << to_hex(pc) << ": " << ins.text << "\n";
}

Address Emulator::resolve_address(const std::string& str) {
    // Try as symbol first
    auto it = asm_result.symbols.find(str);
    if (it != asm_result.symbols.end()) {
        return it->second;
    }

    // Try as hex or decimal
    try {
        if (str.size() > 2 && (str[1] == 'x' || str[1] == 'X')) {
            return static_cast<Address>(std::stoul(str, nullptr, 16));
        }
        return static_cast<Address>(std::stoul(str));
    } catch (...) {
        std::cout << "Invalid address: " << str << "\n";
        return 0;
    }
}

std::vector<std::string> Emulator::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream ss(input);
    std::string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}
