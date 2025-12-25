# fibonacci.asm
# Computes the nth Fibonacci number iteratively
# Result in a0

.text
main:
    li      a0, 10          # n = 10
    jal     ra, fibonacci   # call fibonacci(10)
    ecall                   # halt (result in a0)

# fibonacci(n) - computes F(n)
# Input: a0 = n
# Output: a0 = F(n)
# F(0) = 0, F(1) = 1, F(n) = F(n-1) + F(n-2)
fibonacci:
    # Handle base cases
    li      t0, 1
    beq     a0, zero, fib_zero
    beq     a0, t0, fib_one

    # Iterative computation
    li      t1, 0           # F(i-2) = 0
    li      t2, 1           # F(i-1) = 1
    li      t3, 2           # i = 2

fib_loop:
    bgt     t3, a0, fib_done
    add     t4, t1, t2      # F(i) = F(i-2) + F(i-1)
    mv      t1, t2          # F(i-2) = F(i-1)
    mv      t2, t4          # F(i-1) = F(i)
    addi    t3, t3, 1       # i++
    j       fib_loop

fib_done:
    mv      a0, t2          # return F(n)
    ret

fib_zero:
    li      a0, 0
    ret

fib_one:
    li      a0, 1
    ret
