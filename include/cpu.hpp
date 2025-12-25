/**
 * cpu.hpp
 * 
 * Single-cycle CPU implementation.
 * Executes one instruction per cycle: fetch, decode, execute, memory, writeback.
 */

#ifndef CPU_HPP
#define CPU_HPP

#include "common.hpp"
#include "memory.hpp"
#include "register_file.hpp"
#include "alu.hpp"
#include "decoder.hpp"

class CPU {
public:
    CPU(Memory& mem, RegisterFile& regs);
    void reset();

    // Execute one instruction, returns false if halted
    bool step();

    // Run until halt or breakpoint
    void run();

    // State access
    Address get_pc() const;
    void set_pc(Address addr);
    uint64_t get_cycle_count() const;
    uint64_t get_instruction_count() const;
    bool is_halted() const;

    // Last executed instruction (for display)
    const Instruction& get_last_instruction() const;

    // Breakpoints
    void add_breakpoint(Address addr);
    void remove_breakpoint(Address addr);
    void clear_breakpoints();
    bool has_breakpoint(Address addr) const;

private:
    Memory& mem;
    RegisterFile& regs;
    Address pc;
    uint64_t cycles;
    uint64_t instructions;
    bool halted;
    Instruction last_ins;
    std::vector<Address> breakpoints;

    // Pipeline stages (all in one cycle for single-cycle)
    Word fetch();
    Instruction decode(Word raw);
    Word execute(const Instruction& ins, Word rs1_val, Word rs2_val);
    Word memory_access(const Instruction& ins, Word alu_result, Word rs2_val);
    void writeback(const Instruction& ins, Word result);
};

#endif // CPU_HPP
