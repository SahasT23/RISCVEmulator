/**
 * pipeline.cpp
 * 
 * 5-stage pipelined CPU implementation.
 */

#include "pipeline.hpp"
#include <algorithm>

Pipeline::Pipeline(Memory& mem, RegisterFile& regs)
    : mem(mem), regs(regs), pc(Memory::TEXT_BASE), next_pc(Memory::TEXT_BASE + 4),
      hazard_detection(true), forwarding(true), halted(false), stalled(false),
      cycles(0), instructions(0), stalls(0), flushes(0), forwards(0) {}

void Pipeline::reset() {
    pc = Memory::TEXT_BASE;
    next_pc = Memory::TEXT_BASE + 4;
    halted = false;
    stalled = false;
    cycles = 0;
    instructions = 0;
    stalls = 0;
    flushes = 0;
    forwards = 0;

    if_id.flush();
    id_ex.flush();
    ex_mem.flush();
    mem_wb.flush();

    regs.reset();
}

// =============================================================================
// Forwarding Logic
// =============================================================================

Forward Pipeline::get_forward_a() {
    if (!forwarding) return Forward::NONE;

    int rs1 = id_ex.ins.rs1;
    if (rs1 == 0) return Forward::NONE;

    // Forward from EX/MEM
    if (ex_mem.valid && ex_mem.ins.reg_write && ex_mem.ins.rd == rs1) {
        forwards++;
        return Forward::EX_MEM;
    }

    // Forward from MEM/WB
    if (mem_wb.valid && mem_wb.ins.reg_write && mem_wb.ins.rd == rs1) {
        forwards++;
        return Forward::MEM_WB;
    }

    return Forward::NONE;
}

Forward Pipeline::get_forward_b() {
    if (!forwarding) return Forward::NONE;

    int rs2 = id_ex.ins.rs2;
    if (rs2 == 0) return Forward::NONE;

    // Forward from EX/MEM
    if (ex_mem.valid && ex_mem.ins.reg_write && ex_mem.ins.rd == rs2) {
        forwards++;
        return Forward::EX_MEM;
    }

    // Forward from MEM/WB
    if (mem_wb.valid && mem_wb.ins.reg_write && mem_wb.ins.rd == rs2) {
        forwards++;
        return Forward::MEM_WB;
    }

    return Forward::NONE;
}

Word Pipeline::get_forwarded_value(Forward fwd, Word reg_val) {
    switch (fwd) {
        case Forward::EX_MEM:
            return ex_mem.alu_result;
        case Forward::MEM_WB:
            return mem_wb.ins.mem_to_reg ? mem_wb.mem_data : mem_wb.alu_result;
        default:
            return reg_val;
    }
}

// =============================================================================
// Hazard Detection
// =============================================================================

bool Pipeline::detect_load_use_hazard() {
    if (!hazard_detection) return false;

    // Load-use hazard: ID/EX has a load, and IF/ID needs that register
    if (id_ex.valid && id_ex.ins.mem_read) {
        Instruction next_ins = Decoder::decode(if_id.instruction, if_id.pc);
        
        // Check if next instruction uses the load destination
        if (id_ex.ins.rd != 0) {
            if (id_ex.ins.rd == next_ins.rs1 || id_ex.ins.rd == next_ins.rs2) {
                return true;
            }
        }
    }
    return false;
}

bool Pipeline::detect_control_hazard() {
    // Control hazard detected when branch/jump is taken in EX stage
    return ex_mem.branch_taken;
}

// =============================================================================
// IF Stage
// =============================================================================

void Pipeline::stage_if() {
    if (stalled) return;

    if_id.instruction = mem.read_word(pc);
    if_id.pc = pc;
    if_id.next_pc = pc + 4;
    if_id.valid = true;

    pc = next_pc;
    next_pc = pc + 4;
}

// =============================================================================
// ID Stage
// =============================================================================

void Pipeline::stage_id() {
    if (!if_id.valid) {
        id_ex.flush();
        return;
    }

    Instruction ins = Decoder::decode(if_id.instruction, if_id.pc);

    id_ex.ins = ins;
    id_ex.rs1_val = regs.read(ins.rs1);
    id_ex.rs2_val = regs.read(ins.rs2);
    id_ex.pc = if_id.pc;
    id_ex.next_pc = if_id.next_pc;
    id_ex.valid = true;
}

