/**
 * cpu.cpp
 * 
 * Single-cycle CPU implementation.
 * All stages complete in one cycle.
 */

#include "cpu.hpp"
#include <algorithm>

CPU::CPU(Memory& mem, RegisterFile& regs)
    : mem(mem), regs(regs), pc(Memory::TEXT_BASE),
      cycles(0), instructions(0), halted(false) {}

void CPU::reset() {
    pc = Memory::TEXT_BASE;
    cycles = 0;
    instructions = 0;
    halted = false;
    regs.reset();
}

// =============================================================================
// Fetch
// =============================================================================

Word CPU::fetch() {
    return mem.read_word(pc);
}

// =============================================================================
// Decode
// =============================================================================

Instruction CPU::decode(Word raw) {
    return Decoder::decode(raw, pc);
}

// =============================================================================
// Execute
// =============================================================================

Word CPU::execute(const Instruction& ins, Word rs1_val, Word rs2_val) {
    Word alu_a = rs1_val;
    Word alu_b = ins.alu_src ? static_cast<Word>(ins.imm) : rs2_val;

    // Special case: AUIPC uses PC as operand A
    if (ins.type == InsType::AUIPC) {
        alu_a = pc;
    }

    return ALU::execute(ins.alu_op, alu_a, alu_b);
}

// =============================================================================
// Memory Access
// =============================================================================

Word CPU::memory_access(const Instruction& ins, Word alu_result, Word rs2_val) {
    Address addr = alu_result;

    if (ins.mem_read) {
        switch (ins.type) {
            case InsType::LB:  return static_cast<Word>(mem.read_byte_signed(addr));
            case InsType::LH:  return static_cast<Word>(mem.read_half_signed(addr));
            case InsType::LW:  return mem.read_word(addr);
            case InsType::LBU: return mem.read_byte(addr);
            case InsType::LHU: return mem.read_half(addr);
            default: break;
        }
    }

    if (ins.mem_write) {
        switch (ins.type) {
            case InsType::SB: mem.write_byte(addr, rs2_val & 0xFF); break;
            case InsType::SH: mem.write_half(addr, rs2_val & 0xFFFF); break;
            case InsType::SW: mem.write_word(addr, rs2_val); break;
            default: break;
        }
    }

    return alu_result;
}

// =============================================================================
// Writeback
// =============================================================================

void CPU::writeback(const Instruction& ins, Word result) {
    if (ins.reg_write && ins.rd != 0) {
        regs.write(ins.rd, result);
    }
}

// =============================================================================
// Step (execute one instruction)
// =============================================================================

bool CPU::step() {
    if (halted) return false;

    // Fetch
    Word raw = fetch();

    // Decode
    Instruction ins = decode(raw);
    last_ins = ins;

    // Check for halt (ecall)
    if (ins.type == InsType::ECALL) {
        halted = true;
        cycles++;
        instructions++;
        return false;
    }

    // Read registers
    Word rs1_val = regs.read(ins.rs1);
    Word rs2_val = regs.read(ins.rs2);

    // Execute
    Word alu_result = execute(ins, rs1_val, rs2_val);

    // Compute next PC
    Address next_pc = pc + 4;

    if (ins.jump) {
        if (ins.type == InsType::JAL) {
            next_pc = pc + ins.imm;
            alu_result = pc + 4;  // Return address
        } else if (ins.type == InsType::JALR) {
            next_pc = (rs1_val + ins.imm) & ~1;  // Clear LSB
            alu_result = pc + 4;  // Return address
        }
    } else if (ins.branch) {
        if (ALU::branch_taken(ins.type, rs1_val, rs2_val)) {
            next_pc = pc + ins.imm;
        }
    }

    // Memory
    Word mem_result = memory_access(ins, alu_result, rs2_val);

    // Writeback
    Word wb_result = ins.mem_to_reg ? mem_result : alu_result;
    writeback(ins, wb_result);

    // Update PC
    pc = next_pc;
    cycles++;
    instructions++;

    // Check breakpoint
    if (has_breakpoint(pc)) {
        return false;
    }

    return true;
}

// =============================================================================
// Run
// =============================================================================

void CPU::run() {
    while (step()) {}
}

// =============================================================================
// Accessors
// =============================================================================

Address CPU::get_pc() const { return pc; }
void CPU::set_pc(Address addr) { pc = addr; }
uint64_t CPU::get_cycle_count() const { return cycles; }
uint64_t CPU::get_instruction_count() const { return instructions; }
bool CPU::is_halted() const { return halted; }
const Instruction& CPU::get_last_instruction() const { return last_ins; }

// =============================================================================
// Breakpoints
// =============================================================================

void CPU::add_breakpoint(Address addr) {
    if (!has_breakpoint(addr)) {
        breakpoints.push_back(addr);
    }
}

void CPU::remove_breakpoint(Address addr) {
    breakpoints.erase(
        std::remove(breakpoints.begin(), breakpoints.end(), addr),
        breakpoints.end()
    );
}

void CPU::clear_breakpoints() {
    breakpoints.clear();
}

bool CPU::has_breakpoint(Address addr) const {
    return std::find(breakpoints.begin(), breakpoints.end(), addr) != breakpoints.end();
}
