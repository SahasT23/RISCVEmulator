/**
 * hazard_unit.hpp
 * 
 * Hazard detection and forwarding control unit.
 * Detects data hazards and control hazards.
 * Determines forwarding paths and stall signals.
 */

#ifndef HAZARD_UNIT_HPP
#define HAZARD_UNIT_HPP

#include "common.hpp"

class HazardUnit {
public:
    // Detect load-use hazard (requires stall)
    static bool detect_load_use(const ID_EX& id_ex, const Instruction& next_ins);

    // Detect RAW hazard (may require forwarding or stall)
    static bool detect_raw(int rs, const EX_MEM& ex_mem, const MEM_WB& mem_wb);

    // Determine forwarding source for rs1
    static Forward get_forward_rs1(const ID_EX& id_ex, const EX_MEM& ex_mem, const MEM_WB& mem_wb);

    // Determine forwarding source for rs2
    static Forward get_forward_rs2(const ID_EX& id_ex, const EX_MEM& ex_mem, const MEM_WB& mem_wb);

    // Check if branch causes control hazard
    static bool detect_branch_hazard(const EX_MEM& ex_mem);

    // Get stall signal
    static bool should_stall(const IF_ID& if_id, const ID_EX& id_ex);

    // Get flush signal
    static bool should_flush(const EX_MEM& ex_mem);

    // Print hazard status (for debugging)
    static void print_status(const IF_ID& if_id, const ID_EX& id_ex, 
                             const EX_MEM& ex_mem, const MEM_WB& mem_wb);
};

#endif // HAZARD_UNIT_HPP