// =============================================================================
// EX Stage
// =============================================================================

void Pipeline::stage_ex() {
    if (!id_ex.valid) {
        ex_mem.flush();
        return;
    }

    Instruction ins = id_ex.ins;

    // Get operand values (with forwarding if enabled)
    Forward fwd_a = get_forward_a();
    Forward fwd_b = get_forward_b();

    Word rs1_val = get_forwarded_value(fwd_a, id_ex.rs1_val);
    Word rs2_val = get_forwarded_value(fwd_b, id_ex.rs2_val);

    // ALU inputs
    Word alu_a = rs1_val;
    Word alu_b = ins.alu_src ? static_cast<Word>(ins.imm) : rs2_val;

    // AUIPC uses PC
    if (ins.type == InsType::AUIPC) {
        alu_a = id_ex.pc;
    }

    // ALU operation
    Word alu_result = ALU::execute(ins.alu_op, alu_a, alu_b);

    // Branch/jump handling
    Address branch_target = 0;
    bool branch_taken = false;

    if (ins.jump) {
        if (ins.type == InsType::JAL) {
            branch_target = id_ex.pc + ins.imm;
            branch_taken = true;
            alu_result = id_ex.pc + 4;
        } else if (ins.type == InsType::JALR) {
            branch_target = (rs1_val + ins.imm) & ~1;
            branch_taken = true;
            alu_result = id_ex.pc + 4;
        }
    } else if (ins.branch) {
        if (ALU::branch_taken(ins.type, rs1_val, rs2_val)) {
            branch_target = id_ex.pc + ins.imm;
            branch_taken = true;
        }
    }

    // Update pipeline register
    ex_mem.ins = ins;
    ex_mem.alu_result = alu_result;
    ex_mem.rs2_val = rs2_val;
    ex_mem.branch_target = branch_target;
    ex_mem.branch_taken = branch_taken;
    ex_mem.valid = true;

    // Handle control hazard (branch taken)
    if (branch_taken) {
        next_pc = branch_target;
        pc = branch_target;
        // Flush IF/ID and ID/EX
        if_id.flush();
        id_ex.flush();
        flushes += 2;
    }
}

// =============================================================================
// MEM Stage
// =============================================================================

void Pipeline::stage_mem() {
    if (!ex_mem.valid) {
        mem_wb.flush();
        return;
    }

    Instruction ins = ex_mem.ins;
    Address addr = ex_mem.alu_result;
    Word mem_data = 0;

    // Memory read
    if (ins.mem_read) {
        switch (ins.type) {
            case InsType::LB:  mem_data = static_cast<Word>(mem.read_byte_signed(addr)); break;
            case InsType::LH:  mem_data = static_cast<Word>(mem.read_half_signed(addr)); break;
            case InsType::LW:  mem_data = mem.read_word(addr); break;
            case InsType::LBU: mem_data = mem.read_byte(addr); break;
            case InsType::LHU: mem_data = mem.read_half(addr); break;
            default: break;
        }
    }

    // Memory write
    if (ins.mem_write) {
        Word val = ex_mem.rs2_val;
        switch (ins.type) {
            case InsType::SB: mem.write_byte(addr, val & 0xFF); break;
            case InsType::SH: mem.write_half(addr, val & 0xFFFF); break;
            case InsType::SW: mem.write_word(addr, val); break;
            default: break;
        }
    }

    // Update pipeline register
    mem_wb.ins = ins;
    mem_wb.alu_result = ex_mem.alu_result;
    mem_wb.mem_data = mem_data;
    mem_wb.valid = true;

    // Count completed instruction
    if (ins.type != InsType::UNKNOWN && !ins.is_nop()) {
        instructions++;
    }
}

// =============================================================================
// WB Stage
// =============================================================================

void Pipeline::stage_wb() {
    if (!mem_wb.valid) return;

    Instruction ins = mem_wb.ins;

    if (ins.reg_write && ins.rd != 0) {
        Word result = ins.mem_to_reg ? mem_wb.mem_data : mem_wb.alu_result;
        regs.write(ins.rd, result);
    }

    // Check for halt
    if (ins.type == InsType::ECALL) {
        halted = true;
    }
}

