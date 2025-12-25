/**
 * pipeline.hpp
 * 
 * 5-stage pipelined CPU implementation.
 * Stages: IF -> ID -> EX -> MEM -> WB
 * Supports toggling hazard detection and forwarding.
 */

#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "common.hpp"
#include "memory.hpp"
#include "register_file.hpp"
#include "alu.hpp"
#include "decoder.hpp"
#include "hazard_unit.hpp"

class Pipeline {
public:
    Pipeline(Memory& mem, RegisterFile& regs);
    void reset();

    // Execute one cycle
    bool cycle();

    // Run until halt or breakpoint
    void run();

    // Control toggles
    void set_hazard_detection(bool enabled);
    void set_forwarding(bool enabled);
    bool get_hazard_detection() const;
    bool get_forwarding() const;

    // State access
    Address get_pc() const;
    void set_pc(Address addr);
    uint64_t get_cycle_count() const;
    uint64_t get_instruction_count() const;
    bool is_halted() const;
    bool is_stalled() const;

    // Pipeline register access (for display)
    const IF_ID& get_if_id() const;
    const ID_EX& get_id_ex() const;
    const EX_MEM& get_ex_mem() const;
    const MEM_WB& get_mem_wb() const;

    // Display pipeline state
    void print_state() const;

    // Breakpoints
    void add_breakpoint(Address addr);
    void remove_breakpoint(Address addr);
    void clear_breakpoints();
    bool has_breakpoint(Address addr) const;

    // Statistics
    uint64_t get_stall_count() const;
    uint64_t get_flush_count() const;
    uint64_t get_forward_count() const;

private:
    Memory& mem;
    RegisterFile& regs;

    // Pipeline registers
    IF_ID if_id;
    ID_EX id_ex;
    EX_MEM ex_mem;
    MEM_WB mem_wb;

    // PC
    Address pc;
    Address next_pc;

    // Control
    bool hazard_detection;
    bool forwarding;
    bool halted;
    bool stalled;

    // Statistics
    uint64_t cycles;
    uint64_t instructions;
    uint64_t stalls;
    uint64_t flushes;
    uint64_t forwards;

    // Breakpoints
    std::vector<Address> breakpoints;

    // Stage implementations
    void stage_if();
    void stage_id();
    void stage_ex();
    void stage_mem();
    void stage_wb();

    // Hazard detection
    bool detect_load_use_hazard();
    bool detect_control_hazard();

    // Forwarding
    Forward get_forward_a();
    Forward get_forward_b();
    Word get_forwarded_value(Forward fwd, Word reg_val);
};

#endif // PIPELINE_HPP
