// Tests arithmetic operations, flag setting and conditional branching

// Initialize registers with test values
movz    X0, #0            // X0 = 0
movz    X1, #10           // X1 = 10
movz    X2, #0xF000       // X2 = 0xF000 (large immediate)
movz    X3, #1            // X3 = 1
movz    X4, #0xFF         // X4 = 0xFF
movz    X5, #5            // X5 = 5 (counter)

// Store initial values to memory for later verification
stur    X1, [X0, #16]     // Store X1 at address 16
stur    X2, [X0, #24]     // Store X2 at address 24
stur    X3, [X0, #32]     // Store X3 at address 32

// Loop to test decrements and branches
loop_start:
    subs    X5, X5, #1        // Decrement counter, update flags
    add     X3, X3, X3        // X3 = X3 * 2 (doubles each iteration)
    cbnz    X5, loop_continue // Skip next add if counter not zero
    adds    X4, X4, #1        // Only executes when X5 = 0
loop_continue:
    bne     loop_start        // Branch if not equal (loops until X5 = 0)

// Test logical operations
ands    X6, X4, X2        // X6 = X4 & X2, sets flags
eor     X7, X1, X3        // X7 = X1 ^ X3
orr     X8, X2, X4        // X8 = X2 | X4

// Test shifts
lsl     X9, X3, #4        // X9 = X3 << 4
lsr     X10, X2, #8       // X10 = X2 >> 8

// Test memory operations
stur    X3, [X0, #40]     // Store final X3 value
sturh   X7, [X0, #48]     // Store halfword
sturb   X4, [X0, #56]     // Store byte

// Test loads
ldur    X11, [X0, #16]    // Load X1's initial value
ldurh   X12, [X0, #48]    // Load halfword
ldurb   X13, [X0, #56]    // Load byte

// Compare results
cmp     X11, X1           // Compare loaded value with original
beq     compare_ok        // Branch if equal
movz    X14, #0xFFFF      // Error code if comparison fails
b       end_test
compare_ok:
movz    X14, #0xAAAA      // Success code

// Final calculation for verification
// X3 should be 16, X5 should be 0, X14 should be 0xAAAA
mul     X15, X3, X5       // X15 = X3 * X5 = 0
adds    X15, X15, X14     // Final result = X15 + X14 = 0xAAAA

end_test:
hlt     #0