// =============================================================================
// Cycle
// =============================================================================

bool Pipeline::cycle() {
    if (halted) return false;

    // Check for load-use hazard
    stalled = detect_load_use_hazard();

    if (stalled) {
        stalls++;
        // Stall: keep IF/ID, insert bubble in ID/EX
        stage_wb();
        stage_mem();
        stage_ex();
        id_ex.flush();  // Insert bubble
        // Don't advance IF or ID
    } else {
        // Normal pipeline advance (in reverse order to avoid overwrites)
        stage_wb();
        stage_mem();
        stage_ex();
        stage_id();
        stage_if();
    }

    cycles++;

    // Check breakpoint
    if (has_breakpoint(pc)) {
        return false;
    }

    return !halted;
}

// =============================================================================
// Run
// =============================================================================

void Pipeline::run() {
    while (cycle()) {}
}

// =============================================================================
// Control
// =============================================================================

void Pipeline::set_hazard_detection(bool enabled) { hazard_detection = enabled; }
void Pipeline::set_forwarding(bool enabled) { forwarding = enabled; }
bool Pipeline::get_hazard_detection() const { return hazard_detection; }
bool Pipeline::get_forwarding() const { return forwarding; }

// =============================================================================
// State Access
// =============================================================================

Address Pipeline::get_pc() const { return pc; }
void Pipeline::set_pc(Address addr) { pc = addr; next_pc = addr + 4; }
uint64_t Pipeline::get_cycle_count() const { return cycles; }
uint64_t Pipeline::get_instruction_count() const { return instructions; }
bool Pipeline::is_halted() const { return halted; }
bool Pipeline::is_stalled() const { return stalled; }

const IF_ID& Pipeline::get_if_id() const { return if_id; }
const ID_EX& Pipeline::get_id_ex() const { return id_ex; }
const EX_MEM& Pipeline::get_ex_mem() const { return ex_mem; }
const MEM_WB& Pipeline::get_mem_wb() const { return mem_wb; }

// =============================================================================
// Display
// =============================================================================

void Pipeline::print_state() const {
    std::cout << "Cycle " << cycles << ":\n";

    auto print_stage = [](const char* name, bool valid, Address pc, const std::string& text) {
        std::cout << "  " << name << ": ";
        if (valid) {
            std::cout << "[" << to_hex(pc) << "] " << text << "\n";
        } else {
            std::cout << "(bubble)\n";
        }
    };

    print_stage("IF ", if_id.valid, if_id.pc,
                if_id.valid ? Decoder::decode(if_id.instruction, if_id.pc).text : "");
    print_stage("ID ", id_ex.valid, id_ex.pc, id_ex.ins.text);
    print_stage("EX ", ex_mem.valid, ex_mem.ins.pc, ex_mem.ins.text);
    print_stage("MEM", mem_wb.valid, mem_wb.ins.pc, mem_wb.ins.text);

    std::cout << "  WB : ";
    if (mem_wb.valid && mem_wb.ins.reg_write) {
        std::cout << reg_name(mem_wb.ins.rd) << " <- "
                  << to_hex(mem_wb.ins.mem_to_reg ? mem_wb.mem_data : mem_wb.alu_result) << "\n";
    } else {
        std::cout << "(none)\n";
    }
}

// =============================================================================
// Breakpoints
// =============================================================================

void Pipeline::add_breakpoint(Address addr) {
    if (!has_breakpoint(addr)) breakpoints.push_back(addr);
}

void Pipeline::remove_breakpoint(Address addr) {
    breakpoints.erase(std::remove(breakpoints.begin(), breakpoints.end(), addr), breakpoints.end());
}

void Pipeline::clear_breakpoints() { breakpoints.clear(); }

bool Pipeline::has_breakpoint(Address addr) const {
    return std::find(breakpoints.begin(), breakpoints.end(), addr) != breakpoints.end();
}

// =============================================================================
// Statistics
// =============================================================================

uint64_t Pipeline::get_stall_count() const { return stalls; }
uint64_t Pipeline::get_flush_count() const { return flushes; }
uint64_t Pipeline::get_forward_count() const { return forwards; }
