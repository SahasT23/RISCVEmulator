# factorial.asm
# Computes factorial of n (n!)
# Result in a0

.text
main:
    li      a0, 5           # n = 5
    jal     ra, factorial   # call factorial(5)
    ecall                   # halt (result in a0)

# factorial(n) - computes n!
# Input: a0 = n
# Output: a0 = n!
factorial:
    # Base case: if n <= 1, return 1
    li      t0, 1
    ble     a0, t0, base_case

    # Save ra and n on stack
    addi    sp, sp, -8
    sw      ra, 4(sp)
    sw      a0, 0(sp)

    # Recursive call: factorial(n-1)
    addi    a0, a0, -1
    jal     ra, factorial

    # Multiply result by n
    lw      t0, 0(sp)       # restore n
    mul     a0, a0, t0      # a0 = factorial(n-1) * n

    # Restore ra and stack
    lw      ra, 4(sp)
    addi    sp, sp, 8
    ret

base_case:
    li      a0, 1
    ret
