/**
 * alu.hpp
 * 
 * Arithmetic Logic Unit.
 * Performs all arithmetic, logical, shift, and comparison operations.
 * Includes M extension (multiply/divide).
 */

#ifndef ALU_HPP
#define ALU_HPP

#include "common.hpp"

class ALU {
public:
    // Execute an ALU operation
    static Word execute(AluOp op, Word a, Word b);

    // Evaluate branch condition
    static bool branch_taken(InsType type, Word rs1_val, Word rs2_val);

    // Get description of operation (for debugging)
    static std::string op_name(AluOp op);
};

#endif // ALU_HPP
