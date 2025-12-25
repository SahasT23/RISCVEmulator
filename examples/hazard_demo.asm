# hazard_demo.asm
# Demonstrates various pipeline hazards
# Use: mode p, then step through with 'pipeline' command
# Try with forwarding on/off and hazards on/off

.text

# ============================================
# Demo 1: RAW Hazard (Read After Write)
# Instruction 2 needs result from instruction 1
# ============================================
raw_demo:
    addi    t0, zero, 10    # t0 = 10
    addi    t1, t0, 5       # t1 = t0 + 5 (needs t0 from previous)
    addi    t2, t1, 3       # t2 = t1 + 3 (needs t1 from previous)
    nop

# ============================================
# Demo 2: Load-Use Hazard
# Cannot forward from MEM stage to EX stage
# Requires 1 stall even with forwarding
# ============================================
load_use_demo:
    li      t0, 0x10000000  # data address
    sw      t1, 0(t0)       # store t1 to memory
    lw      t3, 0(t0)       # load into t3
    addi    t4, t3, 1       # use t3 immediately (load-use hazard!)
    nop

# ============================================
# Demo 3: No Hazard (sufficient distance)
# Two instructions between producer and consumer
# ============================================
no_hazard_demo:
    addi    t0, zero, 20    # t0 = 20
    addi    t5, zero, 1     # unrelated
    addi    t6, zero, 2     # unrelated
    addi    t1, t0, 5       # t1 = t0 + 5 (no hazard, t0 ready)
    nop

# ============================================
# Demo 4: Control Hazard (Branch)
# Branch taken causes pipeline flush
# ============================================
control_demo:
    li      t0, 5
    li      t1, 5
    beq     t0, t1, branch_target   # branch taken!
    addi    t2, zero, 999   # should NOT execute (flushed)
    addi    t3, zero, 888   # should NOT execute (flushed)

branch_target:
    addi    t4, zero, 42    # execution continues here
    nop

# ============================================
# Demo 5: Multiple Hazards
# Shows forwarding from both EX/MEM and MEM/WB
# ============================================
multi_forward_demo:
    addi    t0, zero, 1     # t0 = 1
    addi    t1, zero, 2     # t1 = 2
    add     t2, t0, t1      # needs t0 from MEM/WB, t1 from EX/MEM
    nop

# ============================================
# Demo 6: Store After Load (no hazard)
# Store reads register, doesn't write
# ============================================
store_after_load:
    li      t0, 0x10000000
    lw      t1, 0(t0)       # load t1
    sw      t1, 4(t0)       # store t1 (needs forwarding for t1)
    nop

# End
done:
    ecall
