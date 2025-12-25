/**
 * hazard_unit.cpp
 * 
 * Hazard detection and forwarding control implementation.
 */

#include "hazard_unit.hpp"
#include "decoder.hpp"

// =============================================================================
// Load-Use Hazard Detection
// =============================================================================

bool HazardUnit::detect_load_use(const ID_EX& id_ex, const Instruction& next_ins) {
    // Load-use hazard occurs when:
    // 1. Current instruction in ID/EX is a load (mem_read)
    // 2. Next instruction uses the load destination as source
    
    if (!id_ex.valid) return false;
    if (!id_ex.ins.mem_read) return false;
    if (id_ex.ins.rd == 0) return false;

    // Check if next instruction reads from load destination
    bool uses_rs1 = (next_ins.rs1 == id_ex.ins.rd) && (next_ins.rs1 != 0);
    bool uses_rs2 = (next_ins.rs2 == id_ex.ins.rd) && (next_ins.rs2 != 0);

    // For store instructions, rs2 is the data to store
    // We need the address (rs1) and data (rs2)
    if (next_ins.mem_write) {
        uses_rs2 = (next_ins.rs2 == id_ex.ins.rd) && (next_ins.rs2 != 0);
    }

    return uses_rs1 || uses_rs2;
}

// =============================================================================
// RAW Hazard Detection
// =============================================================================

bool HazardUnit::detect_raw(int rs, const EX_MEM& ex_mem, const MEM_WB& mem_wb) {
    if (rs == 0) return false;

    // Check EX/MEM stage
    if (ex_mem.valid && ex_mem.ins.reg_write && ex_mem.ins.rd == rs) {
        return true;
    }

    // Check MEM/WB stage
    if (mem_wb.valid && mem_wb.ins.reg_write && mem_wb.ins.rd == rs) {
        return true;
    }

    return false;
}

// =============================================================================
// Forwarding Logic
// =============================================================================

Forward HazardUnit::get_forward_rs1(const ID_EX& id_ex, const EX_MEM& ex_mem, const MEM_WB& mem_wb) {
    int rs1 = id_ex.ins.rs1;
    if (rs1 == 0) return Forward::NONE;

    // Priority: EX/MEM > MEM/WB (more recent instruction takes precedence)
    
    // Forward from EX/MEM
    if (ex_mem.valid && ex_mem.ins.reg_write && ex_mem.ins.rd != 0 && ex_mem.ins.rd == rs1) {
        return Forward::EX_MEM;
    }

    // Forward from MEM/WB
    if (mem_wb.valid && mem_wb.ins.reg_write && mem_wb.ins.rd != 0 && mem_wb.ins.rd == rs1) {
        return Forward::MEM_WB;
    }

    return Forward::NONE;
}

Forward HazardUnit::get_forward_rs2(const ID_EX& id_ex, const EX_MEM& ex_mem, const MEM_WB& mem_wb) {
    int rs2 = id_ex.ins.rs2;
    if (rs2 == 0) return Forward::NONE;

    // Forward from EX/MEM
    if (ex_mem.valid && ex_mem.ins.reg_write && ex_mem.ins.rd != 0 && ex_mem.ins.rd == rs2) {
        return Forward::EX_MEM;
    }

    // Forward from MEM/WB
    if (mem_wb.valid && mem_wb.ins.reg_write && mem_wb.ins.rd != 0 && mem_wb.ins.rd == rs2) {
        return Forward::MEM_WB;
    }

    return Forward::NONE;
}

// =============================================================================
// Control Hazard Detection
// =============================================================================

bool HazardUnit::detect_branch_hazard(const EX_MEM& ex_mem) {
    return ex_mem.valid && ex_mem.branch_taken;
}

// =============================================================================
// Stall and Flush Signals
// =============================================================================

bool HazardUnit::should_stall(const IF_ID& if_id, const ID_EX& id_ex) {
    if (!if_id.valid || !id_ex.valid) return false;
    
    Instruction next_ins = Decoder::decode(if_id.instruction, if_id.pc);
    return detect_load_use(id_ex, next_ins);
}

bool HazardUnit::should_flush(const EX_MEM& ex_mem) {
    return detect_branch_hazard(ex_mem);
}

// =============================================================================
// Debug Output
// =============================================================================

void HazardUnit::print_status(const IF_ID& if_id, const ID_EX& id_ex,
                               const EX_MEM& ex_mem, const MEM_WB& mem_wb) {
    std::cout << "Hazard Unit Status:\n";

    // Check for load-use hazard
    if (if_id.valid && id_ex.valid) {
        Instruction next_ins = Decoder::decode(if_id.instruction, if_id.pc);
        if (detect_load_use(id_ex, next_ins)) {
            std::cout << "  LOAD-USE HAZARD: stall required\n";
            std::cout << "    Load: " << id_ex.ins.text << " (rd=" << reg_name(id_ex.ins.rd) << ")\n";
            std::cout << "    Next: " << next_ins.text << "\n";
        }
    }

    // Check forwarding paths
    if (id_ex.valid) {
        Forward fwd_a = get_forward_rs1(id_ex, ex_mem, mem_wb);
        Forward fwd_b = get_forward_rs2(id_ex, ex_mem, mem_wb);

        if (fwd_a != Forward::NONE) {
            std::cout << "  FORWARD rs1 (" << reg_name(id_ex.ins.rs1) << ") from ";
            std::cout << (fwd_a == Forward::EX_MEM ? "EX/MEM" : "MEM/WB") << "\n";
        }

        if (fwd_b != Forward::NONE) {
            std::cout << "  FORWARD rs2 (" << reg_name(id_ex.ins.rs2) << ") from ";
            std::cout << (fwd_b == Forward::EX_MEM ? "EX/MEM" : "MEM/WB") << "\n";
        }
    }

    // Check control hazard
    if (detect_branch_hazard(ex_mem)) {
        std::cout << "  CONTROL HAZARD: branch taken, flush IF/ID and ID/EX\n";
        std::cout << "    Branch: " << ex_mem.ins.text << "\n";
        std::cout << "    Target: " << to_hex(ex_mem.branch_target) << "\n";
    }
}